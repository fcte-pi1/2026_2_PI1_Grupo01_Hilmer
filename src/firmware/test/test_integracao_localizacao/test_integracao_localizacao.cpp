/// @file test_integracao_localizacao.cpp
/// @brief Testes de integração da correção do erro acumulado pelas paredes (#188), com o robô
/// simulado andando de verdade no labirinto (`SimulacaoCorrida.h`, a mesma cadeia do `main.cpp`).
///
/// Cada cenário roda com e sem a parte da #188 que ele verifica, para mostrar o efeito: a posição
/// lateral estimada, a correção ao longo do avanço com os encoders, o realinhamento antes dos giros
/// e o rumo pelo corredor contra a deriva do giroscópio. Os números aparecem no log (`TEST_MESSAGE`).
/// Roda no computador: `pio test -e native -f test_integracao_localizacao`.

#include <unity.h>

#include <cmath>
#include <cstdio>

#include "../test_integracao_execucao/SimulacaoCorrida.h"
#include "../test_labirinto_simulado/LabirintosExemplo.h"

using namespace micromouse;
using namespace micromouse::simulacao_corrida;

void setUp() {}
void tearDown() {}

namespace {

/// Robô mais lento que a tabela, com as rodas desiguais (o mesmo da #184).
ParametrosRoboSimulado roboImperfeito() {
  ParametrosRoboSimulado p;
  p.fatorEsquerda = 0.92;
  p.fatorDireita = 0.97;
  return p;
}

/// Sem a #188: sem correção lateral, sem realinhamento e sem rumo pelo corredor.
OpcoesSimulacao sem188(OpcoesSimulacao o) {
  o.correcaoParedes = false;
  o.realinhamento.desvioMinimoMm = UINT16_MAX;
  o.realinhamento.ajusteRumoPeloCorredorPorMil = 0;
  return o;
}

void registrar(const char* texto) { TEST_MESSAGE(texto); }

/// Diferença entre a posição lateral estimada e a real, em mm, no referencial do robô.
double erroLateralEstimadoMm(Simulacao& sim) {
  const int64_t estimadaUm = posicaoLateralUm(sim.odometria());
  const PosicaoCelula c = sim.robo().celula();
  // A posição lateral real no mesmo referencial: o centro da célula mais o desvio real.
  const Direcao d = sim.odometria().pose().direcao;
  const int64_t centroUm = (d == Direcao::Norte || d == Direcao::Sul) ? static_cast<int64_t>(c.coluna) * 180000
                                                                       : static_cast<int64_t>(c.linha) * 180000;
  const int64_t sinal = (d == Direcao::Norte || d == Direcao::Oeste) ? 1 : -1;
  const double realUm = static_cast<double>(sinal * centroUm) + sim.robo().erroLateralMm() * 1000.0;
  return (static_cast<double>(estimadaUm) - realUm) / 1000.0;
}

struct ResultadoCorrida {
  bool chegou = false;
  bool celulaCerta = false;
  bool contato = false;
  uint32_t msContato = 0;
  double tempoS = 0.0;
  uint32_t realinhamentos = 0;
};

ResultadoCorrida correr(const char* labirinto, const OpcoesSimulacao& o) {
  Simulacao sim(labirinto, o);
  sim.correr();
  ResultadoCorrida r;
  const PosicaoCelula objetivo = sim.ciclo().navegacao().obterObjetivo();
  r.chegou = sim.ciclo().estado() == EstadoCiclo::ObjetivoAlcancado;
  r.celulaCerta = sim.robo().celula().linha == sim.odometria().pose().celula.linha &&
                  sim.robo().celula().coluna == sim.odometria().pose().celula.coluna &&
                  sim.robo().celula().linha == objetivo.linha && sim.robo().celula().coluna == objetivo.coluna;
  r.contato = sim.robo().colidiu();
  r.msContato = sim.robo().msEmContato();
  r.tempoS = sim.agoraMs() / 1000.0;
  r.realinhamentos = sim.ciclo().realinhamentos();
  return r;
}

}  // namespace

// ---- Critério 1: ao detectar paredes laterais, a posição é ajustada ao centro da célula

/// A odometria só soma o avanço na direção atual: um robô largado 8 mm fora do centro fica com a
/// posição lateral errada para sempre. Com a correção, as paredes laterais a acertam.
void test_posicao_lateral_estimada_segue_a_real_pelas_paredes() {
  OpcoesSimulacao o;
  o.robo = roboImperfeito();
  o.desvioLateralMm = 8.0;
  for (int comCorrecao = 0; comCorrecao < 2; comCorrecao++) {
    Simulacao sim(exemplos::LABIRINTO_4X4, comCorrecao ? o : sem188(o));
    Direcao direcao = Direcao::Norte;
    sim.esperar(200, direcao);
    const double inicio = erroLateralEstimadoMm(sim);
    sim.executar(Movimento::AvancarCelula, direcao);
    sim.esperar(200, direcao);
    const double fim = erroLateralEstimadoMm(sim);
    char msg[160];
    std::snprintf(msg, sizeof(msg), "%s correcao: erro lateral estimado %.1f mm parado e %.1f mm depois de uma celula",
                  comCorrecao ? "com" : "sem", inicio, fim);
    registrar(msg);
    if (comCorrecao) {
      TEST_ASSERT_TRUE_MESSAGE(std::fabs(inicio) <= 1.5 && std::fabs(fim) <= 1.5, msg);
      TEST_ASSERT_GREATER_THAN_UINT32(0, sim.correcao().correcoesLaterais());
    } else {
      TEST_ASSERT_TRUE_MESSAGE(std::fabs(inicio) >= 7.0, msg);
    }
  }
}

/// Com os encoders e 5% de erro de escala, cada avanço acumula erro na estimativa; a parede da
/// frente e as quinas (agora aplicadas direto na odometria, #188) a corrigem. Três células pelo
/// corredor da coluna 0 do 4×4, até a parede da borda Norte.
void test_com_encoders_a_correcao_segura_o_erro_de_escala() {
  OpcoesSimulacao o;
  o.encoders = true;
  o.fatorEncoder = 1.05;
  double erros[2] = {0.0, 0.0};
  for (int comCorrecao = 0; comCorrecao < 2; comCorrecao++) {
    OpcoesSimulacao opcoes = o;
    opcoes.correcaoParedes = comCorrecao == 1;
    Simulacao sim(exemplos::LABIRINTO_4X4, opcoes);
    Direcao direcao = Direcao::Norte;
    for (int i = 0; i < 3; i++) {
      sim.executar(Movimento::AvancarCelula, direcao);
    }
    sim.esperar(200, direcao);
    // Posição estimada no eixo contra a real (o centro da linha real mais o erro ao longo).
    const double realMm = sim.robo().celula().linha * 180.0 + sim.robo().erroAoLongoMm();
    erros[comCorrecao] = sim.odometria().posicaoNorteMicrometros() / 1000.0 - realMm;
    TEST_ASSERT_EQUAL_UINT8(3, sim.robo().celula().linha);
  }
  char msg[160];
  std::snprintf(msg, sizeof(msg), "encoders +5%%, 3 celulas: erro da estimativa ao longo %.1f mm sem correcao e %.1f mm com",
                erros[0], erros[1]);
  registrar(msg);
  TEST_ASSERT_TRUE_MESSAGE(std::fabs(erros[1]) <= 3.0, msg);
  TEST_ASSERT_TRUE_MESSAGE(std::fabs(erros[0]) >= 15.0, msg);
}

// ---- Realinhamento ao centro antes dos giros

/// 8 mm fora do centro, o giro de 90° encosta a quina (folga de 5,9 mm). O realinhamento leva o
/// robô ao centro sem encostar, sem sair do lugar no eixo e sem mudar o rumo.
void test_realinhamento_leva_ao_centro_e_o_giro_nao_encosta() {
  OpcoesSimulacao o;
  o.inicial = Pose{{1, 0}, Direcao::Norte};
  o.desvioLateralMm = 8.0;
  Simulacao sim(exemplos::LABIRINTO_4X4, o);
  Direcao direcao = Direcao::Norte;
  sim.esperar(200, direcao);
  // Atrás de (1, 0) está livre (a célula (0, 0)): o plano começa de ré, com o espaço maior.
  const PlanoRealinhamento plano = planejarRealinhamento(8000, true, true, TRECHO_MAXIMO_REALINHAMENTO_LIVRE_MM,
                                                         FOLGA_MINIMA_REALINHAMENTO_MM);
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, sim.realinhar(plano, direcao));
  char msg[200];
  std::snprintf(msg, sizeof(msg), "realinhamento de 8 mm: lateral %.1f mm, ao longo %.1f mm, rumo %.2f graus em %u ms",
                sim.robo().erroLateralMm(), sim.robo().erroAoLongoMm(), sim.robo().erroRumoGraus(),
                static_cast<unsigned>(sim.executor().tempoMs()));
  registrar(msg);
  TEST_ASSERT_TRUE_MESSAGE(std::fabs(sim.robo().erroLateralMm()) <= 2.0, msg);
  TEST_ASSERT_TRUE_MESSAGE(std::fabs(sim.robo().erroAoLongoMm()) <= 4.0, msg);
  TEST_ASSERT_TRUE_MESSAGE(std::fabs(sim.robo().erroRumoGraus()) <= 2.0, msg);
  TEST_ASSERT_FALSE(sim.robo().colidiu());
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, sim.executar(Movimento::Girar90Direita, direcao));
  TEST_ASSERT_FALSE_MESSAGE(sim.robo().colidiu(), "o giro depois do realinhamento encostou");
}

/// Num beco (paredes na frente e atrás), o trecho cabe entre a quina do chassi girado e a parede.
void test_realinhamento_num_beco_cabe_no_espaco_curto() {
  OpcoesSimulacao o;
  // (0, 0) virado para o Leste: parede na frente (Leste) e atrás (perímetro Oeste).
  o.inicial = Pose{{0, 0}, Direcao::Leste};
  o.desvioLateralMm = -6.0;
  Simulacao sim(exemplos::LABIRINTO_4X4, o);
  Direcao direcao = Direcao::Leste;
  sim.esperar(200, direcao);
  const PlanoRealinhamento plano = planejarRealinhamento(-6000, true, false, TRECHO_MAXIMO_REALINHAMENTO_LIVRE_MM,
                                                         FOLGA_MINIMA_REALINHAMENTO_MM);
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, sim.realinhar(plano, direcao));
  char msg[200];
  std::snprintf(msg, sizeof(msg), "realinhamento num beco (%.0f graus, trecho %.0f mm): lateral %.1f mm, ao longo %.1f mm",
                plano.anguloMiligraus / 1000.0, plano.trechoUm / 1000.0, sim.robo().erroLateralMm(),
                sim.robo().erroAoLongoMm());
  registrar(msg);
  // O espaço curto corrige só parte: o ciclo realinharia de novo. Não pode encostar.
  TEST_ASSERT_TRUE_MESSAGE(std::fabs(sim.robo().erroLateralMm()) < 6.0, msg);
  TEST_ASSERT_FALSE_MESSAGE(sim.robo().colidiu(), msg);
}

/// Critério 2 (simulação): corridas completas com o robô imperfeito e a largada torta. Com a #188,
/// a célula estimada coincide com a real e o chassi não encosta; sem ela, encosta.
void test_corridas_com_robo_imperfeito_nao_encostam_e_a_celula_coincide() {
  struct Caso {
    const char* labirinto;
    const char* nome;
    double lateral;
    double rumo;
  };
  const Caso casos[] = {{exemplos::LABIRINTO_4X4, "4x4", 8.0, 3.0},
                        {exemplos::LABIRINTO_8X4, "8x4", -8.0, -2.0},
                        {exemplos::LABIRINTO_12X4, "12x4", 8.0, -3.0}};
  uint32_t contatosSem = 0;
  for (const Caso& c : casos) {
    OpcoesSimulacao o;
    o.robo = roboImperfeito();
    o.desvioLateralMm = c.lateral;
    o.desvioRumoGraus = c.rumo;
    const ResultadoCorrida com = correr(c.labirinto, o);
    const ResultadoCorrida sem = correr(c.labirinto, sem188(o));
    contatosSem += sem.contato ? 1 : 0;
    char msg[220];
    std::snprintf(msg, sizeof(msg),
                  "%s: com #188 chegou=%d celula certa=%d contato=%u ms em %.1f s (%u realinhamentos); sem: contato=%u ms em %.1f s",
                  c.nome, com.chegou, com.celulaCerta, static_cast<unsigned>(com.msContato), com.tempoS,
                  static_cast<unsigned>(com.realinhamentos), static_cast<unsigned>(sem.msContato), sem.tempoS);
    registrar(msg);
    TEST_ASSERT_TRUE_MESSAGE(com.chegou && com.celulaCerta, msg);
    TEST_ASSERT_FALSE_MESSAGE(com.contato, msg);
    TEST_ASSERT_TRUE(com.tempoS < 600.0);
  }
  TEST_ASSERT_GREATER_THAN_UINT32(0, contatosSem);
}

// ---- Rumo pelo corredor (deriva do giroscópio)

/// O giroscópio deriva 0,1 °/s depois da calibração. Com o rumo pelo corredor, as paredes seguram o
/// rumo; sem ele, o rumo se perde e o chassi encosta. Mesmas 18 largadas da varredura reduzida.
void test_rumo_pelo_corredor_segura_a_deriva_do_giroscopio() {
  uint32_t contatos[2] = {0, 0};
  uint32_t falhas = 0;
  const double rumos[] = {-2.0, 0.0, 2.0};
  const double laterais[] = {0.0, 8.0};
  for (int comRumo = 0; comRumo < 2; comRumo++) {
    for (int r = 0; r < 3; r++) {
      for (double rumo : rumos) {
        for (double lateral : laterais) {
          OpcoesSimulacao o;
          if (r == 1) {
            o.robo = roboImperfeito();
          } else if (r == 2) {
            o.robo.fatorEsquerda = 1.03;
            o.robo.fatorDireita = 0.95;
          }
          o.desvioRumoGraus = rumo;
          o.desvioLateralMm = lateral;
          o.derivaGiroDecimosDps = 1;
          if (comRumo == 0) {
            o.realinhamento.ajusteRumoPeloCorredorPorMil = 0;
          }
          const ResultadoCorrida c = correr(exemplos::LABIRINTO_4X4, o);
          contatos[comRumo] += c.contato ? 1 : 0;
          if (comRumo == 1 && !(c.chegou && c.celulaCerta)) {
            falhas++;
          }
        }
      }
    }
  }
  char msg[160];
  std::snprintf(msg, sizeof(msg), "deriva 0,1 graus/s, 18 largadas: %u com contato sem o rumo pelo corredor e %u com",
                static_cast<unsigned>(contatos[0]), static_cast<unsigned>(contatos[1]));
  registrar(msg);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, falhas, msg);
  TEST_ASSERT_TRUE_MESSAGE(contatos[1] < contatos[0], msg);
  TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(2, contatos[1], msg);
}

// ---- Varredura reduzida (a completa, de 189 corridas, está em Testes de Software)

/// Três robôs (ideal e dois com as rodas desiguais), largadas de −2° a +2° e de 0 a 8 mm fora do
/// centro, no 4×4: todas chegam com a célula certa, e no máximo duas encostam (o robô com 8% de
/// diferença entre as rodas raspa por pouco num giro, na varredura completa também).
void test_varredura_reduzida_de_largadas_e_robos() {
  const double rumos[] = {-2.0, 0.0, 2.0};
  const double laterais[] = {0.0, 8.0};
  uint32_t total = 0;
  uint32_t contatos = 0;
  uint32_t falhas = 0;
  for (int r = 0; r < 3; r++) {
    for (double rumo : rumos) {
      for (double lateral : laterais) {
        OpcoesSimulacao o;
        if (r == 1) {
          o.robo = roboImperfeito();
        } else if (r == 2) {
          o.robo.fatorEsquerda = 1.03;
          o.robo.fatorDireita = 0.95;
        }
        o.desvioRumoGraus = rumo;
        o.desvioLateralMm = lateral;
        const ResultadoCorrida c = correr(exemplos::LABIRINTO_4X4, o);
        total++;
        contatos += c.contato ? 1 : 0;
        falhas += (c.chegou && c.celulaCerta) ? 0 : 1;
      }
    }
  }
  char msg[120];
  std::snprintf(msg, sizeof(msg), "varredura reduzida: %u corridas, %u com contato, %u sem chegar", total, contatos,
                falhas);
  registrar(msg);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, falhas, msg);
  TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(2, contatos, msg);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_posicao_lateral_estimada_segue_a_real_pelas_paredes);
  RUN_TEST(test_com_encoders_a_correcao_segura_o_erro_de_escala);
  RUN_TEST(test_realinhamento_leva_ao_centro_e_o_giro_nao_encosta);
  RUN_TEST(test_realinhamento_num_beco_cabe_no_espaco_curto);
  RUN_TEST(test_corridas_com_robo_imperfeito_nao_encostam_e_a_celula_coincide);
  RUN_TEST(test_rumo_pelo_corredor_segura_a_deriva_do_giroscopio);
  RUN_TEST(test_varredura_reduzida_de_largadas_e_robos);
  return UNITY_END();
}
