/// @file test_rotina_calibracao.cpp
/// @brief Testes do `CalibradorGiro` e da `RotinaCalibracao` unificada (#240, HU-13 / RF-13 / RNF-08).
/// Roda no computador: `pio test -e native -f test_rotina_calibracao`.

#include <stdint.h>
#include <unity.h>

#include "CalibradorGiro.h"
#include "ImuSimulado.h"
#include "SensorDistanciaSimulado.h"
#include "RotinaCalibracao.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

ConfiguracaoCalibracaoGiro configGiroRapido(uint16_t amostras = 5, uint16_t variacao = 50) {
  ConfiguracaoCalibracaoGiro c;
  c.amostras = amostras;
  c.variacaoMaximaDecimosDps = variacao;
  return c;
}

ConfiguracaoRotinaCalibracao configRotinaRapida() {
  ConfiguracaoRotinaCalibracao c;
  c.orcamentoTotalMs = 1000;
  c.tof.amostras = 5;
  c.tof.variacaoMaximaMm = 8;
  c.tof.referencias = {54, 54, 54};
  c.giro.amostras = 10;
  c.giro.variacaoMaximaDecimosDps = 50;
  return c;
}

}  // namespace

// ===========================================================================
// Testes do CalibradorGiro
// ===========================================================================

void test_calibrador_giro_inicial_nao_calibrado_e_vies_zero() {
  CalibradorGiro cal(configGiroRapido());
  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoGiro::NaoCalibrado),
                    static_cast<int>(cal.estado()));
  TEST_ASSERT_EQUAL_INT16(0, cal.viesDecimosDps());
  TEST_ASSERT_EQUAL_UINT16(0, cal.amostrasRecebidas());
}

void test_calibrador_giro_calcula_vies_pela_media_arredondada() {
  CalibradorGiro cal(configGiroRapido(4));
  cal.iniciar();

  cal.adicionarAmostra(12);
  cal.adicionarAmostra(14);
  cal.adicionarAmostra(13);
  const EstadoCalibracaoGiro estadoFinal = cal.adicionarAmostra(15);

  // Média: (12 + 14 + 13 + 15) / 4 = 54 / 4 = 13,5 -> arredonda para 14
  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoGiro::Calibrado),
                    static_cast<int>(estadoFinal));
  TEST_ASSERT_EQUAL_INT16(14, cal.viesDecimosDps());
}

void test_calibrador_giro_vies_negativo_arredondado() {
  CalibradorGiro cal(configGiroRapido(3));
  cal.iniciar();

  cal.adicionarAmostra(-10);
  cal.adicionarAmostra(-11);
  cal.adicionarAmostra(-11);

  // Média: -32 / 3 = -10,67 -> arredonda para -11
  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoGiro::Calibrado),
                    static_cast<int>(cal.estado()));
  TEST_ASSERT_EQUAL_INT16(-11, cal.viesDecimosDps());
}

void test_calibrador_giro_aplica_vies_nas_leituras_corrigir() {
  CalibradorGiro cal(configGiroRapido(3));
  cal.iniciar();

  cal.adicionarAmostra(20);
  cal.adicionarAmostra(20);
  cal.adicionarAmostra(20);

  TEST_ASSERT_EQUAL_INT16(20, cal.viesDecimosDps());
  // Leitura bruta 500 (50 °/s) com viés 20 (2 °/s) -> 480
  TEST_ASSERT_EQUAL_INT16(480, cal.corrigir(500));
  // Leitura bruta -100 com viés 20 -> -120
  TEST_ASSERT_EQUAL_INT16(-120, cal.corrigir(-100));
  // Leitura em repouso igual ao viés -> 0
  TEST_ASSERT_EQUAL_INT16(0, cal.corrigir(20));
}

void test_calibrador_giro_recusa_se_robo_se_mover_variacao_excessiva() {
  CalibradorGiro cal(configGiroRapido(4, 50));  // Limite: 50 décimos de °/s (5 °/s)
  cal.iniciar();

  cal.adicionarAmostra(5);
  cal.adicionarAmostra(6);
  cal.adicionarAmostra(60);  // Variação = 60 - 5 = 55 > 50 -> robô se moveu
  const EstadoCalibracaoGiro st = cal.adicionarAmostra(5);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoGiro::Recusada), static_cast<int>(st));
  TEST_ASSERT_EQUAL_INT16(0, cal.viesDecimosDps());
}

void test_calibrador_giro_recusa_mantem_vies_anterior() {
  CalibradorGiro cal(configGiroRapido(3, 50));
  cal.iniciar();
  cal.adicionarAmostra(15);
  cal.adicionarAmostra(15);
  cal.adicionarAmostra(15);
  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoGiro::Calibrado),
                    static_cast<int>(cal.estado()));
  TEST_ASSERT_EQUAL_INT16(15, cal.viesDecimosDps());

  // Segunda calibração com movimento
  cal.iniciar();
  cal.adicionarAmostra(10);
  cal.adicionarAmostra(80);
  cal.adicionarAmostra(10);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoGiro::Recusada),
                    static_cast<int>(cal.estado()));
  TEST_ASSERT_EQUAL_INT16(15, cal.viesDecimosDps());  // manteve 15
}

void test_calibrador_giro_amostra_fora_de_calibrando_e_ignorada() {
  CalibradorGiro cal(configGiroRapido(3));
  // Sem chamar iniciar()
  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoGiro::NaoCalibrado),
                    static_cast<int>(cal.adicionarAmostra(100)));
  TEST_ASSERT_EQUAL_UINT16(0, cal.amostrasRecebidas());
}

// ===========================================================================
// Testes da RotinaCalibracao
// ===========================================================================

void test_rotina_calibracao_sucesso_com_imu_simulado_e_sensor_simulado() {
  RotinaCalibracao rotina(configRotinaRapida());
  SensorDistanciaSimulado sensorSimulado;
  ImuSimulado imu;

  // Sensor simulado fornece distâncias com leve offset: frente=58 (+4), esq=50 (-4), dir=56 (+2)
  sensorSimulado.definirLeituraFixa({58, 50, 56});

  // IMU simulada com viés de 15 décimos de °/s (1,5 °/s) e robô parado (taxa=0)
  imu.definirVies(15);
  imu.definirTaxa(0);

  const EstadoRotinaCalibracao st = rotina.executar(sensorSimulado, imu, 10);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::Sucesso), static_cast<int>(st));
  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::Sucesso),
                    static_cast<int>(rotina.estado()));

  // Confere desvios ToF calculados: 58-54 = 4; 50-54 = -4; 56-54 = 2
  TEST_ASSERT_EQUAL_INT16(4, rotina.desviosTof().frenteMm);
  TEST_ASSERT_EQUAL_INT16(-4, rotina.desviosTof().esquerdaMm);
  TEST_ASSERT_EQUAL_INT16(2, rotina.desviosTof().direitaMm);

  // Confere viés do giroscópio: 15
  TEST_ASSERT_EQUAL_INT16(15, rotina.viesGiroDecimosDps());

  // Confere correções aplicadas
  const DistanciasLaterais bruta = {100, 100, 100};
  const DistanciasLaterais corrigida = rotina.corrigirTof(bruta);
  TEST_ASSERT_EQUAL_UINT16(96, corrigida.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(104, corrigida.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(98, corrigida.direitaMm);

  TEST_ASSERT_EQUAL_INT16(185, rotina.corrigirGiro(200));
}

void test_rotina_calibracao_duracao_medida_dentro_do_orcamento_rnf08() {
  // Configuração padrão: 10 amostras ToF (~330 ms) e 50 amostras IMU (500 ms)
  ConfiguracaoRotinaCalibracao configPadrao;
  RotinaCalibracao rotina(configPadrao);
  SensorDistanciaSimulado sensorSimulado;
  ImuSimulado imu;

  sensorSimulado.definirLeituraFixa({54, 54, 54});
  imu.definirVies(0);
  imu.definirTaxa(0);

  const EstadoRotinaCalibracao st = rotina.executar(sensorSimulado, imu, 10);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::Sucesso), static_cast<int>(st));

  // Duração medida: 50 amostras de 10 ms = 500 ms
  TEST_ASSERT_EQUAL_UINT32(500, rotina.duracaoMedidaMs());
  // Confere que coube rigorosamente dentro de 1 s (RNF-08)
  TEST_ASSERT_TRUE(rotina.duracaoMedidaMs() <= ORCAMENTO_CALIBRACAO_MS);
}

void test_rotina_calibracao_falha_tempo_excedido_quando_estoura_orcamento() {
  ConfiguracaoRotinaCalibracao c;
  c.orcamentoTotalMs = 300;  // Força orçamento muito curto: 300 ms
  c.giro.amostras = 50;      // 50 x 10 ms = 500 ms > 300 ms
  RotinaCalibracao rotina(c);
  SensorDistanciaSimulado sensorSimulado;
  ImuSimulado imu;

  sensorSimulado.definirLeituraFixa({54, 54, 54});
  imu.definirVies(10);
  imu.definirTaxa(0);

  const EstadoRotinaCalibracao st = rotina.executar(sensorSimulado, imu, 10);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::TempoExcedido), static_cast<int>(st));
  TEST_ASSERT_TRUE(rotina.duracaoMedidaMs() > c.orcamentoTotalMs);
}

void test_rotina_calibracao_recusa_quando_robo_se_move_no_giro() {
  RotinaCalibracao rotina(configRotinaRapida());
  SensorDistanciaSimulado sensorSimulado;
  ImuSimulado imu;

  sensorSimulado.definirLeituraFixa({54, 54, 54});
  imu.definirVies(0);
  // Simula ruído/movimento abrupto do robô de amplitude 60 décimos de °/s (> 50)
  imu.definirRuidoAlternado(60);

  const EstadoRotinaCalibracao st = rotina.executar(sensorSimulado, imu, 10);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::RecusadaPorMovimento),
                    static_cast<int>(st));
}

void test_rotina_calibracao_recusa_quando_robo_se_move_no_tof() {
  RotinaCalibracao rotina(configRotinaRapida());
  SensorDistanciaSimulado sensorSimulado;
  ImuSimulado imu;

  // Sensor simulado enfileira leituras oscilando mais que variacaoMaximaMm (8 mm)
  sensorSimulado.enfileirarLeitura({50, 54, 54});
  sensorSimulado.enfileirarLeitura({65, 54, 54});  // 65 - 50 = 15 > 8 mm
  sensorSimulado.enfileirarLeitura({50, 54, 54});
  sensorSimulado.enfileirarLeitura({50, 54, 54});
  sensorSimulado.enfileirarLeitura({50, 54, 54});

  imu.definirVies(0);
  imu.definirTaxa(0);

  const EstadoRotinaCalibracao st = rotina.executar(sensorSimulado, imu, 10);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::RecusadaPorMovimento),
                    static_cast<int>(st));
}

void test_rotina_calibracao_aborta_em_caso_de_falha_de_sensor() {
  RotinaCalibracao rotina(configRotinaRapida());
  SensorDistanciaSimulado sensorSimulado;
  ImuSimulado imu;

  sensorSimulado.definirLeituraFixa({54, 54, 54});
  imu.simularFalha(true);  // IMU para de responder

  const EstadoRotinaCalibracao st = rotina.executar(sensorSimulado, imu, 10);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::FalhaSensor), static_cast<int>(st));
}

void test_rotina_calibracao_iniciar_com_referencias_customizadas() {
  RotinaCalibracao rotina(configRotinaRapida());
  rotina.iniciar({60, 70, 80});

  TEST_ASSERT_EQUAL_UINT16(60, rotina.calibradorTof().referencias().frenteMm);
  TEST_ASSERT_EQUAL_UINT16(70, rotina.calibradorTof().referencias().esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(80, rotina.calibradorTof().referencias().direitaMm);
}

void test_rotina_descarta_a_calibracao_se_o_desvio_do_tof_passa_do_maximo() {
  RotinaCalibracao rotina(configRotinaRapida());
  SensorDistanciaSimulado sensorSimulado;
  ImuSimulado imu;
  sensorSimulado.definirLeituraFixa({54, 54, 54 + DESVIO_MAXIMO_TOF_MM + 5});  // referência errada na direita

  const EstadoRotinaCalibracao st = rotina.executar(sensorSimulado, imu, 10);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::DesvioForaDaFaixa), static_cast<int>(st));
  TEST_ASSERT_EQUAL_INT16(0, rotina.desviosTof().direitaMm);  // nada foi adotado
}

void test_rotina_na_largada_calibra_so_os_lados_com_parede() {
  RotinaCalibracao rotina(configRotinaRapida());
  SensorDistanciaSimulado sensorSimulado;
  ImuSimulado imu;
  sensorSimulado.definirLeituraFixa({600, 60, 50});  // frente aberta
  imu.definirVies(12);

  rotina.iniciar(REFERENCIAS_TOF_NA_LARGADA);
  while (rotina.estado() == EstadoRotinaCalibracao::EmAndamento) {
    DistanciasLaterais d;
    sensorSimulado.lerDistanciasLaterais(d);
    int16_t taxa = 0;
    imu.lerTaxaGiroZ(taxa);
    rotina.atualizar(d, true, taxa, true, 10);
  }

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::Sucesso), static_cast<int>(rotina.estado()));
  TEST_ASSERT_EQUAL_INT16(0, rotina.desviosTof().frenteMm);
  TEST_ASSERT_EQUAL_INT16(6, rotina.desviosTof().esquerdaMm);
  TEST_ASSERT_EQUAL_INT16(-4, rotina.desviosTof().direitaMm);
  TEST_ASSERT_EQUAL_INT16(12, rotina.viesGiroDecimosDps());
}

void test_rotina_informa_se_coube_no_orcamento_de_1_s() {
  RotinaCalibracao rotina(configRotinaRapida());
  SensorDistanciaSimulado sensorSimulado;
  ImuSimulado imu;
  sensorSimulado.definirLeituraFixa({54, 54, 54});

  rotina.executar(sensorSimulado, imu, 10);

  TEST_ASSERT_TRUE(rotina.dentroDoOrcamento());
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(ORCAMENTO_CALIBRACAO_MS, rotina.duracaoMedidaMs());
}

void test_rotina_com_passo_lento_estoura_o_orcamento_e_diz_que_nao_coube() {
  RotinaCalibracao rotina(configRotinaRapida());
  SensorDistanciaSimulado sensorSimulado;
  ImuSimulado imu;
  sensorSimulado.definirLeituraFixa({54, 54, 54});

  const EstadoRotinaCalibracao st = rotina.executar(sensorSimulado, imu, 200);  // 10 amostras levariam 2 s

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoRotinaCalibracao::TempoExcedido), static_cast<int>(st));
  TEST_ASSERT_FALSE(rotina.dentroDoOrcamento());
}

void test_nomes_dos_estados_da_rotina() {
  TEST_ASSERT_EQUAL_STRING("nao_iniciada", nomeEstadoRotina(EstadoRotinaCalibracao::NaoIniciada));
  TEST_ASSERT_EQUAL_STRING("em_andamento", nomeEstadoRotina(EstadoRotinaCalibracao::EmAndamento));
  TEST_ASSERT_EQUAL_STRING("sucesso", nomeEstadoRotina(EstadoRotinaCalibracao::Sucesso));
  TEST_ASSERT_EQUAL_STRING("recusada_por_movimento", nomeEstadoRotina(EstadoRotinaCalibracao::RecusadaPorMovimento));
  TEST_ASSERT_EQUAL_STRING("tempo_excedido", nomeEstadoRotina(EstadoRotinaCalibracao::TempoExcedido));
  TEST_ASSERT_EQUAL_STRING("falha_sensor", nomeEstadoRotina(EstadoRotinaCalibracao::FalhaSensor));
  TEST_ASSERT_EQUAL_STRING("desvio_fora_da_faixa", nomeEstadoRotina(EstadoRotinaCalibracao::DesvioForaDaFaixa));
}

int main() {
  UNITY_BEGIN();

  // CalibradorGiro
  RUN_TEST(test_calibrador_giro_inicial_nao_calibrado_e_vies_zero);
  RUN_TEST(test_calibrador_giro_calcula_vies_pela_media_arredondada);
  RUN_TEST(test_calibrador_giro_vies_negativo_arredondado);
  RUN_TEST(test_calibrador_giro_aplica_vies_nas_leituras_corrigir);
  RUN_TEST(test_calibrador_giro_recusa_se_robo_se_mover_variacao_excessiva);
  RUN_TEST(test_calibrador_giro_recusa_mantem_vies_anterior);
  RUN_TEST(test_calibrador_giro_amostra_fora_de_calibrando_e_ignorada);

  // RotinaCalibracao
  RUN_TEST(test_rotina_calibracao_sucesso_com_imu_simulado_e_sensor_simulado);
  RUN_TEST(test_rotina_calibracao_duracao_medida_dentro_do_orcamento_rnf08);
  RUN_TEST(test_rotina_calibracao_falha_tempo_excedido_quando_estoura_orcamento);
  RUN_TEST(test_rotina_calibracao_recusa_quando_robo_se_move_no_giro);
  RUN_TEST(test_rotina_calibracao_recusa_quando_robo_se_move_no_tof);
  RUN_TEST(test_rotina_calibracao_aborta_em_caso_de_falha_de_sensor);
  RUN_TEST(test_rotina_calibracao_iniciar_com_referencias_customizadas);
  RUN_TEST(test_rotina_descarta_a_calibracao_se_o_desvio_do_tof_passa_do_maximo);
  RUN_TEST(test_rotina_na_largada_calibra_so_os_lados_com_parede);
  RUN_TEST(test_rotina_informa_se_coube_no_orcamento_de_1_s);
  RUN_TEST(test_rotina_com_passo_lento_estoura_o_orcamento_e_diz_que_nao_coube);
  RUN_TEST(test_nomes_dos_estados_da_rotina);

  return UNITY_END();
}

