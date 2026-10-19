/// @file test_encoders.cpp
/// @brief Testes da leitura dos encoders: decodificação em quadratura, conversão de pulsos em
/// distância, medidor das duas rodas e o `EncoderSimulado`.
/// Roda no computador: `pio test -e native -f test_encoders`.

#include <stdint.h>
#include <unity.h>

#include "ConversorDeslocamento.h"
#include "DecodificadorQuadratura.h"
#include "EncoderSimulado.h"
#include "MedidorDeslocamento.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

/// Estados dos canais (A, B) do sentido positivo: 00 → 10 → 11 → 01 → 00.
struct Nivel {
  bool a;
  bool b;
};
const Nivel SENTIDO_POSITIVO[4] = {{true, false}, {true, true}, {false, true}, {false, false}};
const Nivel SENTIDO_NEGATIVO[4] = {{false, true}, {true, true}, {true, false}, {false, false}};

/// Aplica `ciclos` ciclos completos (4 pulsos cada) a partir do estado 00.
void aplicarCiclos(DecodificadorQuadratura& decodificador, const Nivel* sequencia, int ciclos) {
  for (int c = 0; c < ciclos; c++) {
    for (int i = 0; i < 4; i++) {
      decodificador.processar(sequencia[i].a, sequencia[i].b);
    }
  }
}

}  // namespace

// ---- Decodificação em quadratura

void test_cada_mudanca_de_nivel_conta_um_pulso_no_sentido_positivo() {
  DecodificadorQuadratura d;
  d.iniciar(false, false);
  TEST_ASSERT_EQUAL_INT8(1, d.processar(true, false));
  TEST_ASSERT_EQUAL_INT8(1, d.processar(true, true));
  TEST_ASSERT_EQUAL_INT8(1, d.processar(false, true));
  TEST_ASSERT_EQUAL_INT8(1, d.processar(false, false));
  TEST_ASSERT_EQUAL_INT32(4, d.contagem());
}

void test_sentido_contrario_conta_negativo() {
  DecodificadorQuadratura d;
  d.iniciar(false, false);
  aplicarCiclos(d, SENTIDO_NEGATIVO, 1);
  TEST_ASSERT_EQUAL_INT32(-4, d.contagem());
}

void test_varios_ciclos_somam_quatro_pulsos_por_ciclo() {
  DecodificadorQuadratura d;
  d.iniciar(false, false);
  aplicarCiclos(d, SENTIDO_POSITIVO, 250);
  TEST_ASSERT_EQUAL_INT32(1000, d.contagem());
  TEST_ASSERT_EQUAL_UINT32(0, d.transicoesInvalidas());
}

void test_sem_mudanca_de_nivel_nao_conta() {
  DecodificadorQuadratura d;
  d.iniciar(true, false);
  TEST_ASSERT_EQUAL_INT8(0, d.processar(true, false));
  TEST_ASSERT_EQUAL_INT8(0, d.processar(true, false));
  TEST_ASSERT_EQUAL_INT32(0, d.contagem());
  TEST_ASSERT_EQUAL_UINT32(0, d.transicoesInvalidas());
}

void test_vai_e_volta_no_mesmo_canal_se_cancela() {
  // Trepidação: A sobe e desce várias vezes sem a roda andar de fato.
  DecodificadorQuadratura d;
  d.iniciar(false, false);
  for (int i = 0; i < 5; i++) {
    d.processar(true, false);
    d.processar(false, false);
  }
  TEST_ASSERT_EQUAL_INT32(0, d.contagem());
}

void test_transicao_invalida_nao_conta_e_e_registrada() {
  DecodificadorQuadratura d;
  d.iniciar(false, false);
  // 00 → 11: os dois canais mudaram, um pulso foi perdido.
  TEST_ASSERT_EQUAL_INT8(0, d.processar(true, true));
  TEST_ASSERT_EQUAL_INT32(0, d.contagem());
  TEST_ASSERT_EQUAL_UINT32(1, d.transicoesInvalidas());
  // 11 → 00 e os saltos 01 ↔ 10 também são inválidos.
  d.processar(false, false);
  d.iniciar(false, true);
  d.processar(true, false);
  TEST_ASSERT_EQUAL_UINT32(3, d.transicoesInvalidas());
  TEST_ASSERT_EQUAL_INT32(0, d.contagem());
}

void test_decodificador_continua_contando_depois_de_uma_transicao_invalida() {
  DecodificadorQuadratura d;
  d.iniciar(false, false);
  d.processar(true, true);  // inválida: o estado passa a ser 11
  d.processar(false, true);
  d.processar(false, false);
  TEST_ASSERT_EQUAL_INT32(2, d.contagem());
}

void test_inverter_sentido_troca_o_sinal() {
  DecodificadorQuadratura normal(false);
  DecodificadorQuadratura invertido(true);
  normal.iniciar(false, false);
  invertido.iniciar(false, false);
  aplicarCiclos(normal, SENTIDO_POSITIVO, 3);
  aplicarCiclos(invertido, SENTIDO_POSITIVO, 3);
  TEST_ASSERT_EQUAL_INT32(12, normal.contagem());
  TEST_ASSERT_EQUAL_INT32(-12, invertido.contagem());
  // O passo individual também sai invertido: de 00 para 10 seria +1.
  TEST_ASSERT_EQUAL_INT8(-1, invertido.processar(true, false));
}

void test_iniciar_define_o_estado_sem_contar() {
  DecodificadorQuadratura d;
  d.iniciar(true, true);
  TEST_ASSERT_EQUAL_INT32(0, d.contagem());
  // A partir de 11, o próximo passo positivo é 01.
  TEST_ASSERT_EQUAL_INT8(1, d.processar(false, true));
}

void test_zerar_limpa_a_contagem_mas_nao_o_estado() {
  DecodificadorQuadratura d;
  d.iniciar(false, false);
  d.processar(true, false);
  d.processar(true, true);
  d.zerar();
  TEST_ASSERT_EQUAL_INT32(0, d.contagem());
  // O estado continua 11, então o próximo passo positivo ainda é 01.
  TEST_ASSERT_EQUAL_INT8(1, d.processar(false, true));
  TEST_ASSERT_EQUAL_INT32(1, d.contagem());
}

// ---- Conversão de pulsos em distância

void test_conversao_usa_a_constante_em_micrometros_por_pulso() {
  ConversorDeslocamento c(50);
  TEST_ASSERT_EQUAL_INT32(0, c.pulsosParaMicrometros(0));
  TEST_ASSERT_EQUAL_INT32(5000, c.pulsosParaMicrometros(100));
  TEST_ASSERT_EQUAL_INT32(-5000, c.pulsosParaMicrometros(-100));
  TEST_ASSERT_EQUAL_INT32(5, c.pulsosParaMm(100));
  TEST_ASSERT_EQUAL_INT32(-5, c.pulsosParaMm(-100));
}

void test_conversao_para_mm_arredonda_para_o_mais_proximo() {
  ConversorDeslocamento c(100);  // 10 pulsos = 1 mm
  TEST_ASSERT_EQUAL_INT32(0, c.pulsosParaMm(4));    // 0,4 mm
  TEST_ASSERT_EQUAL_INT32(1, c.pulsosParaMm(5));    // 0,5 mm sobe
  TEST_ASSERT_EQUAL_INT32(1, c.pulsosParaMm(14));   // 1,4 mm
  TEST_ASSERT_EQUAL_INT32(2, c.pulsosParaMm(15));   // 1,5 mm sobe
  TEST_ASSERT_EQUAL_INT32(-1, c.pulsosParaMm(-5));  // simétrico para negativos
  TEST_ASSERT_EQUAL_INT32(-2, c.pulsosParaMm(-15));
  TEST_ASSERT_EQUAL_INT32(0, c.pulsosParaMm(-4));
}

void test_uma_celula_de_18_cm_com_a_constante_provisoria() {
  ConversorDeslocamento c;  // padrão: valor provisório
  TEST_ASSERT_EQUAL_UINT32(MICROMETROS_POR_PULSO_PROVISORIO, c.obterMicrometrosPorPulso());
  // 180 mm / 0,251 mm por pulso = 717 pulsos (ida e volta pelo arredondamento).
  const int32_t pulsos = static_cast<int32_t>(180000 / MICROMETROS_POR_PULSO_PROVISORIO);
  const int32_t mm = c.pulsosParaMm(pulsos);
  TEST_ASSERT_INT32_WITHIN(1, 180, mm);
}

void test_uma_volta_da_roda_de_32_mm_mede_cerca_de_100_mm() {
  // Disco de 100 linhas x 4 bordas = 400 pulsos por volta; perímetro = pi x 32 mm = 100,5 mm.
  ConversorDeslocamento c;
  TEST_ASSERT_EQUAL_UINT32(400, PULSOS_POR_VOLTA_DA_RODA);
  TEST_ASSERT_INT32_WITHIN(1, 100, c.pulsosParaMm(static_cast<int32_t>(PULSOS_POR_VOLTA_DA_RODA)));
  // Uma célula de 18 cm pede cerca de 1,8 volta, ou 717 pulsos.
  TEST_ASSERT_INT32_WITHIN(1, 180, c.pulsosParaMm(717));
}

void test_conversao_satura_em_vez_de_estourar() {
  ConversorDeslocamento c(1000000);  // 1 m por pulso, só para forçar o limite
  TEST_ASSERT_EQUAL_INT32(INT32_MAX, c.pulsosParaMicrometros(INT32_MAX));
  TEST_ASSERT_EQUAL_INT32(INT32_MIN, c.pulsosParaMicrometros(INT32_MIN));
  TEST_ASSERT_EQUAL_INT32(INT32_MAX, c.pulsosParaMm(INT32_MAX));
  TEST_ASSERT_EQUAL_INT32(INT32_MIN, c.pulsosParaMm(INT32_MIN));
}

void test_definir_a_constante_muda_a_conversao() {
  ConversorDeslocamento c(10);
  c.definirMicrometrosPorPulso(200);
  TEST_ASSERT_EQUAL_UINT32(200, c.obterMicrometrosPorPulso());
  TEST_ASSERT_EQUAL_INT32(20, c.pulsosParaMm(100));
}

void test_calibracao_calcula_micrometros_por_pulso_de_um_percurso_medido() {
  // A roda andou 180 mm e o encoder contou 716 pulsos.
  const uint32_t k = ConversorDeslocamento::calcularMicrometrosPorPulso(180, 716);
  TEST_ASSERT_EQUAL_UINT32(251, k);
  // Arredondamento: 100 mm em 3 pulsos = 33333,3 µm, arredonda para baixo.
  TEST_ASSERT_EQUAL_UINT32(33333, ConversorDeslocamento::calcularMicrometrosPorPulso(100, 3));
  // E para cima: 2 mm em 3 pulsos = 666,67 µm.
  TEST_ASSERT_EQUAL_UINT32(667, ConversorDeslocamento::calcularMicrometrosPorPulso(2, 3));
}

void test_calibracao_sem_pulsos_devolve_zero() {
  TEST_ASSERT_EQUAL_UINT32(0, ConversorDeslocamento::calcularMicrometrosPorPulso(180, 0));
}

void test_calibrar_e_converter_de_volta_reproduz_a_distancia() {
  const uint32_t k = ConversorDeslocamento::calcularMicrometrosPorPulso(180, 716);
  ConversorDeslocamento c(k);
  TEST_ASSERT_INT32_WITHIN(1, 180, c.pulsosParaMm(716));
}

// ---- Medidor das duas rodas

void test_medidor_converte_cada_roda_separadamente() {
  EncoderSimulado esquerdo, direito;
  ConversorDeslocamento c(100);  // 10 pulsos = 1 mm
  MedidorDeslocamento medidor(esquerdo, direito, c);

  esquerdo.adicionarPulsos(1800);  // 180 mm
  direito.adicionarPulsos(900);    // 90 mm
  const DeslocamentoRodas r = medidor.medir();
  TEST_ASSERT_EQUAL_INT32(180, r.esquerdaMm);
  TEST_ASSERT_EQUAL_INT32(90, r.direitaMm);
}

void test_medidor_deslocamento_medio_e_a_media_das_rodas() {
  EncoderSimulado esquerdo, direito;
  ConversorDeslocamento c(100);
  MedidorDeslocamento medidor(esquerdo, direito, c);
  esquerdo.adicionarPulsos(1800);
  direito.adicionarPulsos(1000);
  TEST_ASSERT_EQUAL_INT32(140, medidor.deslocamentoMedioMm());
}

void test_giro_no_lugar_tem_rodas_com_sinais_opostos_e_media_zero() {
  EncoderSimulado esquerdo, direito;
  ConversorDeslocamento c(100);
  MedidorDeslocamento medidor(esquerdo, direito, c);
  esquerdo.adicionarPulsos(500);
  direito.adicionarPulsos(-500);
  const DeslocamentoRodas r = medidor.medir();
  TEST_ASSERT_EQUAL_INT32(50, r.esquerdaMm);
  TEST_ASSERT_EQUAL_INT32(-50, r.direitaMm);
  TEST_ASSERT_EQUAL_INT32(0, medidor.deslocamentoMedioMm());
}

void test_medidor_zerar_recomeca_a_medicao_nas_duas_rodas() {
  EncoderSimulado esquerdo, direito;
  ConversorDeslocamento c(100);
  MedidorDeslocamento medidor(esquerdo, direito, c);
  esquerdo.adicionarPulsos(1800);
  direito.adicionarPulsos(1800);
  medidor.zerar();
  TEST_ASSERT_EQUAL_INT32(0, esquerdo.lerContagem());
  TEST_ASSERT_EQUAL_INT32(0, direito.lerContagem());
  esquerdo.adicionarPulsos(100);
  TEST_ASSERT_EQUAL_INT32(10, medidor.medir().esquerdaMm);
  TEST_ASSERT_EQUAL_INT32(0, medidor.medir().direitaMm);
}

void test_medidor_nao_estoura_na_media_de_valores_grandes() {
  EncoderSimulado esquerdo, direito;
  ConversorDeslocamento c(1000);  // 1 pulso = 1 mm
  MedidorDeslocamento medidor(esquerdo, direito, c);
  esquerdo.definirContagem(INT32_MAX);
  direito.definirContagem(INT32_MAX);
  TEST_ASSERT_EQUAL_INT32(INT32_MAX, medidor.deslocamentoMedioMm());
}

// ---- Cadeia completa: canais A/B → decodificador → encoder → medidor

/// Encoder que usa o decodificador em quadratura, como o driver da placa fará.
class EncoderPorQuadratura : public IEncoder {
 public:
  explicit EncoderPorQuadratura(bool inverter) : decodificador_(inverter) {
    decodificador_.iniciar(false, false);
  }
  void aplicar(const Nivel* sequencia, int ciclos) { aplicarCiclos(decodificador_, sequencia, ciclos); }
  int32_t lerContagem() const override { return decodificador_.contagem(); }
  void zerar() override { decodificador_.zerar(); }

 private:
  DecodificadorQuadratura decodificador_;
};

void test_rodas_espelhadas_avancando_juntas_dao_deslocamento_positivo_nas_duas() {
  // Na roda direita, o motor está espelhado: os canais giram no sentido oposto ao da esquerda.
  EncoderPorQuadratura esquerdo(false);
  EncoderPorQuadratura direito(true);
  ConversorDeslocamento c(50);  // 20 pulsos = 1 mm
  MedidorDeslocamento medidor(esquerdo, direito, c);

  esquerdo.aplicar(SENTIDO_POSITIVO, 100);  // 400 pulsos
  direito.aplicar(SENTIDO_NEGATIVO, 100);   // 400 pulsos, com a inversão
  const DeslocamentoRodas r = medidor.medir();
  TEST_ASSERT_EQUAL_INT32(20, r.esquerdaMm);
  TEST_ASSERT_EQUAL_INT32(20, r.direitaMm);
}

// ---- EncoderSimulado

void test_encoder_simulado_acumula_pulsos_e_zera() {
  EncoderSimulado e;
  TEST_ASSERT_EQUAL_INT32(0, e.lerContagem());
  e.adicionarPulsos(10);
  e.adicionarPulsos(-3);
  TEST_ASSERT_EQUAL_INT32(7, e.lerContagem());
  e.definirContagem(100);
  TEST_ASSERT_EQUAL_INT32(100, e.lerContagem());
  e.zerar();
  TEST_ASSERT_EQUAL_INT32(0, e.lerContagem());
  IEncoder& pela_interface = e;
  pela_interface.zerar();
  TEST_ASSERT_EQUAL_INT32(0, pela_interface.lerContagem());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_cada_mudanca_de_nivel_conta_um_pulso_no_sentido_positivo);
  RUN_TEST(test_sentido_contrario_conta_negativo);
  RUN_TEST(test_varios_ciclos_somam_quatro_pulsos_por_ciclo);
  RUN_TEST(test_sem_mudanca_de_nivel_nao_conta);
  RUN_TEST(test_vai_e_volta_no_mesmo_canal_se_cancela);
  RUN_TEST(test_transicao_invalida_nao_conta_e_e_registrada);
  RUN_TEST(test_decodificador_continua_contando_depois_de_uma_transicao_invalida);
  RUN_TEST(test_inverter_sentido_troca_o_sinal);
  RUN_TEST(test_iniciar_define_o_estado_sem_contar);
  RUN_TEST(test_zerar_limpa_a_contagem_mas_nao_o_estado);
  RUN_TEST(test_conversao_usa_a_constante_em_micrometros_por_pulso);
  RUN_TEST(test_conversao_para_mm_arredonda_para_o_mais_proximo);
  RUN_TEST(test_uma_celula_de_18_cm_com_a_constante_provisoria);
  RUN_TEST(test_uma_volta_da_roda_de_32_mm_mede_cerca_de_100_mm);
  RUN_TEST(test_conversao_satura_em_vez_de_estourar);
  RUN_TEST(test_definir_a_constante_muda_a_conversao);
  RUN_TEST(test_calibracao_calcula_micrometros_por_pulso_de_um_percurso_medido);
  RUN_TEST(test_calibracao_sem_pulsos_devolve_zero);
  RUN_TEST(test_calibrar_e_converter_de_volta_reproduz_a_distancia);
  RUN_TEST(test_medidor_converte_cada_roda_separadamente);
  RUN_TEST(test_medidor_deslocamento_medio_e_a_media_das_rodas);
  RUN_TEST(test_giro_no_lugar_tem_rodas_com_sinais_opostos_e_media_zero);
  RUN_TEST(test_medidor_zerar_recomeca_a_medicao_nas_duas_rodas);
  RUN_TEST(test_medidor_nao_estoura_na_media_de_valores_grandes);
  RUN_TEST(test_rodas_espelhadas_avancando_juntas_dao_deslocamento_positivo_nas_duas);
  RUN_TEST(test_encoder_simulado_acumula_pulsos_e_zera);
  return UNITY_END();
}
