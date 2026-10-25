/// @file test_odometria.cpp
/// @brief Testes da odometria diferencial: orientação, posição na malha e giros.
/// Roda no computador: `pio test -e native -f test_odometria`.
///
/// Nos testes, 1 pulso = 250 µm (4 pulsos por mm) e 400 pulsos de diferença entre as rodas valem
/// 90°. Assim, uma célula de 18 cm são 720 pulsos por roda. Os valores são só para os testes e não
/// dependem dos parâmetros provisórios de `ConfiguracaoOdometria.h`.

#include <stdint.h>
#include <unity.h>

#include "EncoderSimulado.h"
#include "Odometria.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr int32_t PULSOS_POR_CELULA = 720;  // 180 mm a 250 µm por pulso
constexpr int32_t PULSOS_POR_90 = 400;      // diferença entre as rodas num giro de 90°

ConfiguracaoOdometria configuracaoDeTeste() {
  ConfiguracaoOdometria c;
  c.micrometrosPorPulso = 250;
  c.pulsosDiferenciaisPor90Graus = PULSOS_POR_90;
  c.tamanhoCelulaMicrometros = 180000;
  return c;
}

/// Robô virtual que anda sobre as contagens acumuladas das duas rodas.
struct Simulacao {
  Odometria odometria;
  int32_t esquerda = 0;
  int32_t direita = 0;

  explicit Simulacao(Pose inicial = Pose{{0, 0}, Direcao::Norte})
      : odometria(configuracaoDeTeste(), inicial) {}

  /// Anda em linha reta `celulas` células (negativo = marcha à ré), em passos de 10 pulsos.
  void andarCelulas(int celulas) {
    const int32_t total = (celulas < 0 ? -celulas : celulas) * PULSOS_POR_CELULA;
    const int32_t sinal = celulas < 0 ? -1 : 1;
    for (int32_t feito = 0; feito < total; feito += 10) {
      esquerda += sinal * 10;
      direita += sinal * 10;
      odometria.atualizar(esquerda, direita);
    }
  }

  /// Gira no lugar `quartos` vezes 90° (positivo = direita), em passos de 10 pulsos por roda.
  void girar(int quartos) {
    const int32_t total = (quartos < 0 ? -quartos : quartos) * (PULSOS_POR_90 / 2);
    const int32_t sinal = quartos < 0 ? -1 : 1;
    for (int32_t feito = 0; feito < total; feito += 10) {
      esquerda += sinal * 10;
      direita -= sinal * 10;
      odometria.atualizar(esquerda, direita);
    }
  }
};

void verificarPose(uint8_t linha, uint8_t coluna, Direcao direcao, const Pose& pose) {
  TEST_ASSERT_EQUAL_UINT8(linha, pose.celula.linha);
  TEST_ASSERT_EQUAL_UINT8(coluna, pose.celula.coluna);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(direcao), static_cast<uint8_t>(pose.direcao));
}

}  // namespace

// ---- Estado inicial

void test_comeca_na_celula_0_0_virada_para_o_norte() {
  Odometria odometria(configuracaoDeTeste());
  verificarPose(0, 0, Direcao::Norte, odometria.pose());
  TEST_ASSERT_TRUE(odometria.posicaoValida());
  TEST_ASSERT_EQUAL_INT32(0, odometria.posicaoNorteMicrometros());
  TEST_ASSERT_EQUAL_INT32(0, odometria.posicaoLesteMicrometros());
}

void test_configuracao_padrao_compila_e_comeca_na_partida() {
  Odometria odometria;
  verificarPose(0, 0, Direcao::Norte, odometria.pose());
}

void test_sem_movimento_a_pose_nao_muda() {
  Odometria odometria(configuracaoDeTeste());
  odometria.atualizar(0, 0);
  odometria.atualizar(0, 0);
  verificarPose(0, 0, Direcao::Norte, odometria.pose());
}

// ---- Avanço em linha reta

void test_uma_celula_para_o_norte_aumenta_a_linha() {
  Simulacao s;
  s.andarCelulas(1);
  verificarPose(1, 0, Direcao::Norte, s.odometria.pose());
  TEST_ASSERT_EQUAL_INT32(180000, s.odometria.posicaoNorteMicrometros());
}

void test_a_celula_muda_ao_cruzar_a_parede_a_meia_celula() {
  Simulacao s;
  for (int i = 0; i < 35; i++) {  // 350 pulsos = 87,5 mm: ainda antes da parede
    s.esquerda += 10;
    s.direita += 10;
    s.odometria.atualizar(s.esquerda, s.direita);
  }
  verificarPose(0, 0, Direcao::Norte, s.odometria.pose());
  s.esquerda += 20;  // 370 pulsos = 92,5 mm: passou da parede
  s.direita += 20;
  s.odometria.atualizar(s.esquerda, s.direita);
  verificarPose(1, 0, Direcao::Norte, s.odometria.pose());
}

void test_tres_celulas_para_o_norte() {
  Simulacao s;
  s.andarCelulas(3);
  verificarPose(3, 0, Direcao::Norte, s.odometria.pose());
}

void test_marcha_a_re_volta_a_celula_anterior() {
  Simulacao s(Pose{{2, 0}, Direcao::Norte});
  s.andarCelulas(-1);
  verificarPose(1, 0, Direcao::Norte, s.odometria.pose());
}

void test_atualizar_de_uma_vez_ou_em_passos_da_o_mesmo_resultado() {
  Odometria umaVez(configuracaoDeTeste());
  umaVez.atualizar(2 * PULSOS_POR_CELULA, 2 * PULSOS_POR_CELULA);
  Simulacao passos;
  passos.andarCelulas(2);
  verificarPose(2, 0, Direcao::Norte, umaVez.pose());
  verificarPose(2, 0, Direcao::Norte, passos.odometria.pose());
  TEST_ASSERT_EQUAL_INT32(umaVez.posicaoNorteMicrometros(), passos.odometria.posicaoNorteMicrometros());
}

// ---- Giros

void test_giro_de_90_graus_para_a_direita_vai_para_leste() {
  Simulacao s;
  s.girar(1);
  verificarPose(0, 0, Direcao::Leste, s.odometria.pose());
}

void test_giro_de_90_graus_para_a_esquerda_vai_para_oeste() {
  Simulacao s(Pose{{1, 1}, Direcao::Norte});
  s.girar(-1);
  verificarPose(1, 1, Direcao::Oeste, s.odometria.pose());
}

void test_giro_de_180_graus_vira_para_o_sul_nos_dois_sentidos() {
  Simulacao horario;
  horario.girar(2);
  verificarPose(0, 0, Direcao::Sul, horario.odometria.pose());
  Simulacao antiHorario;
  antiHorario.girar(-2);
  verificarPose(0, 0, Direcao::Sul, antiHorario.odometria.pose());
}

void test_quatro_giros_para_a_direita_voltam_ao_norte() {
  Simulacao s;
  s.girar(1);
  verificarPose(0, 0, Direcao::Leste, s.odometria.pose());
  s.girar(1);
  verificarPose(0, 0, Direcao::Sul, s.odometria.pose());
  s.girar(1);
  verificarPose(0, 0, Direcao::Oeste, s.odometria.pose());
  s.girar(1);
  verificarPose(0, 0, Direcao::Norte, s.odometria.pose());
}

void test_giro_de_270_graus_para_a_direita_equivale_a_90_para_a_esquerda() {
  Simulacao s(Pose{{1, 1}, Direcao::Norte});
  s.girar(3);
  verificarPose(1, 1, Direcao::Oeste, s.odometria.pose());
}

void test_giro_no_lugar_nao_muda_a_celula() {
  Simulacao s(Pose{{1, 1}, Direcao::Norte});
  s.girar(1);
  s.girar(1);
  s.girar(-1);
  verificarPose(1, 1, Direcao::Leste, s.odometria.pose());
  TEST_ASSERT_EQUAL_INT32(180000, s.odometria.posicaoNorteMicrometros());
  TEST_ASSERT_EQUAL_INT32(180000, s.odometria.posicaoLesteMicrometros());
}

void test_o_giro_so_passa_para_a_direcao_seguinte_depois_de_45_graus() {
  Odometria odometria(configuracaoDeTeste());
  // 44° (diferença de 195 pulsos): ainda Norte.
  odometria.atualizar(97, -98);
  verificarPose(0, 0, Direcao::Norte, odometria.pose());
  // Passou de 45° (diferença de 201 pulsos): vira para Leste.
  odometria.atualizar(101, -100);
  verificarPose(0, 0, Direcao::Leste, odometria.pose());
}

void test_uma_roda_um_pouco_mais_rapida_nao_muda_a_direcao_em_linha_reta() {
  Odometria odometria(configuracaoDeTeste());
  odometria.atualizar(PULSOS_POR_CELULA, PULSOS_POR_CELULA - 40);  // diferença de 40 pulsos (~9°)
  verificarPose(1, 0, Direcao::Norte, odometria.pose());
}

// ---- Percursos

void test_percurso_em_L_dois_para_o_norte_e_dois_para_o_leste() {
  Simulacao s;
  s.andarCelulas(2);
  s.girar(1);
  s.andarCelulas(2);
  verificarPose(2, 2, Direcao::Leste, s.odometria.pose());
}

void test_percurso_em_U_e_volta_a_linha_da_partida() {
  Simulacao s;
  s.andarCelulas(1);
  s.girar(1);
  s.andarCelulas(1);
  s.girar(1);
  s.andarCelulas(1);
  verificarPose(0, 1, Direcao::Sul, s.odometria.pose());
}

void test_retorno_a_partida_com_meia_volta() {
  Simulacao s;
  s.andarCelulas(3);
  s.girar(2);
  s.andarCelulas(3);
  verificarPose(0, 0, Direcao::Sul, s.odometria.pose());
}

void test_serpentina_pelo_labirinto_12x4() {
  Simulacao s;
  s.girar(1);          // Leste
  s.andarCelulas(11);  // coluna 11, linha 0
  s.girar(-1);         // Norte
  s.andarCelulas(1);   // linha 1
  s.girar(-1);         // Oeste
  s.andarCelulas(11);  // coluna 0
  s.girar(1);          // Norte
  s.andarCelulas(1);   // linha 2
  s.girar(1);          // Leste
  s.andarCelulas(11);  // coluna 11
  s.girar(-1);         // Norte
  s.andarCelulas(1);   // linha 3
  s.girar(-1);         // Oeste
  s.andarCelulas(11);  // coluna 0
  verificarPose(3, 0, Direcao::Oeste, s.odometria.pose());
  TEST_ASSERT_TRUE(s.odometria.posicaoValida());
}

void test_erro_pequeno_em_cada_celula_nao_atrapalha_um_corredor_de_11_celulas() {
  // A roda direita anda 1% a menos em cada passo: o robô desvia um pouco, mas a direção e a célula
  // continuam as certas.
  Odometria odometria(configuracaoDeTeste(), Pose{{0, 0}, Direcao::Leste});
  int32_t esquerda = 0;
  int32_t direita = 0;
  for (int passo = 0; passo < 11 * PULSOS_POR_CELULA / 10; passo++) {
    esquerda += 10;
    direita += 10 - (passo % 100 == 0 ? 1 : 0);
    odometria.atualizar(esquerda, direita);
  }
  verificarPose(0, 11, Direcao::Leste, odometria.pose());
}

// ---- Reinício e leitura dos encoders

void test_reiniciar_define_a_pose_e_usa_as_contagens_como_referencia() {
  Odometria odometria(configuracaoDeTeste());
  odometria.atualizar(PULSOS_POR_CELULA, PULSOS_POR_CELULA);
  verificarPose(1, 0, Direcao::Norte, odometria.pose());

  // Os encoders não foram zerados: a referência é a contagem atual.
  odometria.reiniciar(Pose{{3, 2}, Direcao::Sul}, 5000, 5000);
  verificarPose(3, 2, Direcao::Sul, odometria.pose());
  odometria.atualizar(5000 + PULSOS_POR_CELULA, 5000 + PULSOS_POR_CELULA);
  verificarPose(2, 2, Direcao::Sul, odometria.pose());
}

void test_atualizar_le_os_dois_encoders() {
  EncoderSimulado esquerdo;
  EncoderSimulado direito;
  Odometria odometria(configuracaoDeTeste());
  esquerdo.adicionarPulsos(PULSOS_POR_CELULA);
  direito.adicionarPulsos(PULSOS_POR_CELULA);
  odometria.atualizar(esquerdo, direito);
  verificarPose(1, 0, Direcao::Norte, odometria.pose());
}

void test_contagens_que_voltam_a_zero_depois_de_zerar_exigem_reiniciar() {
  EncoderSimulado esquerdo;
  EncoderSimulado direito;
  Odometria odometria(configuracaoDeTeste());
  esquerdo.adicionarPulsos(PULSOS_POR_CELULA);
  direito.adicionarPulsos(PULSOS_POR_CELULA);
  odometria.atualizar(esquerdo, direito);
  esquerdo.zerar();
  direito.zerar();
  odometria.reiniciar(odometria.pose(), 0, 0);
  esquerdo.adicionarPulsos(PULSOS_POR_CELULA);
  direito.adicionarPulsos(PULSOS_POR_CELULA);
  odometria.atualizar(esquerdo, direito);
  verificarPose(2, 0, Direcao::Norte, odometria.pose());
}

void test_definir_direcao_mantem_a_posicao() {
  Simulacao sim;
  sim.andarCelulas(1);
  // As rodas mostram só 22,5° (patinaram), mas o giroscópio diz que foram 90° para a direita (#184).
  for (int32_t feito = 0; feito < PULSOS_POR_90 / 8; feito += 10) {
    sim.esquerda += 10;
    sim.direita -= 10;
    sim.odometria.atualizar(sim.esquerda, sim.direita);
  }
  verificarPose(1, 0, Direcao::Norte, sim.odometria.pose());
  const int32_t norteAntes = sim.odometria.posicaoNorteMicrometros();

  sim.odometria.definirDirecao(Direcao::Leste);
  verificarPose(1, 0, Direcao::Leste, sim.odometria.pose());
  TEST_ASSERT_EQUAL_INT32(norteAntes, sim.odometria.posicaoNorteMicrometros());

  // O avanço seguinte já vai para o Leste.
  sim.andarCelulas(1);
  verificarPose(1, 1, Direcao::Leste, sim.odometria.pose());
}

void test_definir_direcao_zera_o_giro_acumulado() {
  Simulacao sim;
  // Diferença entre as rodas perto de virar a direção (40,5°), sem chegar lá.
  for (int32_t feito = 0; feito < 90; feito += 10) {
    sim.esquerda += 10;
    sim.direita -= 10;
    sim.odometria.atualizar(sim.esquerda, sim.direita);
  }
  verificarPose(0, 0, Direcao::Norte, sim.odometria.pose());
  sim.odometria.definirDirecao(Direcao::Norte);
  // Sem zerar, mais 18° levariam a Leste; zerado, a direção continua Norte.
  for (int32_t feito = 0; feito < 40; feito += 10) {
    sim.esquerda += 10;
    sim.direita -= 10;
    sim.odometria.atualizar(sim.esquerda, sim.direita);
  }
  verificarPose(0, 0, Direcao::Norte, sim.odometria.pose());
  // Um giro de 90° depois dele conta a partir da direção definida.
  sim.girar(1);
  verificarPose(0, 0, Direcao::Leste, sim.odometria.pose());
}

void test_ajustar_posicao_no_referencial_do_robo_nas_quatro_direcoes() {
  // Frente +10 mm e direita +4 mm, em cada direção.
  struct Caso {
    Direcao direcao;
    int32_t norteUm;
    int32_t lesteUm;
  };
  const Caso casos[] = {{Direcao::Norte, 10000, 4000},
                        {Direcao::Leste, -4000, 10000},
                        {Direcao::Sul, -10000, -4000},
                        {Direcao::Oeste, 4000, -10000}};
  for (const Caso& c : casos) {
    Odometria odometria(configuracaoDeTeste(), Pose{{1, 1}, c.direcao});
    odometria.ajustarPosicao(10000, 4000);
    TEST_ASSERT_EQUAL_INT32(180000 + c.norteUm, odometria.posicaoNorteMicrometros());
    TEST_ASSERT_EQUAL_INT32(180000 + c.lesteUm, odometria.posicaoLesteMicrometros());
    verificarPose(1, 1, c.direcao, odometria.pose());
  }
}

void test_ajustar_posicao_muda_a_celula_e_nao_mexe_nas_contagens() {
  Simulacao sim;
  sim.andarCelulas(1);
  // Do centro de (1, 0), 100 mm à frente: passa a fronteira e vai para (2, 0).
  sim.odometria.ajustarPosicao(100000, 0);
  verificarPose(2, 0, Direcao::Norte, sim.odometria.pose());
  // As contagens seguem de onde estavam: o próximo avanço soma a partir da posição corrigida.
  sim.andarCelulas(1);
  TEST_ASSERT_EQUAL_INT32(2 * 180000 + 100000, sim.odometria.posicaoNorteMicrometros());
  // Fora da malha, a posição fica inválida, como no avanço.
  sim.odometria.ajustarPosicao(0, -100000);
  TEST_ASSERT_FALSE(sim.odometria.posicaoValida());
}

// ---- Fora do alcance da malha

void test_posicao_negativa_e_invalida_e_a_celula_fica_em_zero() {
  Simulacao s;
  s.andarCelulas(-1);
  TEST_ASSERT_FALSE(s.odometria.posicaoValida());
  verificarPose(0, 0, Direcao::Norte, s.odometria.pose());
  TEST_ASSERT_LESS_THAN_INT32(0, s.odometria.posicaoNorteMicrometros());
}

void test_posicao_acima_de_255_celulas_e_invalida_e_satura() {
  Odometria odometria(configuracaoDeTeste(), Pose{{255, 0}, Direcao::Norte});
  odometria.atualizar(PULSOS_POR_CELULA, PULSOS_POR_CELULA);
  TEST_ASSERT_FALSE(odometria.posicaoValida());
  verificarPose(255, 0, Direcao::Norte, odometria.pose());
}

void test_volta_para_dentro_da_malha_volta_a_ser_valida() {
  Simulacao s;
  s.andarCelulas(-1);
  TEST_ASSERT_FALSE(s.odometria.posicaoValida());
  s.andarCelulas(2);
  TEST_ASSERT_TRUE(s.odometria.posicaoValida());
  verificarPose(1, 0, Direcao::Norte, s.odometria.pose());
}

void test_contagem_muito_grande_nao_estoura() {
  Odometria odometria(configuracaoDeTeste());
  odometria.atualizar(INT32_MAX, INT32_MAX);
  odometria.atualizar(INT32_MIN, INT32_MIN);
  TEST_ASSERT_FALSE(odometria.posicaoValida());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_comeca_na_celula_0_0_virada_para_o_norte);
  RUN_TEST(test_configuracao_padrao_compila_e_comeca_na_partida);
  RUN_TEST(test_sem_movimento_a_pose_nao_muda);
  RUN_TEST(test_uma_celula_para_o_norte_aumenta_a_linha);
  RUN_TEST(test_a_celula_muda_ao_cruzar_a_parede_a_meia_celula);
  RUN_TEST(test_tres_celulas_para_o_norte);
  RUN_TEST(test_marcha_a_re_volta_a_celula_anterior);
  RUN_TEST(test_atualizar_de_uma_vez_ou_em_passos_da_o_mesmo_resultado);
  RUN_TEST(test_giro_de_90_graus_para_a_direita_vai_para_leste);
  RUN_TEST(test_giro_de_90_graus_para_a_esquerda_vai_para_oeste);
  RUN_TEST(test_giro_de_180_graus_vira_para_o_sul_nos_dois_sentidos);
  RUN_TEST(test_quatro_giros_para_a_direita_voltam_ao_norte);
  RUN_TEST(test_giro_de_270_graus_para_a_direita_equivale_a_90_para_a_esquerda);
  RUN_TEST(test_giro_no_lugar_nao_muda_a_celula);
  RUN_TEST(test_o_giro_so_passa_para_a_direcao_seguinte_depois_de_45_graus);
  RUN_TEST(test_uma_roda_um_pouco_mais_rapida_nao_muda_a_direcao_em_linha_reta);
  RUN_TEST(test_percurso_em_L_dois_para_o_norte_e_dois_para_o_leste);
  RUN_TEST(test_percurso_em_U_e_volta_a_linha_da_partida);
  RUN_TEST(test_retorno_a_partida_com_meia_volta);
  RUN_TEST(test_serpentina_pelo_labirinto_12x4);
  RUN_TEST(test_erro_pequeno_em_cada_celula_nao_atrapalha_um_corredor_de_11_celulas);
  RUN_TEST(test_reiniciar_define_a_pose_e_usa_as_contagens_como_referencia);
  RUN_TEST(test_atualizar_le_os_dois_encoders);
  RUN_TEST(test_contagens_que_voltam_a_zero_depois_de_zerar_exigem_reiniciar);
  RUN_TEST(test_definir_direcao_mantem_a_posicao);
  RUN_TEST(test_definir_direcao_zera_o_giro_acumulado);
  RUN_TEST(test_ajustar_posicao_no_referencial_do_robo_nas_quatro_direcoes);
  RUN_TEST(test_ajustar_posicao_muda_a_celula_e_nao_mexe_nas_contagens);
  RUN_TEST(test_posicao_negativa_e_invalida_e_a_celula_fica_em_zero);
  RUN_TEST(test_posicao_acima_de_255_celulas_e_invalida_e_satura);
  RUN_TEST(test_volta_para_dentro_da_malha_volta_a_ser_valida);
  RUN_TEST(test_contagem_muito_grande_nao_estoura);
  return UNITY_END();
}
