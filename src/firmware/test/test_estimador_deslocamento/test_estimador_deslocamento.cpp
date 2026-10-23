/// @file test_estimador_deslocamento.cpp
/// @brief Testes do estimador de deslocamento sem encoder (#380): a tabela de velocidade, o
/// `EncoderEstimado`, o par de rodas e a integração com `Odometria`, `ControleMovimento`,
/// `LidarSimulado` e `MotoresSimulados`.
/// Roda no computador: `pio test -e native -f test_estimador_deslocamento`.

#include <unity.h>

#include <cstdint>

#include "ConfiguracaoControle.h"
#include "ConfiguracaoEstimador.h"
#include "ControleMovimento.h"
#include "EncoderEstimado.h"
#include "EncoderSimulado.h"
#include "EstimadorDeslocamento.h"
#include "IEncoder.h"
#include "IMotores.h"
#include "LidarSimulado.h"
#include "MotoresSimulados.h"
#include "Odometria.h"
#include "TabelaVelocidade.h"
#include "Tipos.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

/// Configuração com resposta imediata do motor (sem atraso), para os valores baterem exatamente.
ConfiguracaoEstimador semAtraso() {
  ConfiguracaoEstimador config;
  config.constanteTempoMs = 0;
  return config;
}

/// Roda a estimativa por `ms` milissegundos, em passos de `passoMs`, com a mesma potência.
void andar(EncoderEstimado& roda, int16_t potencia, uint32_t ms, uint32_t passoMs = 10) {
  for (uint32_t t = 0; t + passoMs <= ms; t += passoMs) {
    roda.atualizar(potencia, passoMs);
  }
}

void andar(EstimadorDeslocamento& estimador, PotenciaRodas potencia, uint32_t ms, uint32_t passoMs = 10) {
  for (uint32_t t = 0; t + passoMs <= ms; t += passoMs) {
    estimador.atualizar(potencia, passoMs);
  }
}

}  // namespace

// ---------------------------------------------------------------- TabelaVelocidade

/// Nos pontos da tabela, a velocidade é a do ponto.
void test_tabela_nos_pontos() {
  const TabelaVelocidade tabela;
  TEST_ASSERT_EQUAL_INT32(0, tabela.velocidadeMmPorS(0));
  TEST_ASSERT_EQUAL_INT32(45, tabela.velocidadeMmPorS(200));
  TEST_ASSERT_EQUAL_INT32(120, tabela.velocidadeMmPorS(400));
  TEST_ASSERT_EQUAL_INT32(210, tabela.velocidadeMmPorS(700));
  TEST_ASSERT_EQUAL_INT32(300, tabela.velocidadeMmPorS(1000));
}

/// Entre dois pontos, interpola linearmente com arredondamento: 300 está no meio de 200 (45) e 400 (120).
void test_tabela_interpola_entre_pontos() {
  const TabelaVelocidade tabela;
  TEST_ASSERT_EQUAL_INT32(83, tabela.velocidadeMmPorS(300));  // 45 + 37,5 arredondado
  TEST_ASSERT_EQUAL_INT32(23, tabela.velocidadeMmPorS(150));  // 22,5 arredondado
  TEST_ASSERT_EQUAL_INT32(165, tabela.velocidadeMmPorS(550));
}

/// A zona morta (até 100) não move o robô.
void test_tabela_zona_morta() {
  const TabelaVelocidade tabela;
  TEST_ASSERT_EQUAL_INT32(0, tabela.velocidadeMmPorS(50));
  TEST_ASSERT_EQUAL_INT32(0, tabela.velocidadeMmPorS(100));
  TEST_ASSERT_GREATER_THAN_INT32(0, tabela.velocidadeMmPorS(110));  // 4,5 mm/s arredondado
}

/// Potência negativa dá a mesma velocidade com o sinal trocado; acima do máximo vale o máximo.
void test_tabela_re_e_saturacao() {
  const TabelaVelocidade tabela;
  TEST_ASSERT_EQUAL_INT32(-120, tabela.velocidadeMmPorS(-400));
  TEST_ASSERT_EQUAL_INT32(-83, tabela.velocidadeMmPorS(-300));
  TEST_ASSERT_EQUAL_INT32(300, tabela.velocidadeMmPorS(1500));
  TEST_ASSERT_EQUAL_INT32(-300, tabela.velocidadeMmPorS(-1500));
  TEST_ASSERT_EQUAL_INT32(300, tabela.velocidadeMmPorS(INT16_MAX));
}

/// A velocidade nunca diminui quando a potência aumenta (tabela de exemplo é monótona).
void test_tabela_e_monotona() {
  const TabelaVelocidade tabela;
  int32_t anterior = 0;
  for (int16_t p = 0; p <= 1000; ++p) {
    const int32_t v = tabela.velocidadeMmPorS(p);
    TEST_ASSERT_GREATER_OR_EQUAL_INT32(anterior, v);
    anterior = v;
  }
}

/// Uma tabela própria (outros pontos) é usada como está, e uma tabela vazia dá zero.
void test_tabela_propria_e_vazia() {
  TabelaVelocidade propria;
  propria.pontos[0] = {0, 0};
  propria.pontos[1] = {500, 100};
  propria.quantidade = 2;
  TEST_ASSERT_EQUAL_INT32(50, propria.velocidadeMmPorS(250));
  TEST_ASSERT_EQUAL_INT32(100, propria.velocidadeMmPorS(900));

  TabelaVelocidade vazia;
  vazia.quantidade = 0;
  TEST_ASSERT_EQUAL_INT32(0, vazia.velocidadeMmPorS(400));
}

/// Um trecho em que a velocidade cai (por exemplo, perda de torque em potência alta) também é
/// interpolado, para baixo.
void test_tabela_com_trecho_decrescente() {
  TabelaVelocidade tabela;
  tabela.pontos[0] = {0, 0};
  tabela.pontos[1] = {500, 200};
  tabela.pontos[2] = {1000, 100};
  tabela.quantidade = 3;
  TEST_ASSERT_EQUAL_INT32(100, tabela.velocidadeMmPorS(250));
  TEST_ASSERT_EQUAL_INT32(150, tabela.velocidadeMmPorS(750));  // 200 - 50
  TEST_ASSERT_EQUAL_INT32(-150, tabela.velocidadeMmPorS(-750));
}

// ---------------------------------------------------------------- EncoderEstimado

/// 400 de potência é 120 mm/s: em 1 s, 120 mm = 120 000 µm ÷ 251 µm = 478 pulsos.
void test_potencia_constante_gera_a_contagem_esperada() {
  EncoderEstimado roda(semAtraso());
  andar(roda, 400, 1000);
  TEST_ASSERT_EQUAL_INT32(478, roda.lerContagem());
  TEST_ASSERT_EQUAL_INT64(120000, roda.micrometrosAcumulados());
  TEST_ASSERT_EQUAL_INT32(120, roda.velocidadeMmPorS());
}

/// A potência negativa recua: a contagem fica negativa e o sinal é o mesmo do `IEncoder`.
void test_re_gera_contagem_negativa() {
  EncoderEstimado roda(semAtraso());
  andar(roda, -400, 1000);
  TEST_ASSERT_EQUAL_INT32(-478, roda.lerContagem());
  TEST_ASSERT_EQUAL_INT32(-120, roda.velocidadeMmPorS());
}

/// O tamanho do passo de tempo não muda o resultado: nenhuma fração se perde entre os ciclos.
void test_passo_de_tempo_nao_muda_o_resultado() {
  EncoderEstimado grosso(semAtraso());
  EncoderEstimado fino(semAtraso());
  andar(grosso, 400, 3000, 100);
  andar(fino, 400, 3000, 1);
  TEST_ASSERT_EQUAL_INT64(grosso.micrometrosAcumulados(), fino.micrometrosAcumulados());
  TEST_ASSERT_EQUAL_INT32(grosso.lerContagem(), fino.lerContagem());
}

/// Potência na zona morta não gera movimento, e `dt` zero não muda nada.
void test_zona_morta_e_dt_zero_nao_andam() {
  EncoderEstimado roda(semAtraso());
  andar(roda, 80, 5000);
  TEST_ASSERT_EQUAL_INT32(0, roda.lerContagem());
  roda.atualizar(400, 0);
  TEST_ASSERT_EQUAL_INT32(0, roda.lerContagem());
  TEST_ASSERT_EQUAL_INT32(0, roda.velocidadeMmPorS());
}

/// `zerar()` volta a contagem e a velocidade a zero.
void test_zerar_limpa_a_contagem() {
  EncoderEstimado roda(semAtraso());
  andar(roda, 700, 2000);
  TEST_ASSERT_NOT_EQUAL_INT32(0, roda.lerContagem());
  roda.zerar();
  TEST_ASSERT_EQUAL_INT32(0, roda.lerContagem());
  TEST_ASSERT_EQUAL_INT64(0, roda.micrometrosAcumulados());
  TEST_ASSERT_EQUAL_INT32(0, roda.velocidadeMmPorS());
}

/// A contagem é truncada para zero, também no sentido negativo (não arredonda para baixo).
void test_contagem_trunca_para_zero() {
  EncoderEstimado roda(semAtraso());
  roda.corrigirMicrometros(250);  // menos de um pulso (251 µm)
  TEST_ASSERT_EQUAL_INT32(0, roda.lerContagem());
  roda.corrigirMicrometros(1);
  TEST_ASSERT_EQUAL_INT32(1, roda.lerContagem());
  roda.zerar();
  roda.corrigirMicrometros(-250);
  TEST_ASSERT_EQUAL_INT32(0, roda.lerContagem());
  roda.corrigirMicrometros(-1);
  TEST_ASSERT_EQUAL_INT32(-1, roda.lerContagem());
}

/// A correção dos ToF soma (ou subtrai) deslocamento ao que já foi estimado.
void test_correcao_soma_ao_deslocamento_estimado() {
  EncoderEstimado roda(semAtraso());
  andar(roda, 400, 1000);                 // 120 000 µm
  roda.corrigirMicrometros(-5000);        // o ToF mostrou que o robô andou 5 mm a menos
  TEST_ASSERT_EQUAL_INT64(115000, roda.micrometrosAcumulados());
  TEST_ASSERT_EQUAL_INT32(458, roda.lerContagem());  // 115 000 ÷ 251
  roda.corrigirMicrometros(7000);
  TEST_ASSERT_EQUAL_INT64(122000, roda.micrometrosAcumulados());
}

/// O fator de escala ajusta a velocidade da tabela (robô andando 5% menos que o estimado).
void test_fator_de_escala_ajusta_a_velocidade() {
  ConfiguracaoEstimador config = semAtraso();
  config.fatorEscalaPercentual = 95;
  EncoderEstimado roda(config);
  andar(roda, 400, 1000);
  TEST_ASSERT_EQUAL_INT64(114000, roda.micrometrosAcumulados());  // 95% de 120 000
}

/// A distância por pulso da configuração converte a distância em contagem.
void test_micrometros_por_pulso_vem_da_configuracao() {
  ConfiguracaoEstimador config = semAtraso();
  config.micrometrosPorPulso = 200;
  EncoderEstimado roda(config);
  andar(roda, 400, 1000);
  TEST_ASSERT_EQUAL_INT32(600, roda.lerContagem());  // 120 000 ÷ 200
}

/// Com resposta do motor (atraso), a velocidade sobe aos poucos e anda menos que a resposta imediata,
/// mas converge para a velocidade da tabela.
void test_resposta_do_motor_atrasa_a_partida() {
  ConfiguracaoEstimador com_atraso;  // 80 ms
  EncoderEstimado atrasada(com_atraso);
  EncoderEstimado imediata(semAtraso());
  atrasada.atualizar(400, 10);
  imediata.atualizar(400, 10);
  TEST_ASSERT_LESS_THAN_INT32(imediata.velocidadeMmPorS(), atrasada.velocidadeMmPorS());

  andar(atrasada, 400, 990);
  andar(imediata, 400, 990);
  TEST_ASSERT_LESS_THAN_INT64(imediata.micrometrosAcumulados(), atrasada.micrometrosAcumulados());
  TEST_ASSERT_GREATER_THAN_INT64(100000, atrasada.micrometrosAcumulados());  // perde só uns 10 mm
  TEST_ASSERT_INT32_WITHIN(1, 120, atrasada.velocidadeMmPorS());              // já convergiu
}

/// Soltar a potência também é gradual: o robô ainda anda um pouco depois de a potência zerar.
void test_resposta_do_motor_atrasa_a_parada() {
  EncoderEstimado roda;  // 80 ms
  andar(roda, 400, 1000);
  const int64_t antes = roda.micrometrosAcumulados();
  andar(roda, 0, 500);
  const int64_t depois = roda.micrometrosAcumulados();
  TEST_ASSERT_GREATER_THAN_INT64(antes, depois);
  TEST_ASSERT_LESS_THAN_INT64(antes + 15000, depois);  // desliza só uns milímetros
  TEST_ASSERT_EQUAL_INT32(0, roda.velocidadeMmPorS());
}

/// Uma corrida de 10 minutos na potência máxima não estoura a contagem de 32 bits.
void test_dez_minutos_nao_estouram_a_contagem() {
  EncoderEstimado roda(semAtraso());
  andar(roda, 1000, 600000, 1000);  // 600 s a 300 mm/s = 180 m
  TEST_ASSERT_EQUAL_INT64(180000000, roda.micrometrosAcumulados());
  TEST_ASSERT_EQUAL_INT32(717131, roda.lerContagem());  // 180 000 000 ÷ 251
}

/// A contagem satura no limite de 32 bits, nos dois sentidos, em vez de dar a volta.
void test_contagem_satura_no_limite_de_32_bits() {
  EncoderEstimado para_frente(semAtraso());
  EncoderEstimado para_tras(semAtraso());
  for (int i = 0; i < 300; ++i) {
    para_frente.corrigirMicrometros(INT32_MAX);
    para_tras.corrigirMicrometros(-INT32_MAX);
  }
  TEST_ASSERT_EQUAL_INT32(INT32_MAX, para_frente.lerContagem());
  TEST_ASSERT_EQUAL_INT32(INT32_MIN, para_tras.lerContagem());
}

// ---------------------------------------------------------------- EstimadorDeslocamento

/// Potências iguais dão contagens iguais nas duas rodas.
void test_rodas_com_a_mesma_potencia_andam_igual() {
  EstimadorDeslocamento estimador(semAtraso());
  andar(estimador, PotenciaRodas{400, 400}, 1000);
  TEST_ASSERT_EQUAL_INT32(estimador.esquerdo().lerContagem(), estimador.direito().lerContagem());
  TEST_ASSERT_EQUAL_INT32(478, estimador.esquerdo().lerContagem());
  TEST_ASSERT_EQUAL_INT32(120, estimador.velocidadeMediaMmPorS());
}

/// Potências diferentes dão contagens diferentes (curva), e opostas giram o robô no lugar.
void test_potencias_diferentes_e_opostas() {
  EstimadorDeslocamento curva(semAtraso());
  andar(curva, PotenciaRodas{700, 400}, 1000);
  TEST_ASSERT_GREATER_THAN_INT32(curva.direito().lerContagem(), curva.esquerdo().lerContagem());

  EstimadorDeslocamento giro(semAtraso());
  andar(giro, PotenciaRodas{400, -400}, 1000);
  TEST_ASSERT_EQUAL_INT32(478, giro.esquerdo().lerContagem());
  TEST_ASSERT_EQUAL_INT32(-478, giro.direito().lerContagem());
  TEST_ASSERT_EQUAL_INT32(0, giro.velocidadeMediaMmPorS());
}

/// A correção de avanço soma às duas rodas por igual: não gira o robô.
void test_correcao_de_avanco_nao_gira_o_robo() {
  EstimadorDeslocamento estimador(semAtraso());
  andar(estimador, PotenciaRodas{400, 300}, 1000);
  const int32_t diferencaAntes = estimador.esquerdo().lerContagem() - estimador.direito().lerContagem();
  estimador.corrigirAvancoMicrometros(-10000);
  const int32_t diferencaDepois = estimador.esquerdo().lerContagem() - estimador.direito().lerContagem();
  TEST_ASSERT_INT32_WITHIN(1, diferencaAntes, diferencaDepois);
  estimador.zerar();
  TEST_ASSERT_EQUAL_INT32(0, estimador.esquerdo().lerContagem());
  TEST_ASSERT_EQUAL_INT32(0, estimador.direito().lerContagem());
}

// ---------------------------------------------------------------- integração

/// O estimador alimenta a `Odometria` pela interface `IEncoder`: andando reto, a célula muda ao
/// cruzar a parede (a 90 mm do centro) e o robô chega à célula seguinte.
void test_integracao_odometria_reta_troca_de_celula() {
  EstimadorDeslocamento estimador;  // com a resposta do motor
  Odometria odometria;              // parte em (0, 0), virada para o Norte

  andar(estimador, PotenciaRodas{400, 400}, 500);
  odometria.atualizar(estimador.esquerdo(), estimador.direito());
  TEST_ASSERT_EQUAL_UINT8(0, odometria.pose().celula.linha);  // ~50 mm: ainda na célula 0

  andar(estimador, PotenciaRodas{400, 400}, 500);
  odometria.atualizar(estimador.esquerdo(), estimador.direito());
  TEST_ASSERT_EQUAL_UINT8(1, odometria.pose().celula.linha);  // ~110 mm: cruzou os 90 mm

  andar(estimador, PotenciaRodas{400, 400}, 2000);
  odometria.atualizar(estimador.esquerdo(), estimador.direito());
  TEST_ASSERT_EQUAL_UINT8(2, odometria.pose().celula.linha);  // ~350 mm: célula 2
  TEST_ASSERT_EQUAL_UINT8(0, odometria.pose().celula.coluna);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Norte), static_cast<uint8_t>(odometria.pose().direcao));
  TEST_ASSERT_TRUE(odometria.posicaoValida());
}

/// Rodas em sentidos opostos giram o robô no lugar: com a bitola provisória, cerca de 1 s a 400 de
/// potência dá 90° para a direita (Norte para Leste), sem mudar de célula.
void test_integracao_giro_no_lugar_vira_noventa_graus() {
  EstimadorDeslocamento estimador;
  Odometria odometria;
  andar(estimador, PotenciaRodas{400, -400}, 1000);
  odometria.atualizar(estimador.esquerdo(), estimador.direito());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Leste), static_cast<uint8_t>(odometria.pose().direcao));
  TEST_ASSERT_EQUAL_UINT8(0, odometria.pose().celula.linha);
  TEST_ASSERT_EQUAL_UINT8(0, odometria.pose().celula.coluna);
}

/// Trocar o estimador pelo `EncoderSimulado` (que faz o papel do encoder real) com as mesmas
/// contagens dá exatamente a mesma pose: a `Odometria` não percebe a troca.
void test_integracao_troca_pelo_encoder_real_nao_muda_a_odometria() {
  EstimadorDeslocamento estimador;
  andar(estimador, PotenciaRodas{400, 400}, 1500);
  andar(estimador, PotenciaRodas{300, 300}, 700);

  EncoderSimulado esquerdo;
  EncoderSimulado direito;
  esquerdo.definirContagem(estimador.esquerdo().lerContagem());
  direito.definirContagem(estimador.direito().lerContagem());

  Odometria comEstimador;
  Odometria comEncoder;
  comEstimador.atualizar(estimador.esquerdo(), estimador.direito());
  comEncoder.atualizar(esquerdo, direito);

  TEST_ASSERT_EQUAL_UINT8(comEncoder.pose().celula.linha, comEstimador.pose().celula.linha);
  TEST_ASSERT_EQUAL_UINT8(comEncoder.pose().celula.coluna, comEstimador.pose().celula.coluna);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(comEncoder.pose().direcao),
                          static_cast<uint8_t>(comEstimador.pose().direcao));
  TEST_ASSERT_EQUAL_INT32(comEncoder.posicaoNorteMicrometros(), comEstimador.posicaoNorteMicrometros());
}

/// A pose continua válida no modo estimado e deixa de valer, como com encoder, quando o robô
/// "sai" da malha (a odometria limita a célula a 0..255).
void test_integracao_pose_valida_coerente_com_o_modo_estimado() {
  EstimadorDeslocamento estimador(semAtraso());
  Odometria odometria;
  andar(estimador, PotenciaRodas{1000, 1000}, 5000, 100);  // 1,5 m: dentro da malha
  odometria.atualizar(estimador.esquerdo(), estimador.direito());
  TEST_ASSERT_TRUE(odometria.posicaoValida());

  // 260 células a 180 mm = 46,8 m = 156 s a 300 mm/s: passa de 255.
  andar(estimador, PotenciaRodas{1000, 1000}, 200000, 1000);
  odometria.atualizar(estimador.esquerdo(), estimador.direito());
  TEST_ASSERT_FALSE(odometria.posicaoValida());
}

/// Ciclo completo da navegação sem encoder: o `ControleMovimento` comanda os motores a partir do
/// LiDAR, o estimador transforma a potência em deslocamento, a leitura frontal diminui com o
/// deslocamento e a `Odometria` acompanha. O robô para antes da parede, na célula certa.
void test_integracao_corredor_ate_a_parede_com_estimador() {
  LidarSimulado lidar;
  MotoresSimulados motores;
  ControleMovimento controle;
  EstimadorDeslocamento estimador;
  Odometria odometria;

  const int32_t frenteInicialMm = 84 + 360;  // parede 360 mm à frente do centro da célula 0
  int32_t frenteMm = frenteInicialMm;
  int ciclos = 0;
  while (!controle.parado() && ciclos < 200) {
    lidar.definirLeituraFixa(DistanciasLaterais{static_cast<uint16_t>(frenteMm), 84, 84});
    controle.atualizar(lidar, motores, 100);
    andar(estimador, motores.ultimaPotencia(), 100);
    odometria.atualizar(estimador.esquerdo(), estimador.direito());
    const int32_t andadoMm = estimador.esquerdo().lerContagem() * static_cast<int32_t>(MICROMETROS_POR_PULSO_PROVISORIO) / 1000;
    frenteMm = frenteInicialMm - andadoMm;
    ++ciclos;
  }

  TEST_ASSERT_TRUE(controle.parado());
  TEST_ASSERT_TRUE(motores.freados());
  TEST_ASSERT_LESS_THAN_INT32(200, ciclos);  // chegou, sem ficar rodando para sempre
  TEST_ASSERT_LESS_OR_EQUAL_INT32(84, frenteMm);
  TEST_ASSERT_GREATER_THAN_INT32(60, frenteMm);  // parou antes de encostar na parede
  TEST_ASSERT_EQUAL_UINT8(2, odometria.pose().celula.linha);  // dois passos de 180 mm: célula 2
  TEST_ASSERT_TRUE(odometria.posicaoValida());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_tabela_nos_pontos);
  RUN_TEST(test_tabela_interpola_entre_pontos);
  RUN_TEST(test_tabela_zona_morta);
  RUN_TEST(test_tabela_re_e_saturacao);
  RUN_TEST(test_tabela_e_monotona);
  RUN_TEST(test_tabela_propria_e_vazia);
  RUN_TEST(test_tabela_com_trecho_decrescente);
  RUN_TEST(test_potencia_constante_gera_a_contagem_esperada);
  RUN_TEST(test_re_gera_contagem_negativa);
  RUN_TEST(test_passo_de_tempo_nao_muda_o_resultado);
  RUN_TEST(test_zona_morta_e_dt_zero_nao_andam);
  RUN_TEST(test_zerar_limpa_a_contagem);
  RUN_TEST(test_contagem_trunca_para_zero);
  RUN_TEST(test_correcao_soma_ao_deslocamento_estimado);
  RUN_TEST(test_fator_de_escala_ajusta_a_velocidade);
  RUN_TEST(test_micrometros_por_pulso_vem_da_configuracao);
  RUN_TEST(test_resposta_do_motor_atrasa_a_partida);
  RUN_TEST(test_resposta_do_motor_atrasa_a_parada);
  RUN_TEST(test_dez_minutos_nao_estouram_a_contagem);
  RUN_TEST(test_contagem_satura_no_limite_de_32_bits);
  RUN_TEST(test_rodas_com_a_mesma_potencia_andam_igual);
  RUN_TEST(test_potencias_diferentes_e_opostas);
  RUN_TEST(test_correcao_de_avanco_nao_gira_o_robo);
  RUN_TEST(test_integracao_odometria_reta_troca_de_celula);
  RUN_TEST(test_integracao_giro_no_lugar_vira_noventa_graus);
  RUN_TEST(test_integracao_troca_pelo_encoder_real_nao_muda_a_odometria);
  RUN_TEST(test_integracao_pose_valida_coerente_com_o_modo_estimado);
  RUN_TEST(test_integracao_corredor_ate_a_parede_com_estimador);
  return UNITY_END();
}
