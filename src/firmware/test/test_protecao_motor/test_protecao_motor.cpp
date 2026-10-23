/// @file test_protecao_motor.cpp
/// @brief Testes da proteção contra motor travado sem encoder (#341): a regra de tempo em duty
/// alto sem movimento do ToF frontal nem do giroscópio, e a integração com o `ControleMovimento`,
/// o `LidarSimulado` e os `MotoresSimulados`.
/// Roda no computador: `pio test -e native -f test_protecao_motor`.

#include <unity.h>

#include <cstdint>

#include "ConfiguracaoControle.h"
#include "ConfiguracaoProtecaoMotor.h"
#include "ControleMovimento.h"
#include "IMotores.h"
#include "LidarSimulado.h"
#include "MotoresSimulados.h"
#include "ProtecaoMotorTravado.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint32_t DT_MS = 100;                      ///< Ritmo das leituras do sensor de distância.
constexpr PotenciaRodas CRUZEIRO = {400, 400};       ///< Duty alto.
constexpr PotenciaRodas FRACA = {200, 200};          ///< Abaixo de `POTENCIA_ALTA_MINIMA`.

MedidasProtecao parado(uint16_t frenteMm = 500) { return MedidasProtecao{true, frenteMm, 0}; }

/// Roda a proteção por `ciclos` ciclos de `DT_MS`, com as mesmas medidas, e devolve o estado final.
EstadoProtecao rodar(ProtecaoMotorTravado& protecao, const PotenciaRodas& potencia,
                     const MedidasProtecao& medidas, int ciclos) {
  EstadoProtecao estado = protecao.estado();
  for (int i = 0; i < ciclos; ++i) {
    estado = protecao.atualizar(potencia, medidas, DT_MS);
  }
  return estado;
}

}  // namespace

// ---------------------------------------------------------------- regra de travamento

/// Duty alto por 500 ms, ToF frontal parado e giroscópio parado: travado.
void test_duty_alto_sem_movimento_trava_no_tempo_limite() {
  ProtecaoMotorTravado protecao;
  TEST_ASSERT_EQUAL(EstadoProtecao::Monitorando, rodar(protecao, CRUZEIRO, parado(), 4));
  TEST_ASSERT_FALSE(protecao.travado());
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, rodar(protecao, CRUZEIRO, parado(), 1));
  TEST_ASSERT_TRUE(protecao.travado());
  TEST_ASSERT_EQUAL_UINT32(500, protecao.tempoMonitoradoMs());
}

/// Um ciclo antes do tempo limite ainda não está travado.
void test_nao_trava_antes_do_tempo_limite() {
  ProtecaoMotorTravado protecao;
  TEST_ASSERT_EQUAL(EstadoProtecao::Monitorando, protecao.atualizar(CRUZEIRO, parado(), 499));
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, protecao.atualizar(CRUZEIRO, parado(), 1));
}

/// Potência abaixo do duty alto nunca conta: o motor pode estar só na zona morta.
void test_potencia_baixa_nunca_trava() {
  ProtecaoMotorTravado protecao;
  TEST_ASSERT_EQUAL(EstadoProtecao::Normal, rodar(protecao, FRACA, parado(), 50));
  TEST_ASSERT_EQUAL_UINT32(0, protecao.tempoMonitoradoMs());
}

/// A roda com mais potência decide: uma roda parada e a outra em duty alto também é monitorada.
void test_basta_uma_roda_em_duty_alto() {
  ProtecaoMotorTravado protecao;
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, rodar(protecao, PotenciaRodas{0, 350}, parado(), 5));
}

/// A ré (potência negativa) também conta como duty alto.
void test_re_em_duty_alto_tambem_trava() {
  ProtecaoMotorTravado protecao;
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, rodar(protecao, PotenciaRodas{-400, -400}, parado(), 5));
}

/// A potência exatamente no limite conta como duty alto; um a menos, não.
void test_limite_da_potencia_alta() {
  ProtecaoMotorTravado no_limite;
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, rodar(no_limite, PotenciaRodas{POTENCIA_ALTA_MINIMA, 0}, parado(), 5));
  ProtecaoMotorTravado abaixo;
  TEST_ASSERT_EQUAL(EstadoProtecao::Normal,
                    rodar(abaixo, PotenciaRodas{POTENCIA_ALTA_MINIMA - 1, 0}, parado(), 5));
}

// ---------------------------------------------------------------- sinais de movimento

/// Andando em linha reta, a leitura frontal diminui: não trava.
void test_aproximacao_da_parede_nao_trava() {
  ProtecaoMotorTravado protecao;
  uint16_t frente = 800;
  for (int i = 0; i < 20; ++i) {
    TEST_ASSERT_NOT_EQUAL(EstadoProtecao::Travado,
                          protecao.atualizar(CRUZEIRO, MedidasProtecao{true, frente, 0}, DT_MS));
    frente -= 10;  // 10 mm por ciclo: bem acima do ruído
  }
}

/// Andando de ré, a leitura frontal aumenta: também é movimento, não trava.
void test_afastamento_da_parede_nao_trava() {
  ProtecaoMotorTravado protecao;
  uint16_t frente = 200;
  for (int i = 0; i < 20; ++i) {
    TEST_ASSERT_NOT_EQUAL(EstadoProtecao::Travado,
                          protecao.atualizar(PotenciaRodas{-400, -400}, MedidasProtecao{true, frente, 0}, DT_MS));
    frente += 10;
  }
}

/// Girando no lugar, o ToF frontal fica parado, mas o giroscópio acusa a rotação: não trava.
void test_giro_no_lugar_nao_trava() {
  ProtecaoMotorTravado protecao;
  TEST_ASSERT_NOT_EQUAL(EstadoProtecao::Travado,
                        rodar(protecao, PotenciaRodas{400, -400}, MedidasProtecao{true, 500, 900}, 20));
}

/// O sinal da taxa de giro não importa: giro para os dois lados conta.
void test_giro_negativo_tambem_conta() {
  ProtecaoMotorTravado protecao;
  TEST_ASSERT_NOT_EQUAL(EstadoProtecao::Travado,
                        rodar(protecao, PotenciaRodas{-400, 400}, MedidasProtecao{true, 500, -900}, 20));
}

/// Variação exatamente igual ao mínimo já conta como movimento; um milímetro a menos, não.
void test_limite_da_variacao_frontal() {
  ProtecaoMotorTravado se_moveu;
  se_moveu.atualizar(CRUZEIRO, parado(500), 250);
  TEST_ASSERT_EQUAL(EstadoProtecao::Monitorando,
                    se_moveu.atualizar(CRUZEIRO, parado(500 - VARIACAO_MINIMA_FRENTE_MM), 250));

  ProtecaoMotorTravado preso;
  preso.atualizar(CRUZEIRO, parado(500), 250);
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado,
                    preso.atualizar(CRUZEIRO, parado(500 - VARIACAO_MINIMA_FRENTE_MM + 1), 250));
}

/// Taxa de giro exatamente igual ao mínimo conta como giro; um décimo a menos, não.
void test_limite_da_taxa_de_giro() {
  ProtecaoMotorTravado girando;
  girando.atualizar(CRUZEIRO, MedidasProtecao{true, 500, TAXA_GIRO_MINIMA_DECIMOS_DPS}, 250);
  TEST_ASSERT_EQUAL(EstadoProtecao::Monitorando,
                    girando.atualizar(CRUZEIRO, MedidasProtecao{true, 500, 0}, 250));

  ProtecaoMotorTravado preso;
  preso.atualizar(CRUZEIRO, MedidasProtecao{true, 500, TAXA_GIRO_MINIMA_DECIMOS_DPS - 1}, 250);
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, preso.atualizar(CRUZEIRO, MedidasProtecao{true, 500, 0}, 250));
}

// ---------------------------------------------------------------- reinício da contagem

/// Sem leitura frontal válida não dá para concluir: a contagem recomeça e não trava.
void test_sem_leitura_frontal_nao_trava_e_zera_a_contagem() {
  ProtecaoMotorTravado protecao;
  rodar(protecao, CRUZEIRO, parado(), 4);
  TEST_ASSERT_EQUAL(EstadoProtecao::Normal, protecao.atualizar(CRUZEIRO, MedidasProtecao{false, 0, 0}, DT_MS));
  TEST_ASSERT_EQUAL_UINT32(0, protecao.tempoMonitoradoMs());
  TEST_ASSERT_EQUAL(EstadoProtecao::Monitorando, rodar(protecao, CRUZEIRO, parado(), 4));
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, rodar(protecao, CRUZEIRO, parado(), 1));
}

/// A potência cair abaixo do duty alto (por exemplo, o robô parou) zera a contagem.
void test_potencia_baixa_no_meio_zera_a_contagem() {
  ProtecaoMotorTravado protecao;
  rodar(protecao, CRUZEIRO, parado(), 4);
  TEST_ASSERT_EQUAL(EstadoProtecao::Normal, protecao.atualizar(FRACA, parado(), DT_MS));
  TEST_ASSERT_EQUAL(EstadoProtecao::Monitorando, rodar(protecao, CRUZEIRO, parado(), 4));
}

/// O robô anda por um tempo, para de andar (preso) e a janela seguinte o pega.
void test_trava_depois_de_andar_um_tempo() {
  ProtecaoMotorTravado protecao;
  uint16_t frente = 900;
  for (int i = 0; i < 12; ++i) {
    TEST_ASSERT_NOT_EQUAL(EstadoProtecao::Travado,
                          protecao.atualizar(CRUZEIRO, MedidasProtecao{true, frente, 0}, DT_MS));
    frente -= 20;
  }
  // Encostou e travou: a leitura para de mudar.
  EstadoProtecao estado = EstadoProtecao::Normal;
  for (int i = 0; i < 12 && estado != EstadoProtecao::Travado; ++i) {
    estado = protecao.atualizar(CRUZEIRO, parado(frente), DT_MS);
  }
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, estado);
}

/// `Travado` fica travado até `reiniciar()`, mesmo que a potência caia ou o robô se mova.
void test_travado_so_sai_com_reiniciar() {
  ProtecaoMotorTravado protecao;
  rodar(protecao, CRUZEIRO, parado(), 5);
  TEST_ASSERT_TRUE(protecao.travado());
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, protecao.atualizar(PotenciaRodas{0, 0}, parado(), DT_MS));
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado,
                    protecao.atualizar(CRUZEIRO, MedidasProtecao{true, 100, 900}, DT_MS));

  protecao.reiniciar();
  TEST_ASSERT_FALSE(protecao.travado());
  TEST_ASSERT_EQUAL_UINT32(0, protecao.tempoMonitoradoMs());
  TEST_ASSERT_EQUAL(EstadoProtecao::Monitorando, rodar(protecao, CRUZEIRO, parado(), 4));
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, rodar(protecao, CRUZEIRO, parado(), 1));
}

/// Os limites vêm da configuração: tempo, potência, variação e giro.
void test_configuracao_altera_os_limites() {
  ConfiguracaoProtecaoMotor config;
  config.tempoLimiteMs = 200;
  config.potenciaAltaMinima = 100;
  config.variacaoMinimaFrenteMm = 20;
  config.taxaGiroMinimaDecimosDps = 300;
  ProtecaoMotorTravado protecao(config);

  // Potência 150 já é "alta"; variação de 10 mm e giro de 20 °/s não bastam; 200 ms trava.
  TEST_ASSERT_EQUAL(EstadoProtecao::Monitorando,
                    protecao.atualizar(PotenciaRodas{150, 150}, MedidasProtecao{true, 500, 0}, 100));
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado,
                    protecao.atualizar(PotenciaRodas{150, 150}, MedidasProtecao{true, 490, 200}, 100));
}

/// `dtMs` zero não faz o tempo andar.
void test_dt_zero_nao_avanca_o_tempo() {
  ProtecaoMotorTravado protecao;
  for (int i = 0; i < 100; ++i) {
    protecao.atualizar(CRUZEIRO, parado(), 0);
  }
  TEST_ASSERT_FALSE(protecao.travado());
  TEST_ASSERT_EQUAL_UINT32(0, protecao.tempoMonitoradoMs());
}

// ---------------------------------------------------------------- integração

namespace {

/// Um ciclo da navegação: o controle comanda os motores a partir do LiDAR simulado e a proteção
/// confere o resultado. Se travar, freia, como a tarefa de navegação fará.
EstadoProtecao cicloDaNavegacao(ControleMovimento& controle, LidarSimulado& lidar, MotoresSimulados& motores,
                                ProtecaoMotorTravado& protecao, int16_t taxaGiroDecimosDps) {
  controle.atualizar(lidar, motores, DT_MS);
  DistanciasLaterais leitura = {0, 0, 0};
  const bool valida = lidar.lerDistanciasLaterais(leitura);
  const EstadoProtecao estado = protecao.atualizar(
      motores.ultimaPotencia(), MedidasProtecao{valida, leitura.frenteMm, taxaGiroDecimosDps}, DT_MS);
  if (estado == EstadoProtecao::Travado) {
    motores.parar();
  }
  return estado;
}

}  // namespace

/// Robô preso numa parede falsa: o controle manda potência de cruzeiro, a leitura não muda, e a
/// proteção freia os motores no tempo limite.
void test_integracao_robo_preso_e_freado() {
  LidarSimulado lidar;
  MotoresSimulados motores;
  ControleMovimento controle;
  ProtecaoMotorTravado protecao;
  lidar.definirLeituraFixa(DistanciasLaterais{600, 84, 84});  // centralizado, caminho livre

  EstadoProtecao estado = EstadoProtecao::Normal;
  int ciclos = 0;
  while (estado != EstadoProtecao::Travado && ciclos < 20) {
    estado = cicloDaNavegacao(controle, lidar, motores, protecao, 0);
    ++ciclos;
  }
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, estado);
  TEST_ASSERT_EQUAL_INT(5, ciclos);  // 500 ms em ciclos de 100 ms
  TEST_ASSERT_TRUE(motores.freados());
  TEST_ASSERT_EQUAL_INT16(0, motores.ultimaPotencia().esquerda);
}

/// Robô andando de verdade: a leitura frontal diminui a cada ciclo e a proteção nunca atua, até o
/// controle parar na parede.
void test_integracao_robo_andando_nao_e_freado() {
  LidarSimulado lidar;
  MotoresSimulados motores;
  ControleMovimento controle;
  ProtecaoMotorTravado protecao;

  uint16_t frente = 800;
  for (int ciclo = 0; ciclo < 60 && !controle.parado(); ++ciclo) {
    lidar.definirLeituraFixa(DistanciasLaterais{frente, 84, 84});
    TEST_ASSERT_NOT_EQUAL(EstadoProtecao::Travado,
                          cicloDaNavegacao(controle, lidar, motores, protecao, 0));
    frente = static_cast<uint16_t>(frente > 20 ? frente - 20 : 0);  // 20 mm por ciclo de 100 ms
  }
  TEST_ASSERT_TRUE(controle.parado());
  TEST_ASSERT_EQUAL(EstadoProtecao::Normal, protecao.estado());  // parou na parede: potência zero
  TEST_ASSERT_FALSE(protecao.travado());
}

/// Depois de travar, a navegação reinicia o controle e a proteção; sem o obstáculo, o robô anda
/// de novo sem ser freado.
void test_integracao_destrava_e_volta_a_andar() {
  LidarSimulado lidar;
  MotoresSimulados motores;
  ControleMovimento controle;
  ProtecaoMotorTravado protecao;
  lidar.definirLeituraFixa(DistanciasLaterais{600, 84, 84});
  for (int i = 0; i < 5; ++i) {
    cicloDaNavegacao(controle, lidar, motores, protecao, 0);
  }
  TEST_ASSERT_TRUE(protecao.travado());

  protecao.reiniciar();
  controle.reiniciar();
  uint16_t frente = 600;
  for (int i = 0; i < 10; ++i) {
    lidar.definirLeituraFixa(DistanciasLaterais{frente, 84, 84});
    TEST_ASSERT_NOT_EQUAL(EstadoProtecao::Travado,
                          cicloDaNavegacao(controle, lidar, motores, protecao, 0));
    frente -= 15;
  }
  TEST_ASSERT_FALSE(motores.freados());
}

/// Sem leitura do sensor (falha), a proteção não conclui nada e o controle segue seu fail-safe.
void test_integracao_sem_leitura_nao_dispara_a_protecao() {
  LidarSimulado lidar;
  MotoresSimulados motores;
  ControleMovimento controle;
  ProtecaoMotorTravado protecao;
  lidar.simularFalha(true);
  for (int i = 0; i < 20; ++i) {
    TEST_ASSERT_NOT_EQUAL(EstadoProtecao::Travado,
                          cicloDaNavegacao(controle, lidar, motores, protecao, 0));
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_duty_alto_sem_movimento_trava_no_tempo_limite);
  RUN_TEST(test_nao_trava_antes_do_tempo_limite);
  RUN_TEST(test_potencia_baixa_nunca_trava);
  RUN_TEST(test_basta_uma_roda_em_duty_alto);
  RUN_TEST(test_re_em_duty_alto_tambem_trava);
  RUN_TEST(test_limite_da_potencia_alta);
  RUN_TEST(test_aproximacao_da_parede_nao_trava);
  RUN_TEST(test_afastamento_da_parede_nao_trava);
  RUN_TEST(test_giro_no_lugar_nao_trava);
  RUN_TEST(test_giro_negativo_tambem_conta);
  RUN_TEST(test_limite_da_variacao_frontal);
  RUN_TEST(test_limite_da_taxa_de_giro);
  RUN_TEST(test_sem_leitura_frontal_nao_trava_e_zera_a_contagem);
  RUN_TEST(test_potencia_baixa_no_meio_zera_a_contagem);
  RUN_TEST(test_trava_depois_de_andar_um_tempo);
  RUN_TEST(test_travado_so_sai_com_reiniciar);
  RUN_TEST(test_configuracao_altera_os_limites);
  RUN_TEST(test_dt_zero_nao_avanca_o_tempo);
  RUN_TEST(test_integracao_robo_preso_e_freado);
  RUN_TEST(test_integracao_robo_andando_nao_e_freado);
  RUN_TEST(test_integracao_destrava_e_volta_a_andar);
  RUN_TEST(test_integracao_sem_leitura_nao_dispara_a_protecao);
  return UNITY_END();
}
