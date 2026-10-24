/// @file test_imu.cpp
/// @brief Testes da IMU (#378): conversão da leitura bruta do MPU6050, `ImuSimulado`,
/// calibração do viés e integração do ângulo de giro (yaw), incluindo deriva e ruído.
/// Roda no computador: `pio test -e native -f test_imu`.

#include <unity.h>

#include <cstdint>

#include "AcompanhadorGiro.h"
#include "ConfiguracaoImu.h"
#include "ConversorGiro.h"
#include "IImu.h"
#include "ImuSimulado.h"
#include "IntegradorGiro.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint32_t DT_MS = PERIODO_LEITURA_IMU_MS;  ///< 10 ms, o ciclo da navegação.
constexpr int32_t NOVENTA_GRAUS = 90000;            ///< Em mili-graus.

/// Integra a mesma taxa por `ciclos` ciclos de `DT_MS` e devolve o ângulo final.
int32_t integrar(IntegradorGiro& giro, int16_t taxa, int ciclos) {
  int32_t angulo = giro.anguloMiligraus();
  for (int i = 0; i < ciclos; ++i) {
    angulo = giro.atualizar(taxa, DT_MS);
  }
  return angulo;
}

/// Calibra o integrador com `amostras` leituras iguais e devolve o estado final.
EstadoCalibracao calibrarCom(IntegradorGiro& giro, int16_t taxa, uint16_t amostras = AMOSTRAS_CALIBRACAO_GIRO) {
  giro.iniciarCalibracao();
  EstadoCalibracao estado = giro.estadoCalibracao();
  for (uint16_t i = 0; i < amostras; ++i) {
    estado = giro.adicionarAmostraCalibracao(taxa);
  }
  return estado;
}

/// Lê a IMU e integra, por `ciclos` ciclos, como a tarefa de navegação fará na placa.
int32_t lerEIntegrar(IImu& imu, IntegradorGiro& giro, int ciclos) {
  for (int i = 0; i < ciclos; ++i) {
    int16_t taxa = 0;
    TEST_ASSERT_TRUE(imu.lerTaxaGiroZ(taxa));
    giro.atualizar(taxa, DT_MS);
  }
  return giro.anguloMiligraus();
}

}  // namespace

// ---------------------------------------------------------------- conversão da leitura bruta

void test_conversao_um_grau_por_segundo_em_cada_faixa() {
  TEST_ASSERT_EQUAL_INT16(10, converterBrutoParaDecimosDps(131, FaixaGiro::Dps250));
  TEST_ASSERT_EQUAL_INT16(10, converterBrutoParaDecimosDps(66, FaixaGiro::Dps500));   // 65,5 LSB
  TEST_ASSERT_EQUAL_INT16(10, converterBrutoParaDecimosDps(33, FaixaGiro::Dps1000));  // 32,8 LSB
  TEST_ASSERT_EQUAL_INT16(10, converterBrutoParaDecimosDps(16, FaixaGiro::Dps2000));  // 16,4 LSB
}

void test_conversao_preserva_o_sinal() {
  TEST_ASSERT_EQUAL_INT16(-10, converterBrutoParaDecimosDps(-131, FaixaGiro::Dps250));
  TEST_ASSERT_EQUAL_INT16(0, converterBrutoParaDecimosDps(0, FaixaGiro::Dps500));
}

void test_conversao_arredonda_para_o_mais_proximo() {
  // 7 LSB na faixa de ±250 °/s = 0,534 décimo → 1; 6 LSB = 0,458 → 0.
  TEST_ASSERT_EQUAL_INT16(1, converterBrutoParaDecimosDps(7, FaixaGiro::Dps250));
  TEST_ASSERT_EQUAL_INT16(0, converterBrutoParaDecimosDps(6, FaixaGiro::Dps250));
  TEST_ASSERT_EQUAL_INT16(-1, converterBrutoParaDecimosDps(-7, FaixaGiro::Dps250));
}

void test_conversao_no_limite_da_faixa() {
  // Fundo de escala de ±500 °/s: 32767 LSB ≈ 500,3 °/s.
  TEST_ASSERT_EQUAL_INT16(5003, converterBrutoParaDecimosDps(INT16_MAX, FaixaGiro::Dps500));
  TEST_ASSERT_EQUAL_INT16(-5003, converterBrutoParaDecimosDps(INT16_MIN + 1, FaixaGiro::Dps500));
  TEST_ASSERT_EQUAL_INT16(19980, converterBrutoParaDecimosDps(INT16_MAX, FaixaGiro::Dps2000));  // ≈ 1998 °/s
}

void test_faixa_do_robo_cobre_um_giro_rapido() {
  // Um giro de 90° em 0,3 s passa de 300 °/s: a faixa escolhida precisa medir isso sem saturar.
  TEST_ASSERT_TRUE(converterBrutoParaDecimosDps(INT16_MAX, FAIXA_GIRO) > 3000);
}

void test_valor_do_gyro_config_para_cada_faixa() {
  // FS_SEL nos bits 4 e 3 do GYRO_CONFIG; os bits de autoteste (7 a 5) ficam em zero.
  TEST_ASSERT_EQUAL_HEX8(0x00, valorGyroConfig(FaixaGiro::Dps250));
  TEST_ASSERT_EQUAL_HEX8(0x08, valorGyroConfig(FaixaGiro::Dps500));
  TEST_ASSERT_EQUAL_HEX8(0x10, valorGyroConfig(FaixaGiro::Dps1000));
  TEST_ASSERT_EQUAL_HEX8(0x18, valorGyroConfig(FaixaGiro::Dps2000));
  TEST_ASSERT_EQUAL_HEX8(0x08, valorGyroConfig(FAIXA_GIRO));  // a faixa do robô: ±500 °/s
}

void test_juntar_bytes_em_complemento_de_dois() {
  // O MPU6050 manda o byte alto primeiro.
  TEST_ASSERT_EQUAL_INT16(0x0102, juntarBytesGiro(0x01, 0x02));
  TEST_ASSERT_EQUAL_INT16(-1, juntarBytesGiro(0xFF, 0xFF));
  TEST_ASSERT_EQUAL_INT16(INT16_MIN, juntarBytesGiro(0x80, 0x00));
  TEST_ASSERT_EQUAL_INT16(INT16_MAX, juntarBytesGiro(0x7F, 0xFF));
  // 65,5 LSB = 1 °/s na faixa do robô: 0x0042 (66) vira 10 décimos.
  TEST_ASSERT_EQUAL_INT16(10, converterBrutoParaDecimosDps(juntarBytesGiro(0x00, 0x42), FAIXA_GIRO));
}

void test_sensor_de_cabeca_para_baixo_inverte_o_sinal() {
  TEST_ASSERT_EQUAL_INT16(900, ajustarSinalGiro(900, false));
  TEST_ASSERT_EQUAL_INT16(-900, ajustarSinalGiro(900, true));
  TEST_ASSERT_EQUAL_INT16(900, ajustarSinalGiro(-900, true));
  TEST_ASSERT_EQUAL_INT16(INT16_MAX, ajustarSinalGiro(INT16_MIN, true));  // sem estourar
}

// ---------------------------------------------------------------- ImuSimulado

void test_imu_simulada_soma_taxa_e_vies() {
  ImuSimulado imu;
  imu.definirTaxa(900);
  imu.definirVies(12);
  int16_t taxa = 0;
  TEST_ASSERT_TRUE(imu.lerTaxaGiroZ(taxa));
  TEST_ASSERT_EQUAL_INT16(912, taxa);
  TEST_ASSERT_EQUAL_UINT32(1, imu.leiturasFeitas());
}

void test_imu_simulada_ruido_alterna_o_sinal() {
  ImuSimulado imu;
  imu.definirRuidoAlternado(2);
  int16_t taxa = 0;
  imu.lerTaxaGiroZ(taxa);
  TEST_ASSERT_EQUAL_INT16(2, taxa);
  imu.lerTaxaGiroZ(taxa);
  TEST_ASSERT_EQUAL_INT16(-2, taxa);
  imu.lerTaxaGiroZ(taxa);
  TEST_ASSERT_EQUAL_INT16(2, taxa);
}

void test_imu_simulada_em_falha_nao_altera_a_saida() {
  ImuSimulado imu;
  imu.definirTaxa(500);
  imu.simularFalha(true);
  int16_t taxa = 123;
  TEST_ASSERT_FALSE(imu.lerTaxaGiroZ(taxa));
  TEST_ASSERT_EQUAL_INT16(123, taxa);
  TEST_ASSERT_EQUAL_UINT32(0, imu.leiturasFeitas());
  imu.simularFalha(false);
  TEST_ASSERT_TRUE(imu.lerTaxaGiroZ(taxa));
  TEST_ASSERT_EQUAL_INT16(500, taxa);
}

void test_imu_simulada_satura_no_limite_de_int16() {
  ImuSimulado imu;
  imu.definirTaxa(INT16_MAX);
  imu.definirVies(10);
  int16_t taxa = 0;
  imu.lerTaxaGiroZ(taxa);
  TEST_ASSERT_EQUAL_INT16(INT16_MAX, taxa);
  imu.definirTaxa(INT16_MIN);
  imu.definirVies(-10);
  imu.lerTaxaGiroZ(taxa);
  TEST_ASSERT_EQUAL_INT16(INT16_MIN, taxa);
}

void test_imu_simulada_pode_ser_usada_pela_interface() {
  ImuSimulado imu;
  imu.definirTaxa(-300);
  IImu& sensor = imu;
  int16_t taxa = 0;
  TEST_ASSERT_TRUE(sensor.lerTaxaGiroZ(taxa));
  TEST_ASSERT_EQUAL_INT16(-300, taxa);
}

// ---------------------------------------------------------------- integração do ângulo

void test_noventa_graus_por_segundo_durante_um_segundo_da_noventa_graus() {
  IntegradorGiro giro;
  TEST_ASSERT_EQUAL_INT32(NOVENTA_GRAUS, integrar(giro, 900, 100));
}

void test_giro_para_a_direita_e_negativo() {
  IntegradorGiro giro;
  TEST_ASSERT_EQUAL_INT32(-NOVENTA_GRAUS, integrar(giro, -900, 100));
}

void test_dt_zero_nao_muda_o_angulo() {
  IntegradorGiro giro;
  giro.atualizar(900, 0);
  TEST_ASSERT_EQUAL_INT32(0, giro.anguloMiligraus());
}

void test_fracoes_nao_se_perdem_entre_ciclos() {
  // 0,1 °/s (1 décimo) por 1 ms soma só 0,1 mili-grau por ciclo. Se cada ciclo fosse arredondado,
  // o ângulo ficaria em zero; com a soma exata, 1000 ciclos dão 100 mili-graus.
  ConfiguracaoIntegradorGiro semZonaMorta;
  semZonaMorta.zonaMortaDecimosDps = 0;
  IntegradorGiro giro(semZonaMorta);
  for (int i = 0; i < 1000; ++i) {
    giro.atualizar(1, 1);
  }
  TEST_ASSERT_EQUAL_INT32(100, giro.anguloMiligraus());
}

void test_zerar_angulo_mantem_o_vies() {
  IntegradorGiro giro;
  calibrarCom(giro, 12);
  integrar(giro, 912, 100);
  giro.zerarAngulo();
  TEST_ASSERT_EQUAL_INT32(0, giro.anguloMiligraus());
  TEST_ASSERT_EQUAL_INT16(12, giro.viesDecimosDps());
}

void test_angulo_normalizado_fica_entre_menos_180_e_180() {
  IntegradorGiro giro;
  integrar(giro, 900, 300);  // 270°
  TEST_ASSERT_EQUAL_INT32(270000, giro.anguloMiligraus());
  TEST_ASSERT_EQUAL_INT32(-90000, giro.anguloNormalizadoMiligraus());

  giro.zerarAngulo();
  integrar(giro, 900, 200);  // 180° fica 180°
  TEST_ASSERT_EQUAL_INT32(180000, giro.anguloNormalizadoMiligraus());

  giro.zerarAngulo();
  integrar(giro, -900, 200);  // −180° vira 180°
  TEST_ASSERT_EQUAL_INT32(180000, giro.anguloNormalizadoMiligraus());

  giro.zerarAngulo();
  integrar(giro, 900, 500);  // 450° vira 90°
  TEST_ASSERT_EQUAL_INT32(NOVENTA_GRAUS, giro.anguloNormalizadoMiligraus());
}

// ---------------------------------------------------------------- viés, deriva e calibração

void test_sem_calibrar_o_vies_vira_deriva() {
  // Robô parado com viés de 1,2 °/s: em 10 s, o ângulo andaria 12°.
  IntegradorGiro giro;
  TEST_ASSERT_EQUAL(EstadoCalibracao::NaoCalibrado, giro.estadoCalibracao());
  TEST_ASSERT_EQUAL_INT32(12000, integrar(giro, 12, 1000));
}

void test_calibracao_remove_o_vies_e_a_deriva() {
  IntegradorGiro giro;
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, calibrarCom(giro, 12));
  TEST_ASSERT_EQUAL_INT16(12, giro.viesDecimosDps());
  TEST_ASSERT_EQUAL_INT32(0, integrar(giro, 12, 1000));  // 10 s parado: sem deriva
}

void test_calibracao_com_vies_negativo() {
  IntegradorGiro giro;
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, calibrarCom(giro, -8));
  TEST_ASSERT_EQUAL_INT16(-8, giro.viesDecimosDps());
  TEST_ASSERT_EQUAL_INT32(-NOVENTA_GRAUS, integrar(giro, -908, 100));
}

void test_calibracao_usa_a_media_arredondada() {
  ConfiguracaoIntegradorGiro quatro;
  quatro.amostrasCalibracao = 4;
  IntegradorGiro giro(quatro);
  giro.iniciarCalibracao();
  giro.adicionarAmostraCalibracao(10);
  giro.adicionarAmostraCalibracao(11);
  giro.adicionarAmostraCalibracao(11);
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, giro.adicionarAmostraCalibracao(11));  // média 10,75
  TEST_ASSERT_EQUAL_INT16(11, giro.viesDecimosDps());

  giro.iniciarCalibracao();
  const int16_t negativas[] = {-10, -11, -11, -11};
  for (int16_t amostra : negativas) {
    giro.adicionarAmostraCalibracao(amostra);
  }
  TEST_ASSERT_EQUAL_INT16(-11, giro.viesDecimosDps());
}

void test_calibracao_incompleta_segue_calibrando_e_nao_troca_o_vies() {
  IntegradorGiro giro;
  calibrarCom(giro, 12);
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrando, calibrarCom(giro, 30, AMOSTRAS_CALIBRACAO_GIRO - 1));
  TEST_ASSERT_EQUAL_INT16(12, giro.viesDecimosDps());
}

void test_robo_se_mexendo_na_calibracao_e_recusada() {
  ConfiguracaoIntegradorGiro quatro;
  quatro.amostrasCalibracao = 4;
  IntegradorGiro giro(quatro);
  calibrarCom(giro, 12, 4);

  giro.iniciarCalibracao();
  giro.adicionarAmostraCalibracao(10);
  giro.adicionarAmostraCalibracao(10);
  giro.adicionarAmostraCalibracao(200);  // alguém encostou no robô
  TEST_ASSERT_EQUAL(EstadoCalibracao::Recusada, giro.adicionarAmostraCalibracao(10));
  TEST_ASSERT_EQUAL_INT16(12, giro.viesDecimosDps());  // continua o viés anterior

  // Depois de recusada, amostras soltas são ignoradas até uma nova calibração.
  TEST_ASSERT_EQUAL(EstadoCalibracao::Recusada, giro.adicionarAmostraCalibracao(10));
}

void test_limite_da_variacao_na_calibracao() {
  ConfiguracaoIntegradorGiro dois;
  dois.amostrasCalibracao = 2;
  IntegradorGiro giro(dois);
  giro.iniciarCalibracao();
  giro.adicionarAmostraCalibracao(0);
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, giro.adicionarAmostraCalibracao(VARIACAO_MAXIMA_CALIBRACAO_DECIMOS_DPS));
  giro.iniciarCalibracao();
  giro.adicionarAmostraCalibracao(0);
  TEST_ASSERT_EQUAL(EstadoCalibracao::Recusada, giro.adicionarAmostraCalibracao(VARIACAO_MAXIMA_CALIBRACAO_DECIMOS_DPS + 1));
}

void test_amostra_fora_de_calibracao_e_ignorada() {
  // Com uma amostra só já bastando, uma amostra solta, sem iniciarCalibracao(), não pode calibrar.
  ConfiguracaoIntegradorGiro uma;
  uma.amostrasCalibracao = 1;
  IntegradorGiro giro(uma);
  TEST_ASSERT_EQUAL(EstadoCalibracao::NaoCalibrado, giro.adicionarAmostraCalibracao(50));
  TEST_ASSERT_EQUAL_INT16(0, giro.viesDecimosDps());

  // Depois de uma calibração concluída, também não: o viés só muda com nova calibração.
  calibrarCom(giro, 12, 1);
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, giro.adicionarAmostraCalibracao(50));
  TEST_ASSERT_EQUAL_INT16(12, giro.viesDecimosDps());
}

void test_zona_morta_segura_o_ruido_do_sensor_parado() {
  IntegradorGiro giro;
  calibrarCom(giro, 12);
  // Parado, o sensor oscila 0,3 °/s em torno do viés: dentro da zona morta, o ângulo não anda.
  TEST_ASSERT_EQUAL_INT32(0, integrar(giro, 12 + ZONA_MORTA_GIRO_DECIMOS_DPS, 500));
  TEST_ASSERT_EQUAL_INT32(0, integrar(giro, 12 - ZONA_MORTA_GIRO_DECIMOS_DPS, 500));
  // Um pouco acima da zona morta já conta: 0,4 °/s por 10 s = 4°.
  TEST_ASSERT_EQUAL_INT32(4000, integrar(giro, 12 + ZONA_MORTA_GIRO_DECIMOS_DPS + 1, 1000));
}

void test_deriva_do_vies_depois_da_calibracao() {
  // O viés muda com a temperatura. Se mudar até a zona morta, o ângulo não deriva; acima dela,
  // deriva e é preciso calibrar de novo (por exemplo, a cada parada na largada).
  IntegradorGiro giro;
  calibrarCom(giro, 12);
  TEST_ASSERT_EQUAL_INT32(0, integrar(giro, 15, 1000));       // +0,3 °/s: dentro da zona morta
  TEST_ASSERT_EQUAL_INT32(5000, integrar(giro, 17, 1000));    // +0,5 °/s: 5° em 10 s
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, calibrarCom(giro, 17));
  giro.zerarAngulo();
  TEST_ASSERT_EQUAL_INT32(0, integrar(giro, 17, 1000));       // recalibrado: sem deriva
}

// ---------------------------------------------------------------- integração: IMU simulada + integrador

void test_integracao_giro_de_90_graus_com_vies_e_ruido() {
  ImuSimulado imu;
  imu.definirVies(15);
  imu.definirRuidoAlternado(2);
  IntegradorGiro giro;

  // Largada: robô parado, calibra com o sensor real (viés + ruído).
  giro.iniciarCalibracao();
  for (uint16_t i = 0; i < AMOSTRAS_CALIBRACAO_GIRO; ++i) {
    int16_t taxa = 0;
    TEST_ASSERT_TRUE(imu.lerTaxaGiroZ(taxa));
    giro.adicionarAmostraCalibracao(taxa);
  }
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, giro.estadoCalibracao());
  TEST_ASSERT_EQUAL_INT16(15, giro.viesDecimosDps());

  // Giro para a esquerda a 90 °/s por 1 s.
  imu.definirTaxa(900);
  TEST_ASSERT_EQUAL_INT32(NOVENTA_GRAUS, lerEIntegrar(imu, giro, 100));

  // Parado de novo por 5 s: o ângulo não deriva.
  imu.definirTaxa(0);
  TEST_ASSERT_EQUAL_INT32(NOVENTA_GRAUS, lerEIntegrar(imu, giro, 500));
}

void test_integracao_giro_de_180_graus_para_a_direita() {
  ImuSimulado imu;
  imu.definirVies(-6);
  IntegradorGiro giro;
  giro.iniciarCalibracao();
  for (uint16_t i = 0; i < AMOSTRAS_CALIBRACAO_GIRO; ++i) {
    int16_t taxa = 0;
    imu.lerTaxaGiroZ(taxa);
    giro.adicionarAmostraCalibracao(taxa);
  }
  imu.definirTaxa(-3000);  // 300 °/s para a direita, por 0,6 s
  TEST_ASSERT_EQUAL_INT32(-180000, lerEIntegrar(imu, giro, 60));
  TEST_ASSERT_EQUAL_INT32(180000, giro.anguloNormalizadoMiligraus());
}

void test_integracao_leitura_com_falha_nao_soma_ao_angulo() {
  ImuSimulado imu;
  imu.definirTaxa(900);
  IntegradorGiro giro;
  int16_t taxa = 0;
  imu.simularFalha(true);
  if (imu.lerTaxaGiroZ(taxa)) {
    giro.atualizar(taxa, DT_MS);
  }
  TEST_ASSERT_EQUAL_INT32(0, giro.anguloMiligraus());
}

// ---------------------------------------------------------------- AcompanhadorGiro (ciclo da tarefa da IMU)

namespace {

/// Roda `ciclos` ciclos de `DT_MS` no acompanhador.
const LeituraGiro& rodarCiclos(AcompanhadorGiro& giro, int ciclos) {
  for (int i = 0; i + 1 < ciclos; ++i) {
    giro.ciclo(DT_MS);
  }
  return giro.ciclo(DT_MS);
}

}  // namespace

void test_acompanhador_calibra_na_largada_e_so_entao_fica_valido() {
  ImuSimulado imu;
  imu.definirVies(15);
  imu.definirRuidoAlternado(2);
  AcompanhadorGiro giro(imu);
  giro.iniciarCalibracao();

  const LeituraGiro& durante = rodarCiclos(giro, AMOSTRAS_CALIBRACAO_GIRO - 1);
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrando, durante.calibracao);
  TEST_ASSERT_FALSE(durante.valida);
  TEST_ASSERT_EQUAL_INT32(0, durante.anguloMiligraus);

  const LeituraGiro& calibrado = giro.ciclo(DT_MS);  // a amostra 200 fecha a calibração
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, calibrado.calibracao);
  TEST_ASSERT_TRUE(calibrado.valida);
  TEST_ASSERT_EQUAL_INT32(0, calibrado.anguloMiligraus);
  TEST_ASSERT_EQUAL_UINT32(AMOSTRAS_CALIBRACAO_GIRO, imu.leiturasFeitas());
}

void test_acompanhador_integra_um_giro_de_90_graus_sem_o_vies() {
  ImuSimulado imu;
  imu.definirVies(-8);
  AcompanhadorGiro giro(imu);
  giro.iniciarCalibracao();
  rodarCiclos(giro, AMOSTRAS_CALIBRACAO_GIRO);

  imu.definirTaxa(900);  // 90 °/s para a esquerda, por 1 s
  const LeituraGiro& leitura = rodarCiclos(giro, 100);
  TEST_ASSERT_EQUAL_INT32(NOVENTA_GRAUS, leitura.anguloMiligraus);
  TEST_ASSERT_EQUAL_INT16(900, leitura.taxaDecimosDps);  // a taxa publicada já vem sem o viés
  TEST_ASSERT_TRUE(leitura.valida);

  giro.zerarAngulo();
  TEST_ASSERT_EQUAL_INT32(0, giro.ultima().anguloMiligraus);
  imu.definirTaxa(-900);
  TEST_ASSERT_EQUAL_INT32(-NOVENTA_GRAUS, rodarCiclos(giro, 100).anguloMiligraus);
}

void test_acompanhador_recomeca_a_calibracao_se_o_robo_se_mexer() {
  ImuSimulado imu;
  AcompanhadorGiro giro(imu);
  giro.iniciarCalibracao();
  imu.definirRuidoAlternado(VARIACAO_MAXIMA_CALIBRACAO_DECIMOS_DPS);  // variação de 2 × 5 °/s: o robô se mexeu
  const LeituraGiro& recusada = rodarCiclos(giro, AMOSTRAS_CALIBRACAO_GIRO);
  TEST_ASSERT_EQUAL_UINT32(1, recusada.calibracoesRecusadas);
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrando, recusada.calibracao);  // recomeçou sozinha
  TEST_ASSERT_FALSE(recusada.valida);

  imu.definirRuidoAlternado(0);  // o robô parou
  const LeituraGiro& calibrado = rodarCiclos(giro, AMOSTRAS_CALIBRACAO_GIRO);
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, calibrado.calibracao);
  TEST_ASSERT_TRUE(calibrado.valida);
  TEST_ASSERT_EQUAL_UINT32(1, calibrado.calibracoesRecusadas);
}

void test_acompanhador_falhas_seguidas_invalidam_o_angulo() {
  ImuSimulado imu;
  AcompanhadorGiro giro(imu);
  giro.iniciarCalibracao();
  rodarCiclos(giro, AMOSTRAS_CALIBRACAO_GIRO);
  imu.definirTaxa(900);
  rodarCiclos(giro, 10);
  TEST_ASSERT_EQUAL_INT32(9000, giro.ultima().anguloMiligraus);

  imu.simularFalha(true);
  const LeituraGiro& poucas = rodarCiclos(giro, FALHAS_SEGUIDAS_GIRO_INVALIDO - 1);
  TEST_ASSERT_TRUE(poucas.valida);                       // falhas isoladas ainda não invalidam
  TEST_ASSERT_EQUAL_INT32(9000, poucas.anguloMiligraus);  // e não somam nada ao ângulo
  const LeituraGiro& muitas = giro.ciclo(DT_MS);
  TEST_ASSERT_FALSE(muitas.valida);
  TEST_ASSERT_EQUAL_UINT32(FALHAS_SEGUIDAS_GIRO_INVALIDO, muitas.falhasLeitura);

  imu.simularFalha(false);  // o sensor voltou: a próxima leitura boa revalida
  const LeituraGiro& voltou = giro.ciclo(DT_MS);
  TEST_ASSERT_TRUE(voltou.valida);
  TEST_ASSERT_EQUAL_INT32(9900, voltou.anguloMiligraus);
}

void test_acompanhador_sem_calibrar_nao_e_valido_mas_integra() {
  ImuSimulado imu;
  imu.definirTaxa(100);
  AcompanhadorGiro giro(imu);  // sem iniciarCalibracao(): viés zero
  const LeituraGiro& leitura = rodarCiclos(giro, 100);
  TEST_ASSERT_FALSE(leitura.valida);
  TEST_ASSERT_EQUAL(EstadoCalibracao::NaoCalibrado, leitura.calibracao);
  TEST_ASSERT_EQUAL_INT32(10000, leitura.anguloMiligraus);
}

void test_acompanhador_falha_durante_a_calibracao_nao_conta_amostra() {
  ImuSimulado imu;
  AcompanhadorGiro giro(imu);
  giro.iniciarCalibracao();
  rodarCiclos(giro, 100);
  imu.simularFalha(true);
  rodarCiclos(giro, 3);
  imu.simularFalha(false);
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrando, rodarCiclos(giro, AMOSTRAS_CALIBRACAO_GIRO - 101).calibracao);
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, giro.ciclo(DT_MS).calibracao);
  TEST_ASSERT_EQUAL_UINT32(3, giro.ultima().falhasLeitura);
}

void test_acompanhador_taxa_publicada_nao_estoura_com_o_vies() {
  ImuSimulado imu;
  imu.definirVies(-40);
  AcompanhadorGiro giro(imu);
  giro.iniciarCalibracao();
  rodarCiclos(giro, AMOSTRAS_CALIBRACAO_GIRO);
  imu.definirVies(0);
  imu.definirTaxa(INT16_MAX);  // leitura saturada; menos o viés de −4 °/s passaria do limite
  TEST_ASSERT_EQUAL_INT16(INT16_MAX, giro.ciclo(DT_MS).taxaDecimosDps);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_conversao_um_grau_por_segundo_em_cada_faixa);
  RUN_TEST(test_conversao_preserva_o_sinal);
  RUN_TEST(test_conversao_arredonda_para_o_mais_proximo);
  RUN_TEST(test_conversao_no_limite_da_faixa);
  RUN_TEST(test_faixa_do_robo_cobre_um_giro_rapido);
  RUN_TEST(test_valor_do_gyro_config_para_cada_faixa);
  RUN_TEST(test_juntar_bytes_em_complemento_de_dois);
  RUN_TEST(test_sensor_de_cabeca_para_baixo_inverte_o_sinal);
  RUN_TEST(test_imu_simulada_soma_taxa_e_vies);
  RUN_TEST(test_imu_simulada_ruido_alterna_o_sinal);
  RUN_TEST(test_imu_simulada_em_falha_nao_altera_a_saida);
  RUN_TEST(test_imu_simulada_satura_no_limite_de_int16);
  RUN_TEST(test_imu_simulada_pode_ser_usada_pela_interface);
  RUN_TEST(test_noventa_graus_por_segundo_durante_um_segundo_da_noventa_graus);
  RUN_TEST(test_giro_para_a_direita_e_negativo);
  RUN_TEST(test_dt_zero_nao_muda_o_angulo);
  RUN_TEST(test_fracoes_nao_se_perdem_entre_ciclos);
  RUN_TEST(test_zerar_angulo_mantem_o_vies);
  RUN_TEST(test_angulo_normalizado_fica_entre_menos_180_e_180);
  RUN_TEST(test_sem_calibrar_o_vies_vira_deriva);
  RUN_TEST(test_calibracao_remove_o_vies_e_a_deriva);
  RUN_TEST(test_calibracao_com_vies_negativo);
  RUN_TEST(test_calibracao_usa_a_media_arredondada);
  RUN_TEST(test_calibracao_incompleta_segue_calibrando_e_nao_troca_o_vies);
  RUN_TEST(test_robo_se_mexendo_na_calibracao_e_recusada);
  RUN_TEST(test_limite_da_variacao_na_calibracao);
  RUN_TEST(test_amostra_fora_de_calibracao_e_ignorada);
  RUN_TEST(test_zona_morta_segura_o_ruido_do_sensor_parado);
  RUN_TEST(test_deriva_do_vies_depois_da_calibracao);
  RUN_TEST(test_integracao_giro_de_90_graus_com_vies_e_ruido);
  RUN_TEST(test_integracao_giro_de_180_graus_para_a_direita);
  RUN_TEST(test_integracao_leitura_com_falha_nao_soma_ao_angulo);
  RUN_TEST(test_acompanhador_calibra_na_largada_e_so_entao_fica_valido);
  RUN_TEST(test_acompanhador_integra_um_giro_de_90_graus_sem_o_vies);
  RUN_TEST(test_acompanhador_recomeca_a_calibracao_se_o_robo_se_mexer);
  RUN_TEST(test_acompanhador_falhas_seguidas_invalidam_o_angulo);
  RUN_TEST(test_acompanhador_sem_calibrar_nao_e_valido_mas_integra);
  RUN_TEST(test_acompanhador_falha_durante_a_calibracao_nao_conta_amostra);
  RUN_TEST(test_acompanhador_taxa_publicada_nao_estoura_com_o_vies);
  return UNITY_END();
}
