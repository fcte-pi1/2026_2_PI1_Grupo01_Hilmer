/// @file test_localizacao.cpp
/// @brief Testes unitários da correção do erro acumulado pelas paredes (#188): o desvio lateral
/// medido nas paredes, a `CorrecaoPelasParedes` (lateral e, com os encoders, ao longo do avanço), o
/// plano e a execução do realinhamento ao centro do corredor, o rumo pelo corredor e a decisão de
/// realinhar no `CicloNavegacao`. Entradas montadas à mão e motores simulados, sem física.
///
/// A simulação física completa fica em `test_integracao_localizacao`.
/// Roda no computador: `pio test -e native -f test_localizacao`.

#include <unity.h>

#include "CicloNavegacao.h"
#include "ConfiguracaoLocalizacao.h"
#include "CorrecaoPelasParedes.h"
#include "ExecutorMovimentos.h"
#include "Mapa.h"
#include "MotoresSimulados.h"
#include "Movimento.h"
#include "Odometria.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint16_t LONGE = 1000;      ///< Lado ou frente sem parede (leitura da face do sensor).
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

/// Odometria no centro de (1, 1), virada para a direção dada.
Odometria odometriaNoCentro(Direcao direcao = Direcao::Norte) {
  return Odometria(ConfiguracaoOdometria{}, Pose{{1, 1}, direcao});
}

}  // namespace

// ---- Desvio lateral medido pelas paredes

void test_desvio_com_as_duas_paredes_e_a_metade_da_diferenca() {
  // 4 mm à direita do centro: a esquerda lê 58 e a direita 50.
  const DesvioLateral d = medirDesvioLateral({LONGE, 58, 50}, 120, 30);
  TEST_ASSERT_EQUAL(ReferenciaLateral::DuasParedes, d.referencia);
  TEST_ASSERT_EQUAL_INT32(4000, d.desvioUm);
  TEST_ASSERT_EQUAL_INT32(-6000, medirDesvioLateral({LONGE, 48, 60}, 120, 30).desvioUm);
}

void test_desvio_com_uma_parede_usa_a_distancia_ao_centro() {
  // Só a esquerda: 60 + 30 = 90 mm do centro até ela, 6 mm à direita do centro do corredor.
  DesvioLateral d = medirDesvioLateral({LONGE, 60, LONGE}, 120, 30);
  TEST_ASSERT_EQUAL(ReferenciaLateral::SoEsquerda, d.referencia);
  TEST_ASSERT_EQUAL_INT32(6000, d.desvioUm);
  // Só a direita: 60 + 30 = 90 mm até ela, 6 mm à esquerda.
  d = medirDesvioLateral({LONGE, LONGE, 60}, 120, 30);
  TEST_ASSERT_EQUAL(ReferenciaLateral::SoDireita, d.referencia);
  TEST_ASSERT_EQUAL_INT32(-6000, d.desvioUm);
  // Outro recuo do sensor muda a conta.
  TEST_ASSERT_EQUAL_INT32(0, medirDesvioLateral({LONGE, 74, LONGE}, 120, 10).desvioUm);
}

void test_desvio_sem_parede_perto_nao_tem_referencia() {
  const DesvioLateral d = medirDesvioLateral({LONGE, 121, 400}, 120, 30);
  TEST_ASSERT_EQUAL(ReferenciaLateral::SemParede, d.referencia);
  TEST_ASSERT_EQUAL_INT32(0, d.desvioUm);
  TEST_ASSERT_EQUAL(ReferenciaLateral::SoDireita, medirDesvioLateral({LONGE, 121, 120}, 120, 30).referencia);
}

void test_posicao_lateral_nas_quatro_direcoes() {
  // A direita de cada direção: Norte → Leste, Leste → Sul, Sul → Oeste, Oeste → Norte.
  Odometria norte = odometriaNoCentro(Direcao::Norte);
  norte.ajustarPosicao(0, 5000);
  TEST_ASSERT_EQUAL_INT64(180000 + 5000, posicaoLateralUm(norte));
  Odometria leste = odometriaNoCentro(Direcao::Leste);
  leste.ajustarPosicao(0, 5000);
  TEST_ASSERT_EQUAL_INT64(-(180000 - 5000), posicaoLateralUm(leste));
  Odometria sul = odometriaNoCentro(Direcao::Sul);
  sul.ajustarPosicao(0, 5000);
  TEST_ASSERT_EQUAL_INT64(-(180000 - 5000), posicaoLateralUm(sul));
  Odometria oeste = odometriaNoCentro(Direcao::Oeste);
  oeste.ajustarPosicao(0, 5000);
  TEST_ASSERT_EQUAL_INT64(180000 + 5000, posicaoLateralUm(oeste));
}

// ---- Correção pelas paredes

void test_correcao_lateral_leva_a_posicao_ao_desvio_medido() {
  CorrecaoPelasParedes correcao;
  Odometria odometria = odometriaNoCentro();
  // O robô está 6 mm à direita do centro, mas a odometria acha que está no centro.
  const DistanciasLaterais leitura{LONGE, 57, 51};
  ResultadoCorrecaoParedes r = correcao.aplicar(odometria, leitura, MovimentoRobo::Avancando, false);
  TEST_ASSERT_EQUAL(ReferenciaLateral::DuasParedes, r.lateral);
  TEST_ASSERT_EQUAL_INT32(1500, r.deltaLateralUm);  // 50% de 3 mm
  // Repetida, a leitura leva a posição até o desvio medido.
  for (int i = 0; i < 20; i++) {
    correcao.aplicar(odometria, leitura, MovimentoRobo::Parado, false);
  }
  TEST_ASSERT_INT32_WITHIN(10, 180000 + 3000, odometria.posicaoLesteMicrometros());
  TEST_ASSERT_EQUAL_INT32(180000, odometria.posicaoNorteMicrometros());  // o avanço não muda
  TEST_ASSERT_EQUAL_UINT32(21, correcao.correcoesLaterais());
  TEST_ASSERT_EQUAL(ReferenciaLateral::DuasParedes, correcao.ultima().lateral);
}

void test_correcao_lateral_vale_em_qualquer_direcao() {
  CorrecaoPelasParedes correcao;
  // Virada para o Sul, 4 mm à direita do centro do corredor é 4 mm a Oeste.
  Odometria odometria = odometriaNoCentro(Direcao::Sul);
  ConfiguracaoCorrecaoParedes config;
  config.ganhoCorrecaoLateralPercentual = 100;
  CorrecaoPelasParedes cheia(config);
  cheia.aplicar(odometria, {LONGE, 58, 50}, MovimentoRobo::Avancando, false);
  TEST_ASSERT_EQUAL_INT32(180000 - 4000, odometria.posicaoLesteMicrometros());
  (void)correcao;
}

void test_correcao_lateral_corrige_um_erro_acumulado_numa_celula_vizinha() {
  ConfiguracaoCorrecaoParedes config;
  config.ganhoCorrecaoLateralPercentual = 100;
  CorrecaoPelasParedes correcao(config);
  // A odometria acha que está 30 mm à esquerda; o robô está centrado.
  Odometria odometria = odometriaNoCentro();
  odometria.ajustarPosicao(0, -30000);
  correcao.aplicar(odometria, {LONGE, PAREDE_LADO, PAREDE_LADO}, MovimentoRobo::Avancando, false);
  TEST_ASSERT_EQUAL_INT32(180000, odometria.posicaoLesteMicrometros());
}

void test_correcao_lateral_grande_demais_e_descartada() {
  ConfiguracaoCorrecaoParedes config;
  config.ganhoCorrecaoLateralPercentual = 100;
  CorrecaoPelasParedes correcao(config);
  Odometria odometria = odometriaNoCentro();
  odometria.ajustarPosicao(0, -50000);  // 50 mm de diferença: acima dos 40 mm
  const ResultadoCorrecaoParedes r =
      correcao.aplicar(odometria, {LONGE, PAREDE_LADO, PAREDE_LADO}, MovimentoRobo::Avancando, false);
  TEST_ASSERT_EQUAL(ReferenciaLateral::Descartada, r.lateral);
  TEST_ASSERT_EQUAL_INT32(0, r.deltaLateralUm);
  TEST_ASSERT_EQUAL_INT32(180000 - 50000, odometria.posicaoLesteMicrometros());
}

void test_correcao_nao_usa_leituras_de_manobra_nem_sem_parede() {
  CorrecaoPelasParedes correcao;
  Odometria odometria = odometriaNoCentro();
  ResultadoCorrecaoParedes r = correcao.aplicar(odometria, {LONGE, 70, 40}, MovimentoRobo::Manobrando, true);
  TEST_ASSERT_EQUAL(ReferenciaLateral::Manobrando, r.lateral);
  TEST_ASSERT_EQUAL(ResultadoCorrecao::Girando, r.avanco);
  r = correcao.aplicar(odometria, {LONGE, LONGE, LONGE}, MovimentoRobo::Avancando, false);
  TEST_ASSERT_EQUAL(ReferenciaLateral::SemParede, r.lateral);
  TEST_ASSERT_EQUAL_INT32(180000, odometria.posicaoLesteMicrometros());
  TEST_ASSERT_EQUAL_INT32(180000, odometria.posicaoNorteMicrometros());
  TEST_ASSERT_EQUAL_UINT32(0, correcao.correcoesLaterais());
}

void test_com_os_encoders_corrige_tambem_pela_parede_da_frente() {
  CorrecaoPelasParedes correcao;
  // A odometria acha que está no centro de (1, 1); a parede da frente diz que faltam 10 mm.
  Odometria odometria = odometriaNoCentro();
  const DistanciasLaterais leitura{44, PAREDE_LADO, PAREDE_LADO};  // centro a 94 mm da parede
  ResultadoCorrecaoParedes r = correcao.aplicar(odometria, leitura, MovimentoRobo::Parado, true);
  TEST_ASSERT_EQUAL(ResultadoCorrecao::PelaFrente, r.avanco);
  TEST_ASSERT_EQUAL_INT32(-10000, r.deltaAvancoUm);
  TEST_ASSERT_EQUAL_INT32(180000 - 10000, odometria.posicaoNorteMicrometros());
  // No modo estimado (`corrigirAvanco` falso), quem corrige o avanço é o `DeslocamentoEstimado`.
  Odometria estimada = odometriaNoCentro();
  r = correcao.aplicar(estimada, leitura, MovimentoRobo::Parado, false);
  TEST_ASSERT_EQUAL(ResultadoCorrecao::SemReferencia, r.avanco);
  TEST_ASSERT_EQUAL_INT32(180000, estimada.posicaoNorteMicrometros());
  correcao.reiniciar();
}

void test_nomes_da_referencia_lateral() {
  TEST_ASSERT_EQUAL_STRING("sem_parede", nomeDaReferenciaLateral(ReferenciaLateral::SemParede));
  TEST_ASSERT_EQUAL_STRING("duas_paredes", nomeDaReferenciaLateral(ReferenciaLateral::DuasParedes));
  TEST_ASSERT_EQUAL_STRING("so_esquerda", nomeDaReferenciaLateral(ReferenciaLateral::SoEsquerda));
  TEST_ASSERT_EQUAL_STRING("so_direita", nomeDaReferenciaLateral(ReferenciaLateral::SoDireita));
  TEST_ASSERT_EQUAL_STRING("descartada", nomeDaReferenciaLateral(ReferenciaLateral::Descartada));
  TEST_ASSERT_EQUAL_STRING("manobrando", nomeDaReferenciaLateral(ReferenciaLateral::Manobrando));
  TEST_ASSERT_EQUAL_STRING("realinhar", nomeDoMovimento(Movimento::Realinhar));
  TEST_ASSERT_EQUAL_STRING("realinhando", nomeDoEstadoCiclo(EstadoCiclo::Realinhando));
}

// ---- Plano do realinhamento

void test_plano_vazio_sem_desvio_ou_sem_espaco() {
  TEST_ASSERT_EQUAL_INT32(0, planejarRealinhamento(0, true, true, 60, 4).trechoUm);
  TEST_ASSERT_EQUAL_INT32(0, planejarRealinhamento(5000, true, true, 0, 4).trechoUm);
}

void test_plano_usa_o_menor_angulo_que_cabe() {
  // 5 mm com 60 mm de espaço: 5° bastam (57 mm de trecho).
  PlanoRealinhamento p = planejarRealinhamento(5000, false, true, 60, 4);
  TEST_ASSERT_EQUAL_INT32(5000, p.anguloMiligraus);
  TEST_ASSERT_EQUAL_INT32(57471, p.trechoUm);
  TEST_ASSERT_EQUAL_INT32(-57241, p.voltaUm);  // s · cos 5°, no sentido oposto
  TEST_ASSERT_INT32_WITHIN(50, 5000, p.deslocamentoUm);
  // 8 mm: 5° pediriam 92 mm; 8° pedem 57 mm.
  p = planejarRealinhamento(8000, false, true, 60, 4);
  TEST_ASSERT_EQUAL_INT32(8000, p.anguloMiligraus);
  TEST_ASSERT_INT32_WITHIN(60, 8000, p.deslocamentoUm);
}

void test_plano_sinais_para_cada_lado_e_sentido() {
  // Robô à direita do centro (desvio +): vai para a esquerda (deslocamento +).
  PlanoRealinhamento frente = planejarRealinhamento(5000, false, true, 60, 4);
  TEST_ASSERT_GREATER_THAN_INT32(0, frente.anguloMiligraus);  // de frente, gira para onde vai
  TEST_ASSERT_GREATER_THAN_INT32(0, frente.trechoUm);
  PlanoRealinhamento re = planejarRealinhamento(5000, true, true, 60, 4);
  TEST_ASSERT_LESS_THAN_INT32(0, re.anguloMiligraus);  // de ré, a traseira aponta para lá
  TEST_ASSERT_LESS_THAN_INT32(0, re.trechoUm);
  TEST_ASSERT_GREATER_THAN_INT32(0, re.voltaUm);
  TEST_ASSERT_GREATER_THAN_INT32(0, re.deslocamentoUm);
  // Robô à esquerda (desvio −): vai para a direita.
  PlanoRealinhamento esquerda = planejarRealinhamento(-5000, true, true, 60, 4);
  TEST_ASSERT_GREATER_THAN_INT32(0, esquerda.anguloMiligraus);
  TEST_ASSERT_LESS_THAN_INT32(0, esquerda.deslocamentoUm);
}

void test_plano_nao_deixa_a_quina_chegar_a_parede() {
  // 15 mm fora: com folga de 4 mm, a quina pode ir a 84 − 15 − 4 = 65 mm, e só 5° servem
  // (64,1 mm). O trecho de 25 mm não basta: a correção é parcial.
  const PlanoRealinhamento p = planejarRealinhamento(15000, true, true, 25, 4);
  TEST_ASSERT_EQUAL_INT32(-5000, p.anguloMiligraus);
  TEST_ASSERT_EQUAL_INT32(-25000, p.trechoUm);
  TEST_ASSERT_INT32_WITHIN(50, 2175, p.deslocamentoUm);
  // 8 mm em 25 mm pediriam 20°, mas a quina iria a 73,5 mm, além dos 72 mm que sobram: fica em
  // 15° (70,9 mm) e corrige 25 × sen 15° = 6,5 mm.
  const PlanoRealinhamento q = planejarRealinhamento(8000, true, true, 25, 4);
  TEST_ASSERT_EQUAL_INT32(-15000, q.anguloMiligraus);
  TEST_ASSERT_INT32_WITHIN(60, 6475, q.deslocamentoUm);
  // Com folga de 2 mm, 20° passam a caber (73,5 ≤ 74) e o desvio inteiro é corrigido.
  const PlanoRealinhamento r = planejarRealinhamento(8000, true, true, 25, 2);
  TEST_ASSERT_EQUAL_INT32(-20000, r.anguloMiligraus);
  TEST_ASSERT_INT32_WITHIN(60, 8000, r.deslocamentoUm);
}

void test_plano_num_beco_cabe_entre_a_quina_girada_e_a_parede() {
  // Paredes na frente e atrás: girado 20°, a quina avança 67,5 mm no eixo, e sobram 80 − 67,5 =
  // 12,5 mm de trecho. Ângulos menores pediriam trechos maiores que o espaço deles; 25° levaria a
  // quina lateral a 75,5 mm, além dos 74 mm que sobram com 6 mm de desvio. Correção parcial.
  const PlanoRealinhamento p = planejarRealinhamento(6000, true, false, 60, 4);
  TEST_ASSERT_EQUAL_INT32(-20000, p.anguloMiligraus);
  TEST_ASSERT_EQUAL_INT32(-12494, p.trechoUm);
  TEST_ASSERT_INT32_WITHIN(50, 4273, p.deslocamentoUm);
  // Com o lado livre, o mesmo desvio cabe inteiro com 8° (43 mm).
  const PlanoRealinhamento livre = planejarRealinhamento(6000, true, true, 60, 4);
  TEST_ASSERT_EQUAL_INT32(-8000, livre.anguloMiligraus);
  TEST_ASSERT_INT32_WITHIN(60, 6000, livre.deslocamentoUm);
}

// ---- Execução do realinhamento

namespace {

/// Mundo mínimo para o executor: o ângulo segue a diferença das rodas e a posição, a média.
struct MundoMinimo {
  int32_t anguloMiligraus = 0;
  int64_t posicaoUm = 0;

  void avancar(const PotenciaRodas& p) {
    anguloMiligraus += (p.direita - p.esquerda) * 2;  // ~1° por ciclo a 250 de diferença
    posicaoUm += (p.esquerda + p.direita) / 2 * 3;    // 150 de potência ≈ 45 mm/s
  }
};

EstadoExecucao realinharNoMundo(ExecutorMovimentos& executor, MotoresSimulados& motores, MundoMinimo& mundo,
                                uint32_t& ciclos, bool& passouDeRe, bool& passouDeFrente) {
  EstadoExecucao estado = executor.estado();
  ciclos = 0;
  while (executor.ocupado() && ciclos < 2000) {
    estado = executor.atualizar(entrada(nullptr, mundo.posicaoUm, giroValido(mundo.anguloMiligraus)), motores);
    const PotenciaRodas p = motores.ultimaPotencia();
    passouDeRe = passouDeRe || (p.esquerda < 0 && p.direita < 0);
    passouDeFrente = passouDeFrente || (p.esquerda > 0 && p.direita > 0);
    mundo.avancar(p);
    ciclos++;
  }
  return estado;
}

}  // namespace

void test_realinhamento_faz_as_quatro_etapas_e_volta_ao_rumo() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  MundoMinimo mundo;
  const PlanoRealinhamento plano = planejarRealinhamento(8000, true, true, 60, 4);
  TEST_ASSERT_FALSE(executor.iniciar(Movimento::Realinhar));  // só por iniciarRealinhamento
  TEST_ASSERT_TRUE(executor.iniciarRealinhamento(plano));
  TEST_ASSERT_FALSE(executor.iniciarRealinhamento(plano));  // ocupado
  TEST_ASSERT_EQUAL(Movimento::Realinhar, executor.movimento());
  uint32_t ciclos = 0;
  bool re = false;
  bool frente = false;
  TEST_ASSERT_EQUAL(EstadoExecucao::Concluido, realinharNoMundo(executor, motores, mundo, ciclos, re, frente));
  TEST_ASSERT_TRUE(re);
  TEST_ASSERT_TRUE(frente);
  TEST_ASSERT_EQUAL_UINT8(3, executor.etapaRealinhamento());
  TEST_ASSERT_INT32_WITHIN(2000, 0, mundo.anguloMiligraus);  // de volta ao rumo
  // Ré de `s` e frente de `s · cos θ`: no eixo, termina perto de onde começou.
  TEST_ASSERT_INT64_WITHIN(3000, plano.trechoUm + plano.voltaUm, mundo.posicaoUm);
  TEST_ASSERT_TRUE(motores.freados());
  TEST_ASSERT_EQUAL_INT32(0, executor.rumoAlvoMiligraus());  // o realinhamento não muda o rumo
}

void test_realinhamento_mantem_o_rumo_inclinado_no_trecho() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  executor.iniciarRealinhamento(planejarRealinhamento(8000, false, true, 60, 4));
  // Já no ângulo de ida (+8°): o giro conclui e o trecho começa.
  for (int i = 0; i < 6; i++) {
    executor.atualizar(entrada(nullptr, 0, giroValido(8000)), motores);
  }
  TEST_ASSERT_EQUAL_UINT8(1, executor.etapaRealinhamento());
  // No trecho, 2° a menos que o rumo inclinado: a roda direita recebe mais para voltar a ele.
  executor.atualizar(entrada(nullptr, 1000, giroValido(6000)), motores);
  TEST_ASSERT_GREATER_THAN_INT16(motores.ultimaPotencia().esquerda, motores.ultimaPotencia().direita);
  TEST_ASSERT_GREATER_THAN_INT16(0, motores.ultimaPotencia().esquerda);
}

void test_realinhamento_preso_no_trecho_falha_por_tempo() {
  ConfiguracaoRealinhamento config;
  config.tempoMaximoTrechoMs = 100;
  ExecutorMovimentos executor(ConfiguracaoExecucao{}, ConfiguracaoControle{}, ConfiguracaoGiro{}, config);
  MotoresSimulados motores;
  executor.iniciarRealinhamento(planejarRealinhamento(8000, false, true, 60, 4));
  EstadoExecucao estado = EstadoExecucao::Executando;
  for (int i = 0; i < 100 && estado == EstadoExecucao::Executando; i++) {
    estado = executor.atualizar(entrada(nullptr, 0, giroValido(8000)), motores);  // não sai do lugar
  }
  TEST_ASSERT_EQUAL(EstadoExecucao::Falha, estado);
  TEST_ASSERT_EQUAL(FalhaExecucao::TempoEsgotado, executor.falha());
  TEST_ASSERT_TRUE(motores.freados());
}

void test_realinhamento_sem_giroscopio_falha_no_giro() {
  ConfiguracaoGiro configGiro;
  configGiro.tempoMaximoGiroMs = 50;
  ExecutorMovimentos executor(ConfiguracaoExecucao{}, ConfiguracaoControle{}, configGiro);
  MotoresSimulados motores;
  executor.iniciarRealinhamento(planejarRealinhamento(8000, false, true, 60, 4));
  EstadoExecucao estado = EstadoExecucao::Executando;
  for (int i = 0; i < 20 && estado == EstadoExecucao::Executando; i++) {
    estado = executor.atualizar(entrada(nullptr, 0, LeituraGiro{}), motores);
  }
  TEST_ASSERT_EQUAL(FalhaExecucao::GiroTempoEsgotado, executor.falha());
}

// ---- Rumo pelo corredor

void test_rumo_pelo_corredor_segue_o_angulo_com_duas_paredes_paralelas() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  executor.iniciar(Movimento::AvancarCelula);
  // O giroscópio derivou 2°: o robô anda reto entre as paredes, mas o ângulo marca 2°.
  int64_t posicao = 0;
  for (int i = 0; i < 30; i++) {
    executor.atualizar(entrada(&corredor, posicao, giroValido(2000), 33), motores);
    posicao += 1000;
  }
  TEST_ASSERT_INT32_WITHIN(100, 2000, executor.rumoAlvoMiligraus());
  TEST_ASSERT_EQUAL_INT32(0, executor.rumoNominalMiligraus());
  TEST_ASSERT_EQUAL_INT32(executor.rumoAlvoMiligraus(), executor.controleMovimento().rumoAlvoMiligraus());
  // O giro seguinte parte do rumo corrigido.
  executor.cancelar(motores);
  executor.iniciar(Movimento::Girar90Esquerda);
  TEST_ASSERT_INT32_WITHIN(100, 92000, executor.rumoAlvoMiligraus());
  TEST_ASSERT_EQUAL_INT32(90000, executor.rumoNominalMiligraus());
}

void test_rumo_pelo_corredor_so_com_as_duas_paredes() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  const DistanciasLaterais umaParede{LONGE, PAREDE_LADO, LONGE};
  executor.iniciar(Movimento::AvancarCelula);
  for (int i = 0; i < 30; i++) {
    executor.atualizar(entrada(&umaParede, i * 1000, giroValido(2000), 33), motores);
  }
  TEST_ASSERT_EQUAL_INT32(0, executor.rumoAlvoMiligraus());
}

void test_rumo_pelo_corredor_respeita_o_limite_e_pode_ser_desligado() {
  ConfiguracaoRealinhamento config;
  config.ajusteRumoMaximoMiligraus = 1000;
  ExecutorMovimentos limitado(ConfiguracaoExecucao{}, ConfiguracaoControle{}, ConfiguracaoGiro{}, config);
  MotoresSimulados motores;
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  limitado.iniciar(Movimento::AvancarCelula);
  for (int i = 0; i < 40; i++) {
    limitado.atualizar(entrada(&corredor, i * 1000, giroValido(5000), 33), motores);
  }
  TEST_ASSERT_EQUAL_INT32(1000, limitado.rumoAlvoMiligraus());

  config.ajusteRumoPeloCorredorPorMil = 0;
  ExecutorMovimentos desligado(ConfiguracaoExecucao{}, ConfiguracaoControle{}, ConfiguracaoGiro{}, config);
  desligado.iniciar(Movimento::AvancarCelula);
  for (int i = 0; i < 40; i++) {
    desligado.atualizar(entrada(&corredor, i * 1000, giroValido(5000), 33), motores);
  }
  TEST_ASSERT_EQUAL_INT32(0, desligado.rumoAlvoMiligraus());
}

void test_rumo_pelo_corredor_nao_ajusta_se_o_robo_nao_esta_paralelo() {
  ExecutorMovimentos executor;
  MotoresSimulados motores;
  executor.iniciar(Movimento::AvancarCelula);
  // A distância à parede muda 1 mm por leitura de 33 ms (30 mm/s): o robô está se aproximando dela.
  for (int i = 0; i < 30; i++) {
    const DistanciasLaterais leitura{LONGE, static_cast<uint16_t>(40 + i), static_cast<uint16_t>(68 - i)};
    executor.atualizar(entrada(&leitura, i * 1000, giroValido(2000), 33), motores);
  }
  TEST_ASSERT_INT32_WITHIN(300, 0, executor.rumoAlvoMiligraus());
}

// ---- Decisão de realinhar no ciclo

namespace {

struct CicloDeTeste {
  Mapa mapa{DimensaoMapa::Labirinto4x4};
  MotoresSimulados motores;
  CicloNavegacao ciclo{mapa};

  CicloDeTeste() { ciclo.reiniciar(Pose{{0, 0}, Direcao::Norte}, motores); }

  EntradaCiclo entradaBase(const DistanciasLaterais* leitura, LeituraGiro giro = giroValido()) const {
    EntradaCiclo e;
    e.liberado = true;
    e.pose = Pose{{0, 0}, Direcao::Norte};
    e.leituraNova = leitura;
    e.giro = giro;
    e.dtMs = 10;
    return e;
  }

  EstadoCiclo perceber(const DistanciasLaterais& leitura) {
    EstadoCiclo estado = ciclo.estado();
    for (int i = 0; i < 4; i++) {
      estado = ciclo.atualizar(entradaBase(&leitura), motores);
    }
    return estado;
  }
};

}  // namespace

void test_ciclo_realinha_antes_de_girar_quando_esta_fora_do_centro() {
  CicloDeTeste t;
  // Parede à frente e à esquerda, direita livre: vai girar para o Leste. A esquerda lê 62: 8 mm à
  // direita do centro. Atrás é o perímetro (fechado) e a frente tem parede: espaço curto.
  const DistanciasLaterais leitura{34, 62, LONGE};
  TEST_ASSERT_EQUAL(EstadoCiclo::Realinhando, t.perceber(leitura));
  TEST_ASSERT_EQUAL_INT32(8, t.ciclo.ultimoDesvioLateralMm());
  TEST_ASSERT_EQUAL(MovimentoRobo::Manobrando, t.ciclo.movimentoRobo());
  TEST_ASSERT_TRUE(t.ciclo.direcaoDefinida());
  const PlanoRealinhamento& plano = t.ciclo.executor().planoRealinhamento();
  TEST_ASSERT_LESS_THAN_INT32(0, plano.trechoUm);                  // fechado dos dois lados: de ré
  TEST_ASSERT_LESS_OR_EQUAL_INT32(25000, -plano.trechoUm);         // dentro do espaço curto
  TEST_ASSERT_GREATER_THAN_INT32(0, plano.deslocamentoUm);         // para a esquerda
  TEST_ASSERT_EQUAL(Movimento::Realinhar, t.ciclo.executor().movimento());
}

void test_ciclo_perto_do_centro_ou_sem_parede_gira_direto() {
  CicloDeTeste perto;
  TEST_ASSERT_EQUAL(EstadoCiclo::Girando, perto.perceber({34, 56, LONGE}));  // 2 mm: abaixo do limiar
  CicloDeTeste semParede;
  // Sem parede lateral não há referência (e nada para a quina encostar).
  TEST_ASSERT_EQUAL(EstadoCiclo::Girando, semParede.perceber({34, LONGE, LONGE}));
  CicloDeTeste longeDemais;
  // 30 mm fora: acima do máximo, a leitura é suspeita.
  TEST_ASSERT_EQUAL(EstadoCiclo::Girando, longeDemais.perceber({34, 84, LONGE}));
}

void test_ciclo_confere_depois_do_realinhamento_e_tenta_de_novo() {
  CicloDeTeste t;
  const DistanciasLaterais fora{34, 62, LONGE};
  t.perceber(fora);
  TEST_ASSERT_EQUAL(EstadoCiclo::Realinhando, t.ciclo.estado());
  // Simula o realinhamento até o fim, num mundo que não muda (o robô não sai do lugar de verdade).
  MundoMinimo mundo;
  for (int i = 0; i < 3000 && !(t.ciclo.estado() == EstadoCiclo::Realinhando && t.ciclo.movimentoRobo() ==
                                                                                       MovimentoRobo::Parado);
       i++) {
    EntradaCiclo e = t.entradaBase(nullptr, giroValido(mundo.anguloMiligraus));
    e.posicaoAvancoUm = mundo.posicaoUm;
    t.ciclo.atualizar(e, t.motores);
    mundo.avancar(t.motores.ultimaPotencia());
  }
  TEST_ASSERT_EQUAL_UINT32(1, t.ciclo.realinhamentos());
  TEST_ASSERT_EQUAL(MovimentoRobo::Parado, t.ciclo.movimentoRobo());  // conferindo, parado
  // A conferência ainda vê 8 mm: realinha mais uma vez (segunda tentativa).
  for (int i = 0; i < 3; i++) {
    EntradaCiclo e = t.entradaBase(&fora, giroValido(mundo.anguloMiligraus));
    e.posicaoAvancoUm = mundo.posicaoUm;
    t.ciclo.atualizar(e, t.motores);
  }
  TEST_ASSERT_EQUAL(MovimentoRobo::Manobrando, t.ciclo.movimentoRobo());
  for (int i = 0; i < 3000 && t.ciclo.movimentoRobo() == MovimentoRobo::Manobrando; i++) {
    EntradaCiclo e = t.entradaBase(nullptr, giroValido(mundo.anguloMiligraus));
    e.posicaoAvancoUm = mundo.posicaoUm;
    t.ciclo.atualizar(e, t.motores);
    mundo.avancar(t.motores.ultimaPotencia());
  }
  TEST_ASSERT_EQUAL_UINT32(2, t.ciclo.realinhamentos());
  // Esgotadas as tentativas, gira mesmo com o desvio.
  for (int i = 0; i < 3; i++) {
    EntradaCiclo e = t.entradaBase(&fora, giroValido(mundo.anguloMiligraus));
    t.ciclo.atualizar(e, t.motores);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Girando, t.ciclo.estado());
}

void test_ciclo_realinha_de_re_quando_veio_de_tras() {
  CicloDeTeste t;
  // Primeiro passo: corredor para o Norte, até (1, 0).
  const DistanciasLaterais corredor{LONGE, PAREDE_LADO, PAREDE_LADO};
  t.perceber(corredor);
  TEST_ASSERT_EQUAL(EstadoCiclo::Avancando, t.ciclo.estado());
  EntradaCiclo e = t.entradaBase(&corredor);
  t.ciclo.atualizar(e, t.motores);  // o primeiro ciclo do avanço fixa o destino
  e.posicaoAvancoUm = 178000;
  e.pose = Pose{{1, 0}, Direcao::Norte};
  t.ciclo.atualizar(e, t.motores);
  TEST_ASSERT_EQUAL(EstadoCiclo::Percebendo, t.ciclo.estado());
  // Em (1, 0): parede à frente e à esquerda, 8 mm à direita. Atrás está livre (de onde veio).
  const DistanciasLaterais fora{34, 62, LONGE};
  for (int i = 0; i < 3; i++) {
    e.leituraNova = &fora;
    t.ciclo.atualizar(e, t.motores);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Realinhando, t.ciclo.estado());
  const PlanoRealinhamento& plano = t.ciclo.executor().planoRealinhamento();
  TEST_ASSERT_LESS_THAN_INT32(0, plano.trechoUm);  // de ré, para a célula livre de trás
  TEST_ASSERT_GREATER_THAN_INT32(25000, -plano.trechoUm);  // com o espaço maior
}

void test_ciclo_sem_permissao_no_realinhamento_freia() {
  CicloDeTeste t;
  t.perceber({34, 62, LONGE});
  EntradaCiclo e = t.entradaBase(nullptr);
  e.liberado = false;
  TEST_ASSERT_EQUAL(EstadoCiclo::AguardandoLargada, t.ciclo.atualizar(e, t.motores));
  TEST_ASSERT_TRUE(t.motores.freados());
  TEST_ASSERT_EQUAL(MovimentoRobo::Parado, t.ciclo.movimentoRobo());
}

void test_ciclo_falha_do_realinhamento_para() {
  ConfiguracaoRealinhamento config;
  config.tempoMaximoTrechoMs = 50;
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  MotoresSimulados motores;
  CicloNavegacao ciclo(mapa, ConfiguracaoExecucao{}, ConfiguracaoControle{}, ConfiguracaoGiro{}, config);
  ciclo.reiniciar(Pose{{0, 0}, Direcao::Norte}, motores);
  const DistanciasLaterais fora{34, 62, LONGE};
  EntradaCiclo e;
  e.liberado = true;
  e.leituraNova = &fora;
  e.giro = giroValido();
  e.dtMs = 10;
  for (int i = 0; i < 4; i++) {
    ciclo.atualizar(e, motores);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Realinhando, ciclo.estado());
  // O robô não gira nem anda: o giro de ida esgota o tempo.
  e.leituraNova = nullptr;
  for (int i = 0; i < 400 && ciclo.estado() == EstadoCiclo::Realinhando; i++) {
    ciclo.atualizar(e, motores);
  }
  TEST_ASSERT_EQUAL(EstadoCiclo::Falha, ciclo.estado());
  TEST_ASSERT_EQUAL(FalhaExecucao::GiroTempoEsgotado, ciclo.falhaExecucao());
}

void test_movimento_do_robo_para_a_correcao() {
  TEST_ASSERT_TRUE(ehManobra(Movimento::Realinhar));
  TEST_ASSERT_TRUE(ehManobra(Movimento::Girar90Direita));
  TEST_ASSERT_FALSE(ehManobra(Movimento::AvancarCelula));
  TEST_ASSERT_FALSE(ehGiro(Movimento::Realinhar));
  TEST_ASSERT_EQUAL_INT32(0, variacaoRumoMiligraus(Movimento::Realinhar));
  TEST_ASSERT_EQUAL(Direcao::Leste, direcaoDepois(Direcao::Leste, Movimento::Realinhar));
  CicloDeTeste t;
  TEST_ASSERT_EQUAL(MovimentoRobo::Parado, t.ciclo.movimentoRobo());
  t.perceber({LONGE, PAREDE_LADO, PAREDE_LADO});
  TEST_ASSERT_EQUAL(MovimentoRobo::Avancando, t.ciclo.movimentoRobo());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_desvio_com_as_duas_paredes_e_a_metade_da_diferenca);
  RUN_TEST(test_desvio_com_uma_parede_usa_a_distancia_ao_centro);
  RUN_TEST(test_desvio_sem_parede_perto_nao_tem_referencia);
  RUN_TEST(test_posicao_lateral_nas_quatro_direcoes);
  RUN_TEST(test_correcao_lateral_leva_a_posicao_ao_desvio_medido);
  RUN_TEST(test_correcao_lateral_vale_em_qualquer_direcao);
  RUN_TEST(test_correcao_lateral_corrige_um_erro_acumulado_numa_celula_vizinha);
  RUN_TEST(test_correcao_lateral_grande_demais_e_descartada);
  RUN_TEST(test_correcao_nao_usa_leituras_de_manobra_nem_sem_parede);
  RUN_TEST(test_com_os_encoders_corrige_tambem_pela_parede_da_frente);
  RUN_TEST(test_nomes_da_referencia_lateral);
  RUN_TEST(test_plano_vazio_sem_desvio_ou_sem_espaco);
  RUN_TEST(test_plano_usa_o_menor_angulo_que_cabe);
  RUN_TEST(test_plano_sinais_para_cada_lado_e_sentido);
  RUN_TEST(test_plano_nao_deixa_a_quina_chegar_a_parede);
  RUN_TEST(test_plano_num_beco_cabe_entre_a_quina_girada_e_a_parede);
  RUN_TEST(test_realinhamento_faz_as_quatro_etapas_e_volta_ao_rumo);
  RUN_TEST(test_realinhamento_mantem_o_rumo_inclinado_no_trecho);
  RUN_TEST(test_realinhamento_preso_no_trecho_falha_por_tempo);
  RUN_TEST(test_realinhamento_sem_giroscopio_falha_no_giro);
  RUN_TEST(test_rumo_pelo_corredor_segue_o_angulo_com_duas_paredes_paralelas);
  RUN_TEST(test_rumo_pelo_corredor_so_com_as_duas_paredes);
  RUN_TEST(test_rumo_pelo_corredor_respeita_o_limite_e_pode_ser_desligado);
  RUN_TEST(test_rumo_pelo_corredor_nao_ajusta_se_o_robo_nao_esta_paralelo);
  RUN_TEST(test_ciclo_realinha_antes_de_girar_quando_esta_fora_do_centro);
  RUN_TEST(test_ciclo_perto_do_centro_ou_sem_parede_gira_direto);
  RUN_TEST(test_ciclo_confere_depois_do_realinhamento_e_tenta_de_novo);
  RUN_TEST(test_ciclo_realinha_de_re_quando_veio_de_tras);
  RUN_TEST(test_ciclo_sem_permissao_no_realinhamento_freia);
  RUN_TEST(test_ciclo_falha_do_realinhamento_para);
  RUN_TEST(test_movimento_do_robo_para_a_correcao);
  return UNITY_END();
}
