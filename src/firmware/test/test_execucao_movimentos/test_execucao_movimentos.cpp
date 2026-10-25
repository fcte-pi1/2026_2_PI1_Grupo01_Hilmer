/// @file test_execucao_movimentos.cpp
/// @brief Testes unitários da execução dos movimentos entre células (#184): os comandos e a
/// conversão de direção em giro, o `ExecutorMovimentos` (avançar uma célula, girar 90° e 180°), o
/// `GatilhoLargada` e o `CicloNavegacao`, com entradas montadas à mão e motores simulados.
///
/// A simulação física completa (robô andando no labirinto) fica em `test_integracao_execucao`.
/// Roda no computador: `pio test -e native -f test_execucao_movimentos`.

#include <unity.h>

#include <cstring>

#include "CicloNavegacao.h"
#include "ConfiguracaoControle.h"
#include "ConfiguracaoExecucao.h"
#include "ExecutorMovimentos.h"
#include "GatilhoLargada.h"
#include "Mapa.h"
#include "MotoresSimulados.h"
#include "Movimento.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr int64_t CELULA_UM = 180000;
constexpr uint16_t LONGE = 1000;     ///< Frente livre (leitura da face do sensor).
constexpr uint16_t PAREDE_LADO = 54;  ///< Parede lateral com o robô centralizado (84 − 30).

LeituraGiro giroValido(int32_t anguloMiligraus = 0) {
  LeituraGiro giro;
  giro.valida = true;
  giro.calibracao = EstadoCalibracao::Calibrado;
  giro.anguloMiligraus = anguloMiligraus;
  return giro;
}

EntradaExecucao entrada(const DistanciasLaterais* leitura, int64_t posicaoUm, LeituraGiro giro = giroValido(),
                        uint32_t dtMs = 10) {
  EntradaExecucao e;
  e.leituraNova = leitura;
  e.giro = giro;
  e.posicaoAvancoUm = posicaoUm;
  e.dtMs = dtMs;
  return e;
}

}  // namespace

// ---- Comandos e conversão da direção em giro

void test_giro_ate_cada_direcao() {
  Movimento giro = Movimento::AvancarCelula;
  TEST_ASSERT_FALSE(giroAte(Direcao::Norte, Direcao::Norte, giro));
  TEST_ASSERT_EQUAL(Movimento::AvancarCelula, giro);
  TEST_ASSERT_TRUE(giroAte(Direcao::Norte, Direcao::Leste, giro));
  TEST_ASSERT_EQUAL(Movimento::Girar90Direita, giro);
  TEST_ASSERT_TRUE(giroAte(Direcao::Norte, Direcao::Oeste, giro));
  TEST_ASSERT_EQUAL(Movimento::Girar90Esquerda, giro);
  TEST_ASSERT_TRUE(giroAte(Direcao::Norte, Direcao::Sul, giro));
  TEST_ASSERT_EQUAL(Movimento::Girar180, giro);
  TEST_ASSERT_TRUE(giroAte(Direcao::Oeste, Direcao::Norte, giro));
  TEST_ASSERT_EQUAL(Movimento::Girar90Direita, giro);
  TEST_ASSERT_TRUE(giroAte(Direcao::Leste, Direcao::Oeste, giro));
  TEST_ASSERT_EQUAL(Movimento::Girar180, giro);
}

void test_direcao_depois_de_cada_movimento() {
  TEST_ASSERT_EQUAL(Direcao::Sul, direcaoDepois(Direcao::Sul, Movimento::AvancarCelula));
  TEST_ASSERT_EQUAL(Direcao::Leste, direcaoDepois(Direcao::Sul, Movimento::Girar90Esquerda));
  TEST_ASSERT_EQUAL(Direcao::Oeste, direcaoDepois(Direcao::Sul, Movimento::Girar90Direita));
  TEST_ASSERT_EQUAL(Direcao::Norte, direcaoDepois(Direcao::Sul, Movimento::Girar180));
  // Ida e volta: o giro que leva a uma direção chega a ela.
  for (uint8_t a = 0; a < 4; a++) {
    for (uint8_t b = 0; b < 4; b++) {
      Movimento giro = Movimento::AvancarCelula;
      giroAte(static_cast<Direcao>(a), static_cast<Direcao>(b), giro);
      TEST_ASSERT_EQUAL(static_cast<Direcao>(b), direcaoDepois(static_cast<Direcao>(a), giro));
    }
  }
}

void test_variacao_de_rumo_segue_o_giroscopio() {
  TEST_ASSERT_EQUAL_INT32(0, variacaoRumoMiligraus(Movimento::AvancarCelula));
  TEST_ASSERT_EQUAL_INT32(90000, variacaoRumoMiligraus(Movimento::Girar90Esquerda));
  TEST_ASSERT_EQUAL_INT32(-90000, variacaoRumoMiligraus(Movimento::Girar90Direita));
  TEST_ASSERT_EQUAL_INT32(180000, variacaoRumoMiligraus(Movimento::Girar180));
  TEST_ASSERT_FALSE(ehGiro(Movimento::AvancarCelula));
  TEST_ASSERT_TRUE(ehGiro(Movimento::Girar180));
}

void test_nomes_para_o_log() {
  TEST_ASSERT_EQUAL_STRING("avancar_celula", nomeDoMovimento(Movimento::AvancarCelula));
  TEST_ASSERT_EQUAL_STRING("girar_90_esquerda", nomeDoMovimento(Movimento::Girar90Esquerda));
  TEST_ASSERT_EQUAL_STRING("girar_90_direita", nomeDoMovimento(Movimento::Girar90Direita));
  TEST_ASSERT_EQUAL_STRING("girar_180", nomeDoMovimento(Movimento::Girar180));
  TEST_ASSERT_EQUAL_STRING("aguardando_largada", nomeDoEstadoCiclo(EstadoCiclo::AguardandoLargada));
  TEST_ASSERT_EQUAL_STRING("percebendo", nomeDoEstadoCiclo(EstadoCiclo::Percebendo));
  TEST_ASSERT_EQUAL_STRING("girando", nomeDoEstadoCiclo(EstadoCiclo::Girando));
  TEST_ASSERT_EQUAL_STRING("avancando", nomeDoEstadoCiclo(EstadoCiclo::Avancando));
  TEST_ASSERT_EQUAL_STRING("objetivo_alcancado", nomeDoEstadoCiclo(EstadoCiclo::ObjetivoAlcancado));
  TEST_ASSERT_EQUAL_STRING("sem_caminho", nomeDoEstadoCiclo(EstadoCiclo::SemCaminho));
  TEST_ASSERT_EQUAL_STRING("falha", nomeDoEstadoCiclo(EstadoCiclo::Falha));
  TEST_ASSERT_EQUAL_STRING("ocioso", nomeDoEstadoExecucao(EstadoExecucao::Ocioso));
  TEST_ASSERT_EQUAL_STRING("executando", nomeDoEstadoExecucao(EstadoExecucao::Executando));
  TEST_ASSERT_EQUAL_STRING("concluido", nomeDoEstadoExecucao(EstadoExecucao::Concluido));
  TEST_ASSERT_EQUAL_STRING("falha", nomeDoEstadoExecucao(EstadoExecucao::Falha));
  TEST_ASSERT_EQUAL_STRING("nenhuma", nomeDaFalhaExecucao(FalhaExecucao::Nenhuma));
  TEST_ASSERT_EQUAL_STRING("parede_na_frente", nomeDaFalhaExecucao(FalhaExecucao::ParedeNaFrente));
  TEST_ASSERT_EQUAL_STRING("sem_leitura_tof", nomeDaFalhaExecucao(FalhaExecucao::SemLeituraTof));
  TEST_ASSERT_EQUAL_STRING("tempo_esgotado", nomeDaFalhaExecucao(FalhaExecucao::TempoEsgotado));
  TEST_ASSERT_EQUAL_STRING("giro_tempo_esgotado", nomeDaFalhaExecucao(FalhaExecucao::GiroTempoEsgotado));
  TEST_ASSERT_EQUAL_STRING("desarmado", nomeDoEstadoGatilho(EstadoGatilho::Desarmado));
  TEST_ASSERT_EQUAL_STRING("armado", nomeDoEstadoGatilho(EstadoGatilho::Armado));
  TEST_ASSERT_EQUAL_STRING("coberto", nomeDoEstadoGatilho(EstadoGatilho::Coberto));
  TEST_ASSERT_EQUAL_STRING("contando", nomeDoEstadoGatilho(EstadoGatilho::Contando));
  TEST_ASSERT_EQUAL_STRING("disparado", nomeDoEstadoGatilho(EstadoGatilho::Disparado));
}

// ---- Executor: avançar uma célula

void test_executor_comeca_ocioso_e_nao_aceita_dois_movimentos() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  TEST_ASSERT_EQUAL(EstadoExecucao::Ocioso, executor.estado());
  TEST_ASSERT_EQUAL(EstadoExecucao::Ocioso, executor.atualizar(entrada(nullptr, 0), motores));
  TEST_ASSERT_EQUAL_UINT32(0, motores.comandos());
  TEST_ASSERT_TRUE(executor.iniciar(Movimento::AvancarCelula));
  TEST_ASSERT_TRUE(executor.ocupado());
  TEST_ASSERT_FALSE(executor.iniciar(Movimento::Girar90Direita));
  TEST_ASSERT_EQUAL(Movimento::AvancarCelula, executor.movimento());
}

void test_avanco_anda_em_cruzeiro_e_para_no_centro_da_proxima_celula() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  const DistanciasLaterais livre{LONGE, PAREDE_LADO, PAREDE_LADO};
  executor.iniciar(Movimento::AvancarCelula);

  executor.atualizar(entrada(&livre, 0), motores);
  TEST_ASSERT_EQUAL_INT32(180, executor.restanteMm());
  TEST_ASSERT_EQUAL_INT16(400, motores.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(400, motores.ultimaPotencia().direita);

  // Na rampa (faltam 30 de 60 mm), a potência fica entre a mínima e o cruzeiro.
  executor.atualizar(entrada(&livre, 150000), motores);
  TEST_ASSERT_EQUAL(EstadoExecucao::Executando, executor.estado());
  TEST_ASSERT_EQUAL_INT16(275, motores.ultimaPotencia().esquerda);

  // A 3 mm do centro, conclui e freia.
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, executor.atualizar(entrada(&livre, 177000), motores));
  TEST_ASSERT_TRUE(motores.freados());
  TEST_ASSERT_EQUAL_INT32(3, executor.restanteMm());
  TEST_ASSERT_FALSE(executor.ocupado());
  TEST_ASSERT_EQUAL(FalhaExecucao::Nenhuma, executor.falha());
}

void test_avanco_mira_o_centro_seguinte_ao_mais_proximo() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  const DistanciasLaterais livre{LONGE, PAREDE_LADO, PAREDE_LADO};

  // Parado 20 mm antes do centro de (1): o destino é o centro de (2), a 200 mm.
  executor.iniciar(Movimento::AvancarCelula);
  executor.atualizar(entrada(&livre, CELULA_UM - 20000), motores);
  TEST_ASSERT_EQUAL_INT32(200, executor.restanteMm());

  // Posição negativa (antes do centro da célula 0): o destino é o centro de (1).
  executor.cancelar(motores);
  executor.iniciar(Movimento::AvancarCelula);
  executor.atualizar(entrada(&livre, -30000), motores);
  TEST_ASSERT_EQUAL_INT32(210, executor.restanteMm());

  // Passou do centro para a metade de trás da célula seguinte: o destino é o centro seguinte.
  executor.cancelar(motores);
  executor.iniciar(Movimento::AvancarCelula);
  executor.atualizar(entrada(&livre, -100000), motores);
  TEST_ASSERT_EQUAL_INT32(100, executor.restanteMm());
}

void test_avanco_que_passa_do_destino_tambem_conclui() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  const DistanciasLaterais livre{LONGE, PAREDE_LADO, PAREDE_LADO};
  executor.iniciar(Movimento::AvancarCelula);
  executor.atualizar(entrada(&livre, 0), motores);
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, executor.atualizar(entrada(nullptr, 195000), motores));
  TEST_ASSERT_EQUAL_INT32(-15, executor.restanteMm());
}

void test_avanco_mantem_o_comando_entre_leituras_e_espera_a_primeira() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  const DistanciasLaterais livre{LONGE, PAREDE_LADO, PAREDE_LADO};
  executor.iniciar(Movimento::AvancarCelula);

  // Sem leitura ainda: não sabe o que há à frente, então fica freado.
  executor.atualizar(entrada(nullptr, 0), motores);
  TEST_ASSERT_TRUE(motores.freados());

  executor.atualizar(entrada(&livre, 0), motores);
  const uint32_t comandos = motores.comandos();
  executor.atualizar(entrada(nullptr, 1000), motores);
  executor.atualizar(entrada(nullptr, 2000), motores);
  TEST_ASSERT_EQUAL_UINT32(comandos + 2, motores.comandos());
  TEST_ASSERT_FALSE(motores.freados());
  TEST_ASSERT_EQUAL_INT16(400, motores.ultimaPotencia().direita);
}

void test_avanco_converte_a_leitura_para_o_centro_do_robo() {
  ExecutorMovimentos executor;
  const DistanciasLaterais face{34, 54, 60};
  const DistanciasLaterais centro = executor.paraOCentro(face);
  TEST_ASSERT_EQUAL_UINT16(84, centro.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(84, centro.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(90, centro.direitaMm);
  const DistanciasLaterais saturada = executor.paraOCentro({UINT16_MAX, 90, 0});
  TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, saturada.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(120, saturada.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(30, saturada.direitaMm);
}

/// Perto do fim de uma parede, o raio lateral pega a parede de uma célula vizinha: acima de 100 mm
/// da face, o lado vai como sem parede para a centralização (#188), e não como parede a 150 mm.
void test_avanco_ignora_parede_lateral_distante_na_centralizacao() {
  ExecutorMovimentos executor;
  TEST_ASSERT_EQUAL_UINT16(130, executor.paraOCentro({LONGE, 100, 54}).esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(LATERAL_SEM_PAREDE_MM, executor.paraOCentro({LONGE, 101, 54}).esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(LATERAL_SEM_PAREDE_MM, executor.paraOCentro({LONGE, 54, 150}).direitaMm);

  // Centrado, com a direita lendo a parede vizinha a 150 mm: só a esquerda conta, e não há correção.
  MotoresSimulados motores;
  const DistanciasLaterais fimDaParede{LONGE, PAREDE_LADO, 150};
  executor.iniciar(Movimento::AvancarCelula);
  executor.atualizar(entrada(&fimDaParede, 0), motores);
  TEST_ASSERT_EQUAL(ModoCentralizacao::SoEsquerda, executor.controleMovimento().centralizacao().modo());
  TEST_ASSERT_EQUAL_INT16(motores.ultimaPotencia().esquerda, motores.ultimaPotencia().direita);
}

void test_avanco_centraliza_no_corredor() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  // Mais perto da parede direita (face a 44 mm, centro a 74): vira para a esquerda.
  const DistanciasLaterais torto{LONGE, 64, 44};
  executor.iniciar(Movimento::AvancarCelula);
  executor.atualizar(entrada(&torto, 0), motores);
  TEST_ASSERT_GREATER_THAN_INT16(motores.ultimaPotencia().esquerda, motores.ultimaPotencia().direita);
}

void test_avanco_corrige_o_rumo_pelo_giroscopio_sem_paredes() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  const DistanciasLaterais aberto{LONGE, LONGE, LONGE};
  executor.iniciar(Movimento::AvancarCelula);
  // Rumo alvo 0 e robô 5° para a esquerda: a roda esquerda acelera para voltar à direita.
  executor.atualizar(entrada(&aberto, 0, giroValido(5000)), motores);
  TEST_ASSERT_GREATER_THAN_INT16(motores.ultimaPotencia().direita, motores.ultimaPotencia().esquerda);
  TEST_ASSERT_TRUE(executor.controleMovimento().temRumoValido());

  // Sem giroscópio válido, segue sem a correção de rumo.
  LeituraGiro semGiro = giroValido(5000);
  semGiro.valida = false;
  executor.atualizar(entrada(&aberto, 1000, semGiro), motores);
  TEST_ASSERT_FALSE(executor.controleMovimento().temRumoValido());
  TEST_ASSERT_EQUAL_INT16(motores.ultimaPotencia().esquerda, motores.ultimaPotencia().direita);
}

void test_parede_na_frente_perto_do_destino_conclui() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  const DistanciasLaterais livre{LONGE, PAREDE_LADO, PAREDE_LADO};
  const DistanciasLaterais parede{34, PAREDE_LADO, PAREDE_LADO};  // centro a 84 mm da parede
  executor.iniciar(Movimento::AvancarCelula);
  executor.atualizar(entrada(&livre, 0), motores);
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, executor.atualizar(entrada(&parede, 160000), motores));
  TEST_ASSERT_TRUE(motores.freados());
}

void test_parede_na_frente_longe_do_destino_falha() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  const DistanciasLaterais parede{30, PAREDE_LADO, PAREDE_LADO};
  executor.iniciar(Movimento::AvancarCelula);
  TEST_ASSERT_EQUAL(EstadoExecucao::Falha, executor.atualizar(entrada(&parede, 0), motores));
  TEST_ASSERT_EQUAL(FalhaExecucao::ParedeNaFrente, executor.falha());
  TEST_ASSERT_TRUE(motores.freados());
}

void test_sem_leitura_do_tof_falha_e_freia() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  const DistanciasLaterais livre{LONGE, PAREDE_LADO, PAREDE_LADO};
  executor.iniciar(Movimento::AvancarCelula);
  executor.atualizar(entrada(&livre, 0), motores);
  EstadoExecucao estado = EstadoExecucao::Executando;
  uint32_t ciclos = 0;
  while (estado == EstadoExecucao::Executando && ciclos < 100) {
    estado = executor.atualizar(entrada(nullptr, 1000), motores);
    ciclos++;
  }
  TEST_ASSERT_EQUAL(EstadoExecucao::Falha, estado);
  TEST_ASSERT_EQUAL(FalhaExecucao::SemLeituraTof, executor.falha());
  TEST_ASSERT_EQUAL_UINT32(50, ciclos);  // 500 ms a 10 ms por ciclo
  TEST_ASSERT_TRUE(motores.freados());
}

void test_avanco_que_nao_chega_esgota_o_tempo() {
  ConfiguracaoExecucao config;
  config.tempoMaximoAvancoMs = 200;
  ExecutorMovimentos executor(config);
  MotoresSimulados motores;
  const DistanciasLaterais livre{LONGE, PAREDE_LADO, PAREDE_LADO};
  executor.iniciar(Movimento::AvancarCelula);
  EstadoExecucao estado = EstadoExecucao::Executando;
  for (int i = 0; i < 30 && estado == EstadoExecucao::Executando; i++) {
    estado = executor.atualizar(entrada(&livre, 0), motores);  // robô preso: a posição não muda
  }
  TEST_ASSERT_EQUAL(EstadoExecucao::Falha, estado);
  TEST_ASSERT_EQUAL(FalhaExecucao::TempoEsgotado, executor.falha());
  TEST_ASSERT_EQUAL_UINT32(200, executor.tempoMs());
}

void test_cancelar_freia_e_volta_a_ocioso() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  const DistanciasLaterais livre{LONGE, PAREDE_LADO, PAREDE_LADO};
  executor.cancelar(motores);  // sem movimento: nada acontece
  TEST_ASSERT_EQUAL_UINT32(0, motores.comandos());
  executor.iniciar(Movimento::AvancarCelula);
  executor.atualizar(entrada(&livre, 0), motores);
  executor.cancelar(motores);
  TEST_ASSERT_EQUAL(EstadoExecucao::Ocioso, executor.estado());
  TEST_ASSERT_TRUE(motores.freados());
  const uint32_t comandos = motores.comandos();
  executor.atualizar(entrada(&livre, 1000), motores);
  TEST_ASSERT_EQUAL_UINT32(comandos, motores.comandos());
}

// ---- Executor: giros

/// Gira simulando um robô ideal: o ângulo anda 1° por ciclo no sentido da potência.
EstadoExecucao girarAteTerminar(ExecutorMovimentos& executor, MotoresSimulados& motores, int32_t& angulo,
                                uint32_t maxCiclos = 1000) {
  EstadoExecucao estado = executor.estado();
  for (uint32_t i = 0; i < maxCiclos && executor.ocupado(); i++) {
    estado = executor.atualizar(entrada(nullptr, 0, giroValido(angulo)), motores);
    const PotenciaRodas p = motores.ultimaPotencia();
    if (p.direita > p.esquerda) {
      angulo += p.direita > 200 ? 1000 : 250;
    } else if (p.esquerda > p.direita) {
      angulo -= p.esquerda > 200 ? 1000 : 250;
    }
  }
  return estado;
}

void test_giro_90_esquerda_vai_ao_rumo_alvo() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  int32_t angulo = 0;
  executor.iniciar(Movimento::Girar90Esquerda);
  TEST_ASSERT_EQUAL_INT32(90000, executor.rumoAlvoMiligraus());
  executor.atualizar(entrada(nullptr, 0, giroValido(angulo)), motores);
  // Para a esquerda: a roda esquerda recua e a direita avança.
  TEST_ASSERT_LESS_THAN_INT16(0, motores.ultimaPotencia().esquerda);
  TEST_ASSERT_GREATER_THAN_INT16(0, motores.ultimaPotencia().direita);
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, girarAteTerminar(executor, motores, angulo));
  TEST_ASSERT_INT32_WITHIN(2000, 90000, angulo);
  TEST_ASSERT_TRUE(motores.freados());
}

void test_giro_90_direita_e_180() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  int32_t angulo = 0;
  executor.iniciar(Movimento::Girar90Direita);
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, girarAteTerminar(executor, motores, angulo));
  TEST_ASSERT_INT32_WITHIN(2000, -90000, angulo);

  executor.iniciar(Movimento::Girar180);
  TEST_ASSERT_EQUAL_INT32(90000, executor.rumoAlvoMiligraus());
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, girarAteTerminar(executor, motores, angulo));
  TEST_ASSERT_INT32_WITHIN(2000, 90000, angulo);
}

/// O giro mira o rumo absoluto: um erro de 3° deixado pelo giro anterior não passa para o próximo.
void test_giro_mira_o_rumo_absoluto_e_nao_acumula_erro() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  executor.iniciar(Movimento::Girar90Esquerda);
  executor.atualizar(entrada(nullptr, 0, giroValido(3000)), motores);
  TEST_ASSERT_EQUAL_INT32(90000, executor.controleGiro().anguloAlvoMiligraus());
  TEST_ASSERT_EQUAL_INT32(87000, executor.controleGiro().erroMiligraus());
}

void test_definir_rumo_alvo() {
  ExecutorMovimentos executor;
  executor.definirRumoAlvo(-180000);
  TEST_ASSERT_EQUAL_INT32(-180000, executor.rumoAlvoMiligraus());
  executor.iniciar(Movimento::Girar90Direita);
  TEST_ASSERT_EQUAL_INT32(-270000, executor.rumoAlvoMiligraus());
}

void test_giro_sem_giroscopio_espera_parado_e_desiste_no_tempo_limite() {
  ConfiguracaoGiro configGiro;
  configGiro.tempoMaximoGiroMs = 100;
  ExecutorMovimentos executor(ConfiguracaoExecucao{}, ConfiguracaoControle{}, configGiro);
  MotoresSimulados motores;
  LeituraGiro semGiro;
  executor.iniciar(Movimento::Girar90Direita);
  EstadoExecucao estado = EstadoExecucao::Executando;
  uint32_t ciclos = 0;
  while (estado == EstadoExecucao::Executando && ciclos < 100) {
    estado = executor.atualizar(entrada(nullptr, 0, semGiro), motores);
    TEST_ASSERT_TRUE(motores.freados());
    ciclos++;
  }
  TEST_ASSERT_EQUAL(EstadoExecucao::Falha, estado);
  TEST_ASSERT_EQUAL(FalhaExecucao::GiroTempoEsgotado, executor.falha());
  TEST_ASSERT_EQUAL_UINT32(10, ciclos);
}

void test_giro_preso_esgota_o_tempo() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  executor.iniciar(Movimento::Girar180);
  EstadoExecucao estado = EstadoExecucao::Executando;
  for (int i = 0; i < 1000 && estado == EstadoExecucao::Executando; i++) {
    estado = executor.atualizar(entrada(nullptr, 0, giroValido(0)), motores);  // o ângulo não muda
  }
  TEST_ASSERT_EQUAL(EstadoExecucao::Falha, estado);
  TEST_ASSERT_EQUAL(FalhaExecucao::GiroTempoEsgotado, executor.falha());
  TEST_ASSERT_TRUE(motores.freados());
}

void test_giro_que_perde_o_giroscopio_no_meio_freia() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  executor.iniciar(Movimento::Girar90Esquerda);
  executor.atualizar(entrada(nullptr, 0, giroValido(0)), motores);
  TEST_ASSERT_FALSE(motores.freados());
  LeituraGiro semGiro;
  executor.atualizar(entrada(nullptr, 0, semGiro), motores);
  TEST_ASSERT_TRUE(motores.freados());
  TEST_ASSERT_EQUAL(EstadoExecucao::Executando, executor.estado());
}

void test_cancelar_um_giro() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  executor.iniciar(Movimento::Girar90Esquerda);
  executor.atualizar(entrada(nullptr, 0, giroValido(0)), motores);
  executor.cancelar(motores);
  TEST_ASSERT_EQUAL(EstadoExecucao::Ocioso, executor.estado());
  TEST_ASSERT_EQUAL(EstadoGiro::Inativo, executor.controleGiro().estado());
  TEST_ASSERT_TRUE(motores.freados());
}

// ---- Gatilho da largada

/// Passa `ms` com a mesma frente, uma leitura nova a cada 10 ms.
bool manter(GatilhoLargada& gatilho, uint16_t frenteMm, uint32_t ms) {
  const DistanciasLaterais leitura{frenteMm, PAREDE_LADO, PAREDE_LADO};
  bool disparou = false;
  for (uint32_t t = 0; t < ms; t += 10) {
    disparou = gatilho.atualizar(&leitura, 10);
  }
  return disparou;
}

void test_gesto_completo_dispara_depois_da_espera() {
  GatilhoLargada gatilho;
  TEST_ASSERT_EQUAL(EstadoGatilho::Desarmado, gatilho.estado());
  TEST_ASSERT_FALSE(manter(gatilho, 600, 100));
  TEST_ASSERT_EQUAL(EstadoGatilho::Armado, gatilho.estado());
  TEST_ASSERT_FALSE(manter(gatilho, 30, 400));
  TEST_ASSERT_EQUAL(EstadoGatilho::Coberto, gatilho.estado());
  TEST_ASSERT_FALSE(manter(gatilho, 600, 900));
  TEST_ASSERT_EQUAL(EstadoGatilho::Contando, gatilho.estado());
  TEST_ASSERT_TRUE(manter(gatilho, 600, 200));
  TEST_ASSERT_TRUE(gatilho.disparado());
  // Disparado fica disparado, mesmo com a mão de volta.
  TEST_ASSERT_TRUE(manter(gatilho, 30, 100));
}

void test_vulto_rapido_nao_conta() {
  GatilhoLargada gatilho;
  manter(gatilho, 600, 50);
  manter(gatilho, 30, 200);  // menos que os 300 ms
  manter(gatilho, 600, 2000);
  TEST_ASSERT_EQUAL(EstadoGatilho::Armado, gatilho.estado());
  TEST_ASSERT_FALSE(gatilho.disparado());
}

void test_parede_parada_na_frente_nunca_arma() {
  GatilhoLargada gatilho;
  TEST_ASSERT_FALSE(manter(gatilho, 34, 5000));
  TEST_ASSERT_EQUAL(EstadoGatilho::Desarmado, gatilho.estado());
  gatilho.dispararManual();
  TEST_ASSERT_TRUE(gatilho.disparado());
  TEST_ASSERT_TRUE(gatilho.atualizar(nullptr, 10));
}

void test_mao_de_volta_durante_a_espera_recomeca() {
  GatilhoLargada gatilho;
  manter(gatilho, 600, 50);
  manter(gatilho, 30, 400);
  manter(gatilho, 600, 500);
  TEST_ASSERT_EQUAL(EstadoGatilho::Contando, gatilho.estado());
  manter(gatilho, 30, 100);
  TEST_ASSERT_EQUAL(EstadoGatilho::Coberto, gatilho.estado());
  // A contagem do tempo coberto recomeçou: 100 ms não bastam.
  manter(gatilho, 600, 50);
  TEST_ASSERT_EQUAL(EstadoGatilho::Armado, gatilho.estado());
}

void test_mao_saindo_entre_os_limiares_espera_a_frente_livre() {
  GatilhoLargada gatilho;
  manter(gatilho, 600, 50);
  manter(gatilho, 30, 400);
  manter(gatilho, 70, 500);  // entre 50 e 100 mm: ainda saindo
  TEST_ASSERT_EQUAL(EstadoGatilho::Coberto, gatilho.estado());
  manter(gatilho, 600, 10);
  TEST_ASSERT_EQUAL(EstadoGatilho::Contando, gatilho.estado());
}

void test_sem_leitura_vale_a_ultima_frente() {
  GatilhoLargada gatilho;
  TEST_ASSERT_FALSE(gatilho.atualizar(nullptr, 10));  // nunca viu a frente
  TEST_ASSERT_EQUAL(EstadoGatilho::Desarmado, gatilho.estado());
  manter(gatilho, 600, 10);
  manter(gatilho, 30, 10);
  for (int i = 0; i < 40; i++) {
    gatilho.atualizar(nullptr, 10);  // a mão continua ali: o tempo coberto conta
  }
  manter(gatilho, 600, 10);
  TEST_ASSERT_EQUAL(EstadoGatilho::Contando, gatilho.estado());
}

void test_reiniciar_o_gatilho() {
  GatilhoLargada gatilho;
  gatilho.dispararManual();
  gatilho.reiniciar();
  TEST_ASSERT_EQUAL(EstadoGatilho::Desarmado, gatilho.estado());
  TEST_ASSERT_FALSE(gatilho.atualizar(nullptr, 10));
}

// ---- Ciclo de navegação

namespace {

/// Monta o ciclo num 4×4 vazio (só o perímetro), na partida, virado para o Norte.
struct CicloDeTeste {
  Mapa mapa{DimensaoMapa::Labirinto4x4};
  MotoresSimulados motores;
  CicloNavegacao ciclo{mapa};

  CicloDeTeste() { ciclo.reiniciar(Pose{{0, 0}, Direcao::Norte}, motores); }

  EntradaCiclo entradaBase(const DistanciasLaterais* leitura, int64_t posicaoUm = 0,
                           Pose pose = Pose{{0, 0}, Direcao::Norte}) const {
    EntradaCiclo e;
    e.liberado = true;
    e.pose = pose;
    e.poseValida = true;
    e.posicaoAvancoUm = posicaoUm;
    e.leituraNova = leitura;
    e.giro = giroValido();
    e.dtMs = 10;
    return e;
  }

  /// Passa pela percepção com a mesma leitura (largada + 3 amostras).
  EstadoCiclo perceber(const DistanciasLaterais& leitura, Pose pose = Pose{{0, 0}, Direcao::Norte},
                       int64_t posicaoUm = 0) {
    EstadoCiclo estado = ciclo.estado();
    for (int i = 0; i < 4; i++) {
      estado = ciclo.atualizar(entradaBase(&leitura, posicaoUm, pose), motores);
    }
    return estado;
  }
};

}  // namespace

void test_ciclo_espera_a_largada_parado() {
  CicloDeTeste t;
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, t.ciclo.estado());
  TEST_ASSERT_TRUE(t.motores.freados());
  const DistanciasLaterais livre{LONGE, PAREDE_LADO, PAREDE_LADO};
  EntradaCiclo e = t.entradaBase(&livre);
  e.liberado = false;
  const uint32_t comandos = t.motores.comandos();
  for (int i = 0; i < 10; i++) {
    t.ciclo.atualizar(e, t.motores);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, t.ciclo.estado());
  TEST_ASSERT_EQUAL_UINT32(comandos, t.motores.comandos());

  // Liberado, mas sem giroscópio: continua esperando.
  e.liberado = true;
  e.giro.valida = false;
  t.ciclo.atualizar(e, t.motores);
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, t.ciclo.estado());
}

void test_ciclo_percebe_grava_as_paredes_e_decide_avancar() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.atualizar(t.entradaBase(nullptr), t.motores));
  // Ciclos sem leitura nova não contam como amostra.
  t.ciclo.atualizar(t.entradaBase(nullptr), t.motores);
  t.ciclo.atualizar(t.entradaBase(&corredor), t.motores);
  t.ciclo.atualizar(t.entradaBase(&corredor), t.motores);
  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.estado());
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, t.ciclo.atualizar(t.entradaBase(&corredor), t.motores));

  TEST_ASSERT_EQUAL(EstadoParede::Parede, t.mapa.obterParede({0, 0}, Direcao::Oeste));
  TEST_ASSERT_EQUAL(EstadoParede::Parede, t.mapa.obterParede({0, 0}, Direcao::Leste));
  TEST_ASSERT_EQUAL(EstadoParede::Livre, t.mapa.obterParede({0, 0}, Direcao::Norte));
  TEST_ASSERT_TRUE(t.ciclo.navegacao().passoPendente());
  TEST_ASSERT_EQUAL_UINT8(1, t.ciclo.navegacao().obterCelulaEsperada().linha);
}

void test_ciclo_avanca_confirma_a_pose_e_percebe_de_novo() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.perceber(corredor);
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, t.ciclo.estado());
  t.ciclo.atualizar(t.entradaBase(&corredor, 0), t.motores);
  TEST_ASSERT_FALSE(t.motores.freados());
  TEST_ASSERT_TRUE(t.ciclo.direcaoDefinida());
  t.ciclo.atualizar(t.entradaBase(&corredor, 90000), t.motores);
  const EstadoCiclo estado =
      t.ciclo.atualizar(t.entradaBase(&corredor, 178000, Pose{{1, 0}, Direcao::Norte}), t.motores);
  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, estado);
  TEST_ASSERT_TRUE(t.motores.freados());
  TEST_ASSERT_EQUAL(ResultadoConfirmacao::Confirmada, t.ciclo.ultimaConfirmacao());
  TEST_ASSERT_EQUAL_UINT8(1, t.ciclo.navegacao().obterPosicao().linha);
  TEST_ASSERT_EQUAL_UINT32(1, t.ciclo.celulasAvancadas());
  TEST_ASSERT_FALSE(t.ciclo.navegacao().passoPendente());
}

void test_ciclo_gira_antes_de_avancar_e_adota_a_direcao_do_giro() {
  CicloDeTeste t;
  // Parede à frente e à esquerda, livre à direita: o Flood Fill manda para o Leste.
  const DistanciasLaterais leitura{34, PAREDE_LADO, LONGE};
  t.perceber(leitura);
  TEST_ASSERT_EQUAL(EstadoCiclo::Girando, t.ciclo.estado());
  TEST_ASSERT_FALSE(t.ciclo.direcaoDefinida());
  TEST_ASSERT_EQUAL(Movimento::Girar90Direita, t.ciclo.executor().movimento());
  TEST_ASSERT_EQUAL_INT32(-90000, t.ciclo.executor().rumoAlvoMiligraus());

  // O giroscópio chega a −90° e fica: o giro conclui e o avanço começa no ciclo seguinte.
  EntradaCiclo e = t.entradaBase(nullptr);
  e.giro = giroValido(-90000);
  for (int i = 0; i < 5 && t.ciclo.estado() == EstadoCiclo::Girando; i++) {
    t.ciclo.atualizar(e, t.motores);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, t.ciclo.estado());
  TEST_ASSERT_EQUAL(Direcao::Leste, t.ciclo.direcao());
  TEST_ASSERT_TRUE(t.ciclo.direcaoDefinida());
  TEST_ASSERT_EQUAL_UINT32(1, t.ciclo.girosFeitos());
  TEST_ASSERT_EQUAL(Movimento::AvancarCelula, t.ciclo.executor().movimento());
}

void test_ciclo_parede_nova_no_caminho_replaneja() {
  CicloDeTeste t;
  // Frente e direita livres: o Flood Fill prefere seguir em frente.
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, LONGE};
  t.perceber(corredor);
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, t.ciclo.estado());
  // Uma parede que a percepção classificou como livre aparece logo à frente.
  const DistanciasLaterais parede{30, PAREDE_LADO, LONGE};
  t.ciclo.atualizar(t.entradaBase(&corredor, 0), t.motores);
  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.atualizar(t.entradaBase(&parede, 1000), t.motores));
  TEST_ASSERT_TRUE(t.motores.freados());
  TEST_ASSERT_FALSE(t.ciclo.navegacao().passoPendente());
  TEST_ASSERT_EQUAL_UINT32(0, t.ciclo.celulasAvancadas());

  // O mapa ficou com a parede, apesar do conflito com a percepção anterior, e o próximo
  // planejamento a contorna pela direita.
  TEST_ASSERT_EQUAL(EstadoParede::Parede, t.mapa.obterParede({0, 0}, Direcao::Norte));
  t.perceber(parede);
  TEST_ASSERT_EQUAL(EstadoCiclo::Girando, t.ciclo.estado());
  TEST_ASSERT_EQUAL(Movimento::Girar90Direita, t.ciclo.executor().movimento());
}

void test_ciclo_sem_caminho_para() {
  CicloDeTeste t;
  // Fechado nos três lados (e o Sul é o perímetro): não há caminho.
  const DistanciasLaterais fechado{34, PAREDE_LADO, PAREDE_LADO};
  TEST_ASSERT_EQUAL(EstadoCiclo::SemCaminho, t.perceber(fechado));
  TEST_ASSERT_TRUE(t.motores.freados());
  // Parado de vez: nem a leitura nova muda o estado.
  TEST_ASSERT_EQUAL(EstadoCiclo::SemCaminho, t.ciclo.atualizar(t.entradaBase(&fechado), t.motores));
}

void test_ciclo_no_objetivo_para() {
  CicloDeTeste t;
  t.ciclo.reiniciar(Pose{{3, 3}, Direcao::Norte}, t.motores);
  const DistanciasLaterais leitura{34, PAREDE_LADO, PAREDE_LADO};
  TEST_ASSERT_EQUAL(EstadoCiclo::ObjetivoAlcancado, t.perceber(leitura, Pose{{3, 3}, Direcao::Norte}));
  TEST_ASSERT_TRUE(t.motores.freados());
}

void test_ciclo_falha_do_movimento_para_tudo() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.perceber(corredor);
  t.ciclo.atualizar(t.entradaBase(&corredor, 0), t.motores);
  for (int i = 0; i < 60; i++) {
    t.ciclo.atualizar(t.entradaBase(nullptr, 1000), t.motores);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Falha, t.ciclo.estado());
  TEST_ASSERT_EQUAL(FalhaExecucao::SemLeituraTof, t.ciclo.falhaExecucao());
  TEST_ASSERT_TRUE(t.motores.freados());
  TEST_ASSERT_FALSE(t.ciclo.navegacao().passoPendente());

  // Só o reinício tira do estado de falha.
  t.ciclo.reiniciar(Pose{{0, 0}, Direcao::Norte}, t.motores);
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, t.ciclo.estado());
  TEST_ASSERT_EQUAL(FalhaExecucao::Nenhuma, t.ciclo.falhaExecucao());
}

void test_ciclo_falha_do_giro() {
  ConfiguracaoGiro configGiro;
  configGiro.tempoMaximoGiroMs = 50;
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  MotoresSimulados motores;
  CicloNavegacao ciclo(mapa, ConfiguracaoExecucao{}, ConfiguracaoControle{}, configGiro);
  ciclo.reiniciar(Pose{{0, 0}, Direcao::Norte}, motores);
  const DistanciasLaterais leitura{34, PAREDE_LADO, LONGE};
  EntradaCiclo e;
  e.liberado = true;
  e.leituraNova = &leitura;
  e.giro = giroValido();
  e.dtMs = 10;
  for (int i = 0; i < 4; i++) {
    ciclo.atualizar(e, motores);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Girando, ciclo.estado());
  e.leituraNova = nullptr;
  for (int i = 0; i < 10; i++) {
    ciclo.atualizar(e, motores);  // o ângulo não sai do lugar
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Falha, ciclo.estado());
  TEST_ASSERT_EQUAL(FalhaExecucao::GiroTempoEsgotado, ciclo.falhaExecucao());
}

void test_ciclo_pose_fora_do_mapa_falha() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.perceber(corredor);
  t.ciclo.atualizar(t.entradaBase(&corredor, 0), t.motores);
  EntradaCiclo e = t.entradaBase(&corredor, 178000, Pose{{1, 0}, Direcao::Norte});
  e.poseValida = false;
  TEST_ASSERT_EQUAL(EstadoCiclo::Falha, t.ciclo.atualizar(e, t.motores));
  TEST_ASSERT_EQUAL(FalhaExecucao::Nenhuma, t.ciclo.falhaExecucao());
  TEST_ASSERT_TRUE(t.motores.freados());

  // Uma pose medida fora do mapa também para.
  CicloDeTeste u;
  u.perceber(corredor);
  u.ciclo.atualizar(u.entradaBase(&corredor, 0), u.motores);
  TEST_ASSERT_EQUAL(EstadoCiclo::Falha,
                    u.ciclo.atualizar(u.entradaBase(&corredor, 178000, Pose{{9, 0}, Direcao::Norte}), u.motores));
  TEST_ASSERT_EQUAL(ResultadoConfirmacao::Invalida, u.ciclo.ultimaConfirmacao());
}

void test_ciclo_sem_permissao_no_meio_freia_e_retoma_percebendo() {
  CicloDeTeste t;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.perceber(corredor);
  t.ciclo.atualizar(t.entradaBase(&corredor, 0), t.motores);
  TEST_ASSERT_FALSE(t.motores.freados());

  EntradaCiclo e = t.entradaBase(&corredor, 50000);
  e.liberado = false;
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, t.ciclo.atualizar(e, t.motores));
  TEST_ASSERT_TRUE(t.motores.freados());
  TEST_ASSERT_FALSE(t.ciclo.executor().ocupado());
  TEST_ASSERT_FALSE(t.ciclo.navegacao().passoPendente());

  // De volta: o rumo é o da direção atual, mais próximo do ângulo de agora (uma volta inteira).
  e.liberado = true;
  e.giro = giroValido(361000);
  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.atualizar(e, t.motores));
  TEST_ASSERT_EQUAL_INT32(360000, t.ciclo.executor().rumoAlvoMiligraus());
}

/// A largada guarda o ângulo de partida: o giroscópio que derivou enquanto esperava não vira erro
/// de rumo, e uma partida virada para o Leste também funciona.
void test_ciclo_rumo_de_referencia_e_o_da_largada() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  MotoresSimulados motores;
  CicloNavegacao ciclo(mapa);
  ciclo.reiniciar(Pose{{0, 0}, Direcao::Leste}, motores);
  EntradaCiclo e;
  e.liberado = true;
  e.pose = Pose{{0, 0}, Direcao::Leste};
  e.giro = giroValido(1500);
  e.dtMs = 10;
  ciclo.atualizar(e, motores);
  TEST_ASSERT_EQUAL_INT32(1500, ciclo.executor().rumoAlvoMiligraus());
  // Frente (Leste) livre, esquerda (Norte) livre: o Flood Fill prefere seguir em frente.
  const DistanciasLaterais leitura{LONGE, LONGE, PAREDE_LADO};
  e.leituraNova = &leitura;
  for (int i = 0; i < 3; i++) {
    ciclo.atualizar(e, motores);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, ciclo.estado());
  TEST_ASSERT_EQUAL(Direcao::Leste, ciclo.direcao());
}

void test_ciclo_quantidade_de_amostras_limitada() {
  ConfiguracaoExecucao config;
  config.amostrasPercepcao = 0;
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  MotoresSimulados motores;
  CicloNavegacao umaAmostra(mapa, config);
  umaAmostra.reiniciar(Pose{{0, 0}, Direcao::Norte}, motores);
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  EntradaCiclo e;
  e.liberado = true;
  e.giro = giroValido();
  e.dtMs = 10;
  umaAmostra.atualizar(e, motores);
  e.leituraNova = &corredor;
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, umaAmostra.atualizar(e, motores));

  config.amostrasPercepcao = 200;
  Mapa outro(DimensaoMapa::Labirinto4x4);
  CicloNavegacao muitas(outro, config);
  muitas.reiniciar(Pose{{0, 0}, Direcao::Norte}, motores);
  e.leituraNova = nullptr;
  muitas.atualizar(e, motores);
  e.leituraNova = &corredor;
  int ciclos = 0;
  while (muitas.estado() == EstadoCiclo::Percebendo && ciclos < 100) {
    muitas.atualizar(e, motores);
    ciclos++;
  }
  TEST_ASSERT_EQUAL_INT(16, ciclos);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_giro_ate_cada_direcao);
  RUN_TEST(test_direcao_depois_de_cada_movimento);
  RUN_TEST(test_variacao_de_rumo_segue_o_giroscopio);
  RUN_TEST(test_nomes_para_o_log);
  RUN_TEST(test_executor_comeca_ocioso_e_nao_aceita_dois_movimentos);
  RUN_TEST(test_avanco_anda_em_cruzeiro_e_para_no_centro_da_proxima_celula);
  RUN_TEST(test_avanco_mira_o_centro_seguinte_ao_mais_proximo);
  RUN_TEST(test_avanco_que_passa_do_destino_tambem_conclui);
  RUN_TEST(test_avanco_mantem_o_comando_entre_leituras_e_espera_a_primeira);
  RUN_TEST(test_avanco_converte_a_leitura_para_o_centro_do_robo);
  RUN_TEST(test_avanco_ignora_parede_lateral_distante_na_centralizacao);
  RUN_TEST(test_avanco_centraliza_no_corredor);
  RUN_TEST(test_avanco_corrige_o_rumo_pelo_giroscopio_sem_paredes);
  RUN_TEST(test_parede_na_frente_perto_do_destino_conclui);
  RUN_TEST(test_parede_na_frente_longe_do_destino_falha);
  RUN_TEST(test_sem_leitura_do_tof_falha_e_freia);
  RUN_TEST(test_avanco_que_nao_chega_esgota_o_tempo);
  RUN_TEST(test_cancelar_freia_e_volta_a_ocioso);
  RUN_TEST(test_giro_90_esquerda_vai_ao_rumo_alvo);
  RUN_TEST(test_giro_90_direita_e_180);
  RUN_TEST(test_giro_mira_o_rumo_absoluto_e_nao_acumula_erro);
  RUN_TEST(test_definir_rumo_alvo);
  RUN_TEST(test_giro_sem_giroscopio_espera_parado_e_desiste_no_tempo_limite);
  RUN_TEST(test_giro_preso_esgota_o_tempo);
  RUN_TEST(test_giro_que_perde_o_giroscopio_no_meio_freia);
  RUN_TEST(test_cancelar_um_giro);
  RUN_TEST(test_gesto_completo_dispara_depois_da_espera);
  RUN_TEST(test_vulto_rapido_nao_conta);
  RUN_TEST(test_parede_parada_na_frente_nunca_arma);
  RUN_TEST(test_mao_de_volta_durante_a_espera_recomeca);
  RUN_TEST(test_mao_saindo_entre_os_limiares_espera_a_frente_livre);
  RUN_TEST(test_sem_leitura_vale_a_ultima_frente);
  RUN_TEST(test_reiniciar_o_gatilho);
  RUN_TEST(test_ciclo_espera_a_largada_parado);
  RUN_TEST(test_ciclo_percebe_grava_as_paredes_e_decide_avancar);
  RUN_TEST(test_ciclo_avanca_confirma_a_pose_e_percebe_de_novo);
  RUN_TEST(test_ciclo_gira_antes_de_avancar_e_adota_a_direcao_do_giro);
  RUN_TEST(test_ciclo_parede_nova_no_caminho_replaneja);
  RUN_TEST(test_ciclo_sem_caminho_para);
  RUN_TEST(test_ciclo_no_objetivo_para);
  RUN_TEST(test_ciclo_falha_do_movimento_para_tudo);
  RUN_TEST(test_ciclo_falha_do_giro);
  RUN_TEST(test_ciclo_pose_fora_do_mapa_falha);
  RUN_TEST(test_ciclo_sem_permissao_no_meio_freia_e_retoma_percebendo);
  RUN_TEST(test_ciclo_rumo_de_referencia_e_o_da_largada);
  RUN_TEST(test_ciclo_quantidade_de_amostras_limitada);
  return UNITY_END();
}
