/// @file test_colisao.cpp
/// @brief Testes unitários da detecção de colisão (#192): a conversão do acelerômetro, o
/// `ImuSimulado` com aceleração, o `RegistradorPotencia` e o `DetectorColisao` (impacto pela IMU e
/// frente parada pelo ToF).
///
/// Roda no computador: `pio test -e native -f test_colisao`.

#include <unity.h>

#include "ConfiguracaoColisao.h"
#include "ConversorAceleracao.h"
#include "DetectorColisao.h"
#include "ImuSimulado.h"
#include "MotoresSimulados.h"
#include "RegistradorPotencia.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

/// Um ciclo de 10 ms com o robô em movimento e a aceleração dada.
EntradaDeteccao andando(int16_t axMg, int16_t ayMg) {
  EntradaDeteccao e;
  e.ativo = true;
  e.aceleracaoValida = true;
  e.axMg = axMg;
  e.ayMg = ayMg;
  e.dtMs = 10;
  return e;
}

/// Um ciclo com o robô parado, aprendendo o repouso.
EntradaDeteccao parado(int16_t axMg, int16_t ayMg) {
  EntradaDeteccao e = andando(axMg, ayMg);
  e.ativo = false;
  return e;
}

/// Ciclo andando em linha reta contra algo: potência alta, ToF frontal e giroscópio parados.
EntradaDeteccao preso(uint16_t frenteMm = 100, int16_t taxaGiro = 0) {
  EntradaDeteccao e;
  e.ativo = true;
  e.aceleracaoValida = true;  // acelerômetro calmo
  e.potencia = {400, 400};
  e.medidas = MedidasProtecao{true, frenteMm, taxaGiro};
  e.dtMs = 10;
  return e;
}

/// Alimenta o detector com `ms` de ciclos iguais e devolve o primeiro tipo que não foi `Nenhuma`.
TipoColisao rodar(DetectorColisao& detector, const EntradaDeteccao& entrada, uint32_t ms) {
  for (uint32_t t = 0; t < ms; t += entrada.dtMs) {
    const TipoColisao tipo = detector.atualizar(entrada);
    if (tipo != TipoColisao::Nenhuma) {
      return tipo;
    }
  }
  return TipoColisao::Nenhuma;
}

}  // namespace

// ---- Conversão do acelerômetro

void test_converte_o_bruto_do_acelerometro_para_mg_em_cada_faixa() {
  TEST_ASSERT_EQUAL_INT16(1000, converterBrutoParaMg(8192, FaixaAcelerometro::G4));
  TEST_ASSERT_EQUAL_INT16(-1000, converterBrutoParaMg(-8192, FaixaAcelerometro::G4));
  TEST_ASSERT_EQUAL_INT16(0, converterBrutoParaMg(0, FaixaAcelerometro::G4));
  TEST_ASSERT_EQUAL_INT16(500, converterBrutoParaMg(4096, FaixaAcelerometro::G4));
  TEST_ASSERT_EQUAL_INT16(1000, converterBrutoParaMg(16384, FaixaAcelerometro::G2));
  TEST_ASSERT_EQUAL_INT16(1000, converterBrutoParaMg(4096, FaixaAcelerometro::G8));
  TEST_ASSERT_EQUAL_INT16(1000, converterBrutoParaMg(2048, FaixaAcelerometro::G16));
  // O fim da escala: 32767 em ±4 g são 4 g.
  TEST_ASSERT_EQUAL_INT16(4000, converterBrutoParaMg(32767, FaixaAcelerometro::G4));
}

void test_a_conversao_do_acelerometro_arredonda_para_o_mais_proximo() {
  // 1 LSB em ±4 g = 0,122 mg: arredonda para zero; 5 LSB = 0,61 mg: arredonda para 1.
  TEST_ASSERT_EQUAL_INT16(0, converterBrutoParaMg(1, FaixaAcelerometro::G4));
  TEST_ASSERT_EQUAL_INT16(1, converterBrutoParaMg(5, FaixaAcelerometro::G4));
  TEST_ASSERT_EQUAL_INT16(-1, converterBrutoParaMg(-5, FaixaAcelerometro::G4));
}

void test_junta_os_bytes_do_acelerometro_em_complemento_de_dois() {
  TEST_ASSERT_EQUAL_INT16(0x2000, juntarBytesAceleracao(0x20, 0x00));
  TEST_ASSERT_EQUAL_INT16(-1, juntarBytesAceleracao(0xFF, 0xFF));
  TEST_ASSERT_EQUAL_INT16(-8192, juntarBytesAceleracao(0xE0, 0x00));
}

void test_registrador_de_faixa_do_acelerometro() {
  TEST_ASSERT_EQUAL_UINT8(0x00, valorAccelConfig(FaixaAcelerometro::G2));
  TEST_ASSERT_EQUAL_UINT8(0x08, valorAccelConfig(FaixaAcelerometro::G4));
  TEST_ASSERT_EQUAL_UINT8(0x10, valorAccelConfig(FaixaAcelerometro::G8));
  TEST_ASSERT_EQUAL_UINT8(0x18, valorAccelConfig(FaixaAcelerometro::G16));
  TEST_ASSERT_EQUAL_UINT16(8192, sensibilidadeAcelerometroLsbPorG(FAIXA_ACELEROMETRO));
}

// ---- IMU simulada com aceleração

void test_imu_simulada_devolve_a_aceleracao_definida() {
  ImuSimulado imu;
  IAcelerometro& acelerometro = imu;
  int16_t ax = 77;
  int16_t ay = 88;
  TEST_ASSERT_TRUE(acelerometro.lerAceleracaoPlano(ax, ay));
  TEST_ASSERT_EQUAL_INT16(0, ax);
  TEST_ASSERT_EQUAL_INT16(0, ay);
  imu.definirAceleracao(-1200, 350);
  TEST_ASSERT_TRUE(acelerometro.lerAceleracaoPlano(ax, ay));
  TEST_ASSERT_EQUAL_INT16(-1200, ax);
  TEST_ASSERT_EQUAL_INT16(350, ay);
}

void test_imu_simulada_com_falha_nao_mexe_na_aceleracao() {
  ImuSimulado imu;
  imu.definirAceleracao(10, 20);
  imu.simularFalha(true);
  int16_t ax = 1;
  int16_t ay = 2;
  TEST_ASSERT_FALSE(imu.lerAceleracaoPlano(ax, ay));
  TEST_ASSERT_EQUAL_INT16(1, ax);
  TEST_ASSERT_EQUAL_INT16(2, ay);
}

// ---- Registrador da potência comandada

void test_registrador_repassa_e_guarda_a_ultima_potencia() {
  MotoresSimulados motores;
  RegistradorPotencia registrador(motores);
  TEST_ASSERT_EQUAL_INT16(0, registrador.ultimaPotencia().esquerda);

  registrador.definirPotencia({300, -200});
  TEST_ASSERT_EQUAL_INT16(300, registrador.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(-200, registrador.ultimaPotencia().direita);
  TEST_ASSERT_EQUAL_INT16(300, motores.ultimaPotencia().esquerda);

  registrador.parar();
  TEST_ASSERT_EQUAL_INT16(0, registrador.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(0, registrador.ultimaPotencia().direita);
  TEST_ASSERT_TRUE(motores.freados());
}

void test_registrador_limita_a_potencia_como_o_motor() {
  MotoresSimulados motores;
  RegistradorPotencia registrador(motores);
  registrador.definirPotencia({5000, -5000});
  TEST_ASSERT_EQUAL_INT16(POTENCIA_MAXIMA, registrador.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(-POTENCIA_MAXIMA, registrador.ultimaPotencia().direita);
}

// ---- Impacto pelo acelerômetro

void test_leituras_calmas_andando_nao_sao_colisao() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  // Partida e frenagem do robô: até 0,4 g no plano.
  const int16_t calmas[][2] = {{400, 0}, {-350, 100}, {0, 300}, {250, -250}, {-400, 0}, {100, 100}};
  for (const auto& a : calmas) {
    TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(a[0], a[1])));
  }
  TEST_ASSERT_EQUAL_UINT32(0, detector.colisoes());
}

void test_pico_de_aceleracao_e_impacto() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(200, 0)));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(-2500, 100)));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.ultima());
  TEST_ASSERT_EQUAL_UINT32(1, detector.colisoes());
  TEST_ASSERT_TRUE(detector.desvioMg() >= 2500);
}

void test_o_modulo_do_plano_conta_os_dois_eixos() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  // 1700 em cada eixo: módulo 2404 mg, acima do limiar de 2200, embora cada eixo esteja abaixo.
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(1700, 1700)));
  TEST_ASSERT_EQUAL_UINT32(2404, detector.desvioMg());
}

void test_o_desvio_e_o_modulo_exato() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  detector.atualizar(andando(3000, 4000));
  TEST_ASSERT_EQUAL_UINT32(5000, detector.desvioMg());
}

void test_abaixo_do_limiar_nao_e_impacto_e_no_limiar_e() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(LIMIAR_IMPACTO_MG - 1, 0)));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(LIMIAR_IMPACTO_MG, 0)));
}

void test_o_zero_e_a_inclinacao_do_sensor_nao_contam() {
  DetectorColisao detector;
  // Sensor inclinado: parado, lê 300 mg em X e −200 em Y. É a referência de repouso.
  for (int i = 0; i < 100; i++) {
    detector.atualizar(parado(300, -200));
  }
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(300, -200)));
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(700, -200)));
  // O mesmo valor absoluto de uma batida, mas a partir do repouso inclinado.
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(-2100, -200)));
}

void test_a_referencia_acompanha_a_inclinacao_devagar() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  // O sensor vai inclinando aos poucos (rampa): a referência acompanha e não dispara.
  for (int mg = 0; mg <= 300; mg += 1) {
    TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(static_cast<int16_t>(mg), 0)));
  }
  TEST_ASSERT_EQUAL_UINT32(0, detector.colisoes());
}

void test_a_referencia_e_media_lenta_e_nao_a_ultima_leitura_calma() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  // Uma leitura calma (400 mg) move a referência só 1/16 do caminho (25 mg), e não até 400 mg.
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(400, 0)));
  // Desvio de 2475 mg passa do limiar de 2200; se a referência pulasse para 400 mg seria 2100 mg e passaria batido.
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(2500, 0)));
}

void test_um_impacto_nao_vira_o_novo_repouso() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(2600, 0)));
  // Depois do aviso (o ciclo está recuperando), o robô para e volta a andar com o mesmo repouso:
  // o impacto não deslocou a referência.
  detector.atualizar(parado(0, 0));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(2300, 0)));
}

void test_confirmacao_exige_leituras_seguidas() {
  ConfiguracaoColisao config;
  config.amostrasConfirmacao = 2;
  DetectorColisao detector(config);
  detector.atualizar(parado(0, 0));
  // Um pico isolado não basta...
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(2600, 0)));
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(0, 0)));
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(2600, 0)));
  // ...dois seguidos, sim.
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(2600, 0)));
}

void test_zero_amostras_de_confirmacao_vale_uma() {
  ConfiguracaoColisao config;
  config.amostrasConfirmacao = 0;
  DetectorColisao detector(config);
  detector.atualizar(parado(0, 0));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(2600, 0)));
}

void test_limiar_configuravel() {
  ConfiguracaoColisao config;
  config.limiarImpactoMg = 400;
  DetectorColisao detector(config);
  detector.atualizar(parado(0, 0));
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(399, 0)));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(400, 0)));
}

void test_parado_ou_sem_permissao_um_pico_nao_e_colisao() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  // O robô erguido ou empurrado à mão: nada de colisão.
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(parado(3000, 3000)));
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(parado(-3000, 0)));
  TEST_ASSERT_EQUAL_UINT32(0, detector.colisoes());
}

void test_o_extremo_da_escala_nao_estoura() {
  DetectorColisao detector;
  detector.atualizar(parado(INT16_MAX, INT16_MAX));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(INT16_MIN, INT16_MIN)));
  TEST_ASSERT_TRUE(detector.desvioMg() > 90000);
}

void test_cada_colisao_e_avisada_uma_vez_ate_o_robo_parar() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(2600, 0)));
  // O pico continua por alguns ciclos: não avisa de novo.
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(2700, 0)));
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(2600, 0)));
  TEST_ASSERT_EQUAL_UINT32(1, detector.colisoes());
  // Parou (a recuperação): a próxima batida é uma colisão nova.
  detector.atualizar(parado(0, 0));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(2600, 0)));
  TEST_ASSERT_EQUAL_UINT32(2, detector.colisoes());
}

void test_reiniciar_permite_avisar_de_novo() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(2600, 0)));
  detector.reiniciar();
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(2600, 0)));
}

void test_sem_acelerometro_valido_nao_ha_impacto() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  EntradaDeteccao e = andando(9000, 9000);
  e.aceleracaoValida = false;
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(e));
  TEST_ASSERT_EQUAL_UINT32(0, detector.colisoes());
}

void test_a_primeira_leitura_andando_vira_a_referencia() {
  DetectorColisao detector;
  // Sem nenhuma leitura parada antes: a primeira andando é a referência, e não dispara.
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(andando(2500, 0)));
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(andando(-500, 0)));
}

// ---- Frente parada pelo ToF

void test_potencia_alta_sem_nenhum_movimento_e_frente_parada() {
  DetectorColisao detector;
  TEST_ASSERT_EQUAL(TipoColisao::FrenteParada, rodar(detector, preso(), 600));
  TEST_ASSERT_EQUAL(TipoColisao::FrenteParada, detector.ultima());
  TEST_ASSERT_EQUAL_UINT32(1, detector.colisoes());
}

void test_so_depois_do_tempo_limite() {
  DetectorColisao detector;
  // 480 ms ainda não é o limite de 500 ms.
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, rodar(detector, preso(), 480));
  TEST_ASSERT_EQUAL(TipoColisao::FrenteParada, rodar(detector, preso(), 40));
}

void test_frente_que_varia_nao_e_parada() {
  DetectorColisao detector;
  EntradaDeteccao e = preso();
  uint16_t frente = 600;
  for (int i = 0; i < 100; i++) {
    e.medidas.frenteMm = frente;
    TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, detector.atualizar(e));
    frente = static_cast<uint16_t>(frente - 2);  // anda 2 mm a cada 10 ms
  }
}

void test_girando_no_lugar_nao_e_parada() {
  DetectorColisao detector;
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, rodar(detector, preso(100, 900), 1000));
}

void test_potencia_baixa_nao_e_parada() {
  DetectorColisao detector;
  EntradaDeteccao e = preso();
  e.potencia = {100, 100};
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, rodar(detector, e, 1000));
}

void test_sem_leitura_frontal_valida_nao_conclui_nada() {
  DetectorColisao detector;
  EntradaDeteccao e = preso();
  e.medidas.frenteValida = false;
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, rodar(detector, e, 1000));
}

void test_frente_alem_do_alcance_nao_e_parada() {
  // Corredor mais longo que o alcance do ToF: a leitura fica no máximo (1200 mm) enquanto o robô
  // anda. Não pode virar "frente parada".
  DetectorColisao detector;
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, rodar(detector, preso(1200), 2000));
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, rodar(detector, preso(8190), 2000));
  TEST_ASSERT_EQUAL_UINT32(0, detector.colisoes());
}

void test_parar_de_andar_zera_a_janela_da_frente_parada() {
  DetectorColisao detector;
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, rodar(detector, preso(), 400));
  detector.atualizar(parado(0, 0));
  // A janela recomeça: mais 400 ms presos ainda não bastam.
  TEST_ASSERT_EQUAL(TipoColisao::Nenhuma, rodar(detector, preso(), 400));
  TEST_ASSERT_EQUAL(TipoColisao::FrenteParada, rodar(detector, preso(), 200));
}

void test_o_impacto_vem_antes_da_frente_parada_no_mesmo_ciclo() {
  DetectorColisao detector;
  detector.atualizar(parado(0, 0));
  rodar(detector, preso(), 480);
  EntradaDeteccao e = preso();
  e.axMg = 2500;  // o tempo limite fecha neste ciclo, junto com um pico
  e.dtMs = 40;
  TEST_ASSERT_EQUAL(TipoColisao::Impacto, detector.atualizar(e));
  TEST_ASSERT_EQUAL_UINT32(1, detector.colisoes());
}

void test_nomes_dos_tipos_de_colisao() {
  TEST_ASSERT_EQUAL_STRING("nenhuma", nomeDoTipoColisao(TipoColisao::Nenhuma));
  TEST_ASSERT_EQUAL_STRING("impacto", nomeDoTipoColisao(TipoColisao::Impacto));
  TEST_ASSERT_EQUAL_STRING("frente_parada", nomeDoTipoColisao(TipoColisao::FrenteParada));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_converte_o_bruto_do_acelerometro_para_mg_em_cada_faixa);
  RUN_TEST(test_a_conversao_do_acelerometro_arredonda_para_o_mais_proximo);
  RUN_TEST(test_junta_os_bytes_do_acelerometro_em_complemento_de_dois);
  RUN_TEST(test_registrador_de_faixa_do_acelerometro);
  RUN_TEST(test_imu_simulada_devolve_a_aceleracao_definida);
  RUN_TEST(test_imu_simulada_com_falha_nao_mexe_na_aceleracao);
  RUN_TEST(test_registrador_repassa_e_guarda_a_ultima_potencia);
  RUN_TEST(test_registrador_limita_a_potencia_como_o_motor);
  RUN_TEST(test_leituras_calmas_andando_nao_sao_colisao);
  RUN_TEST(test_pico_de_aceleracao_e_impacto);
  RUN_TEST(test_o_modulo_do_plano_conta_os_dois_eixos);
  RUN_TEST(test_o_desvio_e_o_modulo_exato);
  RUN_TEST(test_abaixo_do_limiar_nao_e_impacto_e_no_limiar_e);
  RUN_TEST(test_o_zero_e_a_inclinacao_do_sensor_nao_contam);
  RUN_TEST(test_a_referencia_acompanha_a_inclinacao_devagar);
  RUN_TEST(test_a_referencia_e_media_lenta_e_nao_a_ultima_leitura_calma);
  RUN_TEST(test_um_impacto_nao_vira_o_novo_repouso);
  RUN_TEST(test_confirmacao_exige_leituras_seguidas);
  RUN_TEST(test_zero_amostras_de_confirmacao_vale_uma);
  RUN_TEST(test_limiar_configuravel);
  RUN_TEST(test_parado_ou_sem_permissao_um_pico_nao_e_colisao);
  RUN_TEST(test_o_extremo_da_escala_nao_estoura);
  RUN_TEST(test_cada_colisao_e_avisada_uma_vez_ate_o_robo_parar);
  RUN_TEST(test_reiniciar_permite_avisar_de_novo);
  RUN_TEST(test_sem_acelerometro_valido_nao_ha_impacto);
  RUN_TEST(test_a_primeira_leitura_andando_vira_a_referencia);
  RUN_TEST(test_potencia_alta_sem_nenhum_movimento_e_frente_parada);
  RUN_TEST(test_so_depois_do_tempo_limite);
  RUN_TEST(test_frente_que_varia_nao_e_parada);
  RUN_TEST(test_girando_no_lugar_nao_e_parada);
  RUN_TEST(test_potencia_baixa_nao_e_parada);
  RUN_TEST(test_sem_leitura_frontal_valida_nao_conclui_nada);
  RUN_TEST(test_frente_alem_do_alcance_nao_e_parada);
  RUN_TEST(test_parar_de_andar_zera_a_janela_da_frente_parada);
  RUN_TEST(test_o_impacto_vem_antes_da_frente_parada_no_mesmo_ciclo);
  RUN_TEST(test_nomes_dos_tipos_de_colisao);
  return UNITY_END();
}
