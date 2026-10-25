/// @file test_deslocamento.cpp
/// @brief Testes do deslocamento estimado sem encoder (#380): tabela de velocidade, integração no
/// tempo, encoders e motores estimados com a `Odometria` sem mudança, correção pela parede da
/// frente e pelas quinas laterais, e confiança da pose.
///
/// As correções são testadas num corredor do `LABIRINTO_4X4`: um robô "real" anda com velocidade
/// diferente da que a tabela supõe (o erro que o modo estimado comete na prática) e as leituras do
/// ToF vêm da posição real dele.
/// Roda no computador: `pio test -e native -f test_deslocamento`.

#include <unity.h>

#include "../test_labirinto_simulado/LabirintosExemplo.h"
#include "ConfiguracaoControle.h"
#include "DeslocamentoEstimado.h"
#include "LabirintoSimulado.h"
#include "MotoresSimulados.h"
#include "Odometria.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr int64_t CELULA_UM = static_cast<int64_t>(TAMANHO_CELULA_MM) * 1000;
constexpr uint16_t LONGE_FRENTE = 1000;  ///< Frente sem parede ao alcance da correção.

/// Tabela simples para os testes: zona morta abaixo de 100, 1 mm/s por unidade até 1000.
constexpr PontoVelocidade TABELA_TESTE[] = {{100, 100}, {1000, 1000}};

ConfiguracaoDeslocamento configTeste() {
  ConfiguracaoDeslocamento config;
  config.tabela = TABELA_TESTE;
  config.pontosTabela = 2;
  config.micrometrosPorPulso = 250;
  config.distanciaSensorFrontalAoCentroMm = 50;
  config.avancoSensoresLateraisMm = 0;
  return config;
}

ConfiguracaoOdometria odometriaTeste() {
  ConfiguracaoOdometria config;
  config.micrometrosPorPulso = 250;
  config.pulsosDiferenciaisPor90Graus = 400;
  return config;
}

/// Robô "real" no corredor da coluna 0 do `LABIRINTO_4X4`, virado para o Norte, partindo do centro
/// de (0, 0). Lá, a parede da frente é a borda Norte (linha 3) e o lado direito (Leste) alterna:
/// parede na linha 0, livre na 1, parede na 2 e livre na 3.
class CorredorReal {
 public:
  explicit CorredorReal(const LabirintoSimulado& labirinto) : labirinto_(labirinto) {}

  /// Anda `dtMs` com a velocidade real `mmPorS`.
  void andar(int32_t mmPorS, uint32_t dtMs) { posicaoUm_ += static_cast<int64_t>(mmPorS) * dtMs; }

  int64_t posicaoUm() const { return posicaoUm_; }

  /// O que os 3 ToF leriam na posição real (sensor frontal a 50 mm do centro, laterais no centro).
  DistanciasLaterais leitura() const {
    // Parede da frente: a primeira fronteira ao Norte com parede, menos meia espessura.
    int64_t faceUm = 0;
    for (uint8_t linha = 0; linha < 4; linha++) {
      if (labirinto_.temParede({linha, 0}, Direcao::Norte)) {
        faceUm = linha * CELULA_UM + CELULA_UM / 2 - ESPESSURA_PAREDE_MM * 1000 / 2;
        break;
      }
    }
    const int64_t frenteMm = (faceUm - posicaoUm_) / 1000 - 50;
    return {static_cast<uint16_t>(frenteMm < 0 ? 0 : frenteMm), ladoEsquerdo(), ladoDireito()};
  }

 private:
  /// Linha onde está o ponto, e se ele está em cima de uma coluna (quina da fronteira).
  bool naColuna(int64_t pontoUm) const {
    const int64_t relativo = (pontoUm + CELULA_UM / 2) % CELULA_UM;  // 0 na fronteira de baixo
    const int64_t meia = ESPESSURA_PAREDE_MM * 1000 / 2;
    return relativo < meia || relativo > CELULA_UM - meia;
  }

  uint8_t linhaDe(int64_t pontoUm) const {
    return static_cast<uint8_t>((pontoUm + CELULA_UM / 2) / CELULA_UM);
  }

  uint16_t lado(Direcao direcao) const {
    const bool parede = naColuna(posicaoUm_) || labirinto_.temParede({linhaDe(posicaoUm_), 0}, direcao);
    return parede ? 30 : 400;
  }

  uint16_t ladoEsquerdo() const { return lado(Direcao::Oeste); }
  uint16_t ladoDireito() const { return lado(Direcao::Leste); }

  const LabirintoSimulado& labirinto_;
  int64_t posicaoUm_ = 0;
};

/// Erro da posição estimada ao longo do Norte em relação à real, em mm.
int32_t erroMm(const Odometria& odometria, const CorredorReal& real) {
  return static_cast<int32_t>((odometria.posicaoNorteMicrometros() - real.posicaoUm()) / 1000);
}

int32_t modulo(int32_t valor) { return valor < 0 ? -valor : valor; }

/// Faz o robô andar `ms` com potência `potencia`, com o real a `fatorPercentual`% da velocidade da
/// tabela, lendo o ToF a cada 30 ms. Devolve quantas correções foram aplicadas.
uint32_t percorrer(DeslocamentoEstimado& deslocamento, Odometria& odometria, CorredorReal& real,
                   int16_t potencia, uint32_t ms, int32_t fatorPercentual, bool usarTof) {
  deslocamento.motores().definirPotencia({potencia, potencia});
  const int32_t velocidadeReal =
      deslocamento.estimador().velocidadeMmPorS(potencia) * fatorPercentual / 100;
  uint32_t correcoes = 0;
  for (uint32_t t = 10; t <= ms; t += 10) {
    real.andar(velocidadeReal, 10);
    DistanciasLaterais leitura = real.leitura();
    const bool temLeitura = usarTof && t % 30 == 0;
    const ResultadoCorrecao r = deslocamento.ciclo(10, odometria, temLeitura ? &leitura : nullptr);
    if (r == ResultadoCorrecao::PelaFrente || r == ResultadoCorrecao::PelaLateral) {
      correcoes++;
    }
  }
  return correcoes;
}

}  // namespace

// ---- Tabela de velocidade ---------------------------------------------------------------------

void test_tabela_zona_morta_pontos_e_interpolacao() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores);  // tabela padrão do ConfiguracaoDeslocamento.h
  const EstimadorDeslocamento& e = deslocamento.estimador();
  TEST_ASSERT_EQUAL_INT32(0, e.velocidadeMmPorS(0));
  TEST_ASSERT_EQUAL_INT32(0, e.velocidadeMmPorS(149));  // zona morta
  TEST_ASSERT_EQUAL_INT32(45, e.velocidadeMmPorS(150));
  TEST_ASSERT_EQUAL_INT32(120, e.velocidadeMmPorS(400));
  TEST_ASSERT_EQUAL_INT32(165, e.velocidadeMmPorS(550));  // meio do caminho entre 400 e 700
  TEST_ASSERT_EQUAL_INT32(300, e.velocidadeMmPorS(1000));
}

void test_tabela_re_e_saturacao() {
  EstimadorDeslocamento e(configTeste());
  TEST_ASSERT_EQUAL_INT32(-500, e.velocidadeMmPorS(-500));
  TEST_ASSERT_EQUAL_INT32(0, e.velocidadeMmPorS(-99));
  TEST_ASSERT_EQUAL_INT32(1000, e.velocidadeMmPorS(1000));
  e.comandar({2000, -2000});  // fora da faixa: limitado a ±1000
  TEST_ASSERT_EQUAL_INT16(1000, e.potencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(-1000, e.potencia().direita);
}

void test_tabela_vazia_nao_move() {
  ConfiguracaoDeslocamento config = configTeste();
  config.pontosTabela = 0;
  EstimadorDeslocamento e(config);
  TEST_ASSERT_EQUAL_INT32(0, e.velocidadeMmPorS(800));
}

void test_tabela_acima_do_ultimo_ponto_satura_e_re_arredonda_para_baixo() {
  static constexpr PontoVelocidade curta[] = {{100, 100}, {500, 500}};
  ConfiguracaoDeslocamento config = configTeste();
  config.tabela = curta;
  config.pontosTabela = 2;
  EstimadorDeslocamento e(config);
  TEST_ASSERT_EQUAL_INT32(500, e.velocidadeMmPorS(800));
  TEST_ASSERT_EQUAL_INT32(-500, e.velocidadeMmPorS(-800));
  // Ré de 0,1 mm (100 µm): menos de um pulso, mas a contagem já é -1 (arredonda para baixo).
  e.comandar({-100, -100});
  e.atualizar(1);
  TEST_ASSERT_EQUAL_INT32(-1, e.contagem(Roda::Esquerda));
}

// ---- Integração no tempo e encoders estimados ---------------------------------------------------

void test_integra_velocidade_vezes_tempo_em_pulsos() {
  EstimadorDeslocamento e(configTeste());
  e.comandar({500, 250});
  for (int i = 0; i < 100; i++) {
    e.atualizar(10);  // 1 s
  }
  // 500 mm e 250 mm, em pulsos de 250 µm.
  TEST_ASSERT_EQUAL_INT32(2000, e.contagem(Roda::Esquerda));
  TEST_ASSERT_EQUAL_INT32(1000, e.contagem(Roda::Direita));
}

void test_parar_congela_a_contagem_e_re_diminui() {
  EstimadorDeslocamento e(configTeste());
  e.comandar({400, 400});
  e.atualizar(500);  // 200 mm = 800 pulsos
  e.parar();
  e.atualizar(500);
  TEST_ASSERT_EQUAL_INT32(800, e.contagem(Roda::Esquerda));
  e.comandar({-400, -400});
  e.atualizar(250);  // volta 100 mm
  TEST_ASSERT_EQUAL_INT32(400, e.contagem(Roda::Direita));
}

void test_encoders_estimados_seguem_o_iencoder() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  IEncoder& esquerdo = deslocamento.encoderEsquerdo();
  IEncoder& direito = deslocamento.encoderDireito();
  deslocamento.motores().definirPotencia({300, 300});
  Odometria odometria(odometriaTeste());
  deslocamento.ciclo(100, odometria, nullptr);
  TEST_ASSERT_EQUAL_INT32(120, esquerdo.lerContagem());  // 30 mm
  esquerdo.zerar();
  TEST_ASSERT_EQUAL_INT32(0, esquerdo.lerContagem());
  TEST_ASSERT_EQUAL_INT32(120, direito.lerContagem());
}

void test_motores_com_estimativa_repassam_ao_driver_real() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  deslocamento.motores().definirPotencia({300, -200});
  TEST_ASSERT_EQUAL_INT16(300, motores.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(-200, motores.ultimaPotencia().direita);
  TEST_ASSERT_EQUAL_INT16(300, deslocamento.estimador().potencia().esquerda);
  TEST_ASSERT_TRUE(deslocamento.estimador().girando());
  deslocamento.motores().parar();
  TEST_ASSERT_TRUE(motores.freados());
  TEST_ASSERT_EQUAL_INT16(0, deslocamento.estimador().potencia().direita);
}

// ---- Odometria sem mudança, alimentada pelos encoders estimados ---------------------------------

void test_odometria_avanca_uma_celula_com_encoders_estimados() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  deslocamento.motores().definirPotencia({360, 360});  // 360 mm/s
  for (int i = 0; i < 50; i++) {
    deslocamento.ciclo(10, odometria, nullptr);  // 0,5 s = 180 mm
  }
  TEST_ASSERT_EQUAL_INT32(180000, odometria.posicaoNorteMicrometros());
  TEST_ASSERT_EQUAL_UINT8(1, odometria.pose().celula.linha);
  TEST_ASSERT_EQUAL(Direcao::Norte, odometria.pose().direcao);
}

void test_odometria_gira_90_graus_com_rodas_opostas() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  // 400 pulsos de diferença = 90°: cada roda anda 200 pulsos (50 mm) em sentidos opostos.
  deslocamento.motores().definirPotencia({500, -500});
  for (int i = 0; i < 10; i++) {
    deslocamento.ciclo(10, odometria, nullptr);  // 0,1 s × 500 mm/s = 50 mm
  }
  TEST_ASSERT_EQUAL(Direcao::Leste, odometria.pose().direcao);
  TEST_ASSERT_EQUAL_UINT8(0, odometria.pose().celula.linha);
  TEST_ASSERT_EQUAL_UINT8(0, odometria.pose().celula.coluna);
}

// ---- Correção pela parede da frente --------------------------------------------------------------

void test_correcao_pela_frente_alinha_a_posicao_real() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  // Perto da parede da linha 3 (face a 624 mm da partida): real em 500 mm, estimativa em 450 mm.
  LabirintoSimulado labirinto;
  labirinto.carregar(exemplos::LABIRINTO_4X4);
  CorredorReal real(labirinto);
  real.andar(1000, 500);  // 500 mm
  deslocamento.motores().definirPotencia({1000, 1000});
  for (int i = 0; i < 45; i++) {
    deslocamento.ciclo(10, odometria, nullptr);  // estimativa: 450 mm
  }
  deslocamento.motores().parar();
  DistanciasLaterais leitura = real.leitura();
  TEST_ASSERT_TRUE(leitura.frenteMm <= DISTANCIA_FRONTAL_MAXIMA_PARA_CORRIGIR_MM);
  const ResultadoCorrecao r = deslocamento.ciclo(0, odometria, &leitura);
  TEST_ASSERT_EQUAL(ResultadoCorrecao::PelaFrente, r);
  TEST_ASSERT_INT32_WITHIN(1, 0, erroMm(odometria, real));
  TEST_ASSERT_EQUAL_UINT32(0, deslocamento.distanciaSemCorrecaoMm());
}

void test_parede_longe_nao_serve_de_referencia() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  DistanciasLaterais leitura = {500, 400, 400};  // sem parede perto e lados livres
  TEST_ASSERT_EQUAL(ResultadoCorrecao::SemReferencia, deslocamento.ciclo(10, odometria, &leitura));
}

void test_correcao_grande_demais_e_descartada() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  // No centro de (0, 0), a face da parede mais próxima fica a 84 mm do centro (o sensor leria
  // 34 mm). Lendo 4 mm, a parede está a 54 mm do centro: o robô andou 30 mm a mais.
  DistanciasLaterais leitura = {4, 30, 30};
  TEST_ASSERT_EQUAL(ResultadoCorrecao::PelaFrente, deslocamento.ciclo(0, odometria, &leitura));
  TEST_ASSERT_INT32_WITHIN(1, 30, odometria.posicaoNorteMicrometros() / 1000);
  TEST_ASSERT_EQUAL(ResultadoCorrecao::PelaFrente, deslocamento.ultimaCorrecao().resultado);
  TEST_ASSERT_INT32_WITHIN(1000, 30000, deslocamento.ultimaCorrecao().deltaUm);

  // Com limite de 20 mm, a mesma leitura é descartada e a posição não muda.

  ConfiguracaoDeslocamento estrito = configTeste();
  estrito.correcaoMaximaMm = 20;
  DeslocamentoEstimado deslocamentoEstrito(motores, estrito);
  Odometria odometria2(odometriaTeste());
  TEST_ASSERT_EQUAL(ResultadoCorrecao::Descartada, deslocamentoEstrito.ciclo(0, odometria2, &leitura));
  TEST_ASSERT_EQUAL_INT32(0, deslocamentoEstrito.ultimaCorrecao().deltaUm);
  TEST_ASSERT_EQUAL_INT32(0, odometria2.posicaoNorteMicrometros());
}

void test_girando_nao_corrige_e_esquece_as_laterais() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  deslocamento.motores().definirPotencia({300, -300});
  DistanciasLaterais leitura = {40, 30, 30};
  TEST_ASSERT_EQUAL(ResultadoCorrecao::Girando, deslocamento.ciclo(10, odometria, &leitura));
}

/// O corretor também aceita a potência direto, para servir aos encoders reais (#188).
void test_corretor_pela_potencia_vale_sem_o_estimador() {
  CorretorDeslocamentoTof corretor(configTeste());
  Odometria odometria(odometriaTeste());
  const DistanciasLaterais leitura = {40, 30, 30};
  TEST_ASSERT_EQUAL(ResultadoCorrecao::Girando, corretor.avaliar(odometria, leitura, PotenciaRodas{-200, 200}).resultado);
  // Parado, a parede da frente ainda corrige: centro a 90 mm dela, no centro da célula 0.
  const Correcao c = corretor.avaliar(odometria, {40, 30, 30}, PotenciaRodas{0, 0});
  TEST_ASSERT_EQUAL(ResultadoCorrecao::PelaFrente, c.resultado);
}

/// `esquecerLaterais` evita a quina falsa depois de uma manobra sem leituras (#188).
void test_esquecer_laterais_evita_quina_falsa_depois_da_manobra() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  deslocamento.motores().definirPotencia({300, 300});
  const DistanciasLaterais comParede = {LONGE_FRENTE, 50, 50};
  const DistanciasLaterais semParede = {LONGE_FRENTE, 400, 50};
  deslocamento.ciclo(10, odometria, &comParede);
  deslocamento.esquecerLaterais();
  // A esquerda passou a livre, mas o estado anterior foi esquecido: não é quina.
  TEST_ASSERT_EQUAL(ResultadoCorrecao::SemReferencia, deslocamento.ciclo(10, odometria, &semParede));
}

// ---- Correção pelas quinas laterais --------------------------------------------------------------

void test_quina_lateral_corrige_na_fronteira_da_celula() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  LabirintoSimulado labirinto;
  labirinto.carregar(exemplos::LABIRINTO_4X4);
  CorredorReal real(labirinto);

  // O real anda a 80% da velocidade da tabela: sem correção, a estimativa sai na frente.
  bool corrigiuNaLateral = false;
  deslocamento.motores().definirPotencia({300, 300});
  for (uint32_t t = 10; t <= 450; t += 10) {
    real.andar(240, 10);
    DistanciasLaterais leitura = real.leitura();
    leitura.frenteMm = 500;  // ignora a frente: só a quina lateral corrige
    const ResultadoCorrecao r = deslocamento.ciclo(10, odometria, &leitura);
    if (r == ResultadoCorrecao::PelaLateral) {
      corrigiuNaLateral = true;
      // A direita sai da parede da linha 0 logo depois da coluna da fronteira, a 90 + 6 mm.
      TEST_ASSERT_INT32_WITHIN(3, 96, odometria.posicaoNorteMicrometros() / 1000);
      TEST_ASSERT_INT32_WITHIN(3, 0, erroMm(odometria, real));
    }
  }
  TEST_ASSERT_TRUE(corrigiuNaLateral);
}

void test_histerese_impede_quina_inventada_pelo_ruido() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  deslocamento.motores().definirPotencia({300, 300});
  // Parede a 30 mm; o ruído leva a leitura até 150 mm, dentro da histerese (120 a 160 mm).
  const uint16_t leituras[] = {30, 150, 30, 159, 121, 30};
  for (uint16_t direita : leituras) {
    DistanciasLaterais leitura = {500, 30, direita};
    TEST_ASSERT_EQUAL(ResultadoCorrecao::SemReferencia, deslocamento.ciclo(10, odometria, &leitura));
  }
}

void test_sem_estado_anterior_nao_corrige_pela_lateral() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  deslocamento.motores().definirPotencia({300, 300});
  DistanciasLaterais livre = {500, 400, 400};
  TEST_ASSERT_EQUAL(ResultadoCorrecao::SemReferencia, deslocamento.ciclo(10, odometria, &livre));
  // Parado, uma transição não vale (só com o robô avançando).
  deslocamento.motores().parar();
  DistanciasLaterais parede = {500, 30, 400};
  TEST_ASSERT_EQUAL(ResultadoCorrecao::SemReferencia, deslocamento.ciclo(10, odometria, &parede));
}

// ---- Corrida no corredor: com e sem ToF ---------------------------------------------------------

void test_sem_tof_a_deriva_cresce_e_a_pose_deixa_de_ser_confiavel() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  LabirintoSimulado labirinto;
  labirinto.carregar(exemplos::LABIRINTO_4X4);
  CorredorReal real(labirinto);
  // 80% da velocidade da tabela, por 1,5 s a 300 mm/s estimados: 450 mm estimados, 360 reais.
  percorrer(deslocamento, odometria, real, 300, 1500, 80, false);
  TEST_ASSERT_INT32_WITHIN(2, 90, erroMm(odometria, real));
  TEST_ASSERT_FALSE(deslocamento.poseConfiavel());
}

void test_com_tof_o_erro_fica_pequeno_ate_a_parede_da_frente() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  LabirintoSimulado labirinto;
  labirinto.carregar(exemplos::LABIRINTO_4X4);
  CorredorReal real(labirinto);
  // Mesmo erro de 20%, mas com o ToF: o real vai de (0, 0) até 30 mm antes da parede da linha 3.
  const uint32_t correcoes = percorrer(deslocamento, odometria, real, 300, 2000, 80, true);
  TEST_ASSERT_TRUE(correcoes >= 3);
  TEST_ASSERT_INT32_WITHIN(10, 0, erroMm(odometria, real));
  TEST_ASSERT_TRUE(deslocamento.poseConfiavel());
  TEST_ASSERT_EQUAL_UINT8(3, odometria.pose().celula.linha);
}

void test_reiniciar_volta_a_pose_conhecida_e_confiavel() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  deslocamento.motores().definirPotencia({1000, 1000});
  for (int i = 0; i < 50; i++) {
    deslocamento.ciclo(10, odometria, nullptr);  // 500 mm sem correção
  }
  TEST_ASSERT_FALSE(deslocamento.poseConfiavel());
  deslocamento.motores().parar();
  deslocamento.reiniciar(odometria, {{2, 1}, Direcao::Leste});
  TEST_ASSERT_TRUE(deslocamento.poseConfiavel());
  TEST_ASSERT_EQUAL_UINT8(2, odometria.pose().celula.linha);
  TEST_ASSERT_EQUAL_UINT8(1, odometria.pose().celula.coluna);
  TEST_ASSERT_EQUAL(Direcao::Leste, odometria.pose().direcao);
  TEST_ASSERT_EQUAL_INT32(0, deslocamento.encoderEsquerdo().lerContagem());
}

void test_posicao_ao_longo_do_avanco_nas_quatro_direcoes() {
  ConfiguracaoOdometria config = odometriaTeste();
  Odometria norte(config, {{2, 1}, Direcao::Norte});
  Odometria sul(config, {{2, 1}, Direcao::Sul});
  Odometria leste(config, {{2, 1}, Direcao::Leste});
  Odometria oeste(config, {{2, 1}, Direcao::Oeste});
  TEST_ASSERT_EQUAL_INT64(2 * CELULA_UM, posicaoAoLongoDoAvancoUm(norte));
  TEST_ASSERT_EQUAL_INT64(-2 * CELULA_UM, posicaoAoLongoDoAvancoUm(sul));
  TEST_ASSERT_EQUAL_INT64(1 * CELULA_UM, posicaoAoLongoDoAvancoUm(leste));
  TEST_ASSERT_EQUAL_INT64(-1 * CELULA_UM, posicaoAoLongoDoAvancoUm(oeste));
}

void test_correcao_pela_frente_virado_para_o_sul() {
  MotoresSimulados motores;
  DeslocamentoEstimado deslocamento(motores, configTeste());
  Odometria odometria(odometriaTeste());
  deslocamento.reiniciar(odometria, {{2, 0}, Direcao::Sul});
  // No centro de (2, 0) virado para o Sul, a face da parede ao Sul da célula fica a 84 mm do
  // centro, e o sensor leria 34 mm (84 − 50). Lendo 24 mm, o robô está 10 mm mais perto da parede,
  // ou seja, 10 mm ao Sul do centro.
  DistanciasLaterais leitura = {24, 30, 30};
  TEST_ASSERT_EQUAL(ResultadoCorrecao::PelaFrente, deslocamento.ciclo(0, odometria, &leitura));
  TEST_ASSERT_INT32_WITHIN(1, 2 * 180 - 10, odometria.posicaoNorteMicrometros() / 1000);
  TEST_ASSERT_EQUAL_UINT8(2, odometria.pose().celula.linha);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_tabela_zona_morta_pontos_e_interpolacao);
  RUN_TEST(test_tabela_re_e_saturacao);
  RUN_TEST(test_tabela_vazia_nao_move);
  RUN_TEST(test_tabela_acima_do_ultimo_ponto_satura_e_re_arredonda_para_baixo);
  RUN_TEST(test_integra_velocidade_vezes_tempo_em_pulsos);
  RUN_TEST(test_parar_congela_a_contagem_e_re_diminui);
  RUN_TEST(test_encoders_estimados_seguem_o_iencoder);
  RUN_TEST(test_motores_com_estimativa_repassam_ao_driver_real);
  RUN_TEST(test_odometria_avanca_uma_celula_com_encoders_estimados);
  RUN_TEST(test_odometria_gira_90_graus_com_rodas_opostas);
  RUN_TEST(test_correcao_pela_frente_alinha_a_posicao_real);
  RUN_TEST(test_parede_longe_nao_serve_de_referencia);
  RUN_TEST(test_correcao_grande_demais_e_descartada);
  RUN_TEST(test_girando_nao_corrige_e_esquece_as_laterais);
  RUN_TEST(test_corretor_pela_potencia_vale_sem_o_estimador);
  RUN_TEST(test_esquecer_laterais_evita_quina_falsa_depois_da_manobra);
  RUN_TEST(test_quina_lateral_corrige_na_fronteira_da_celula);
  RUN_TEST(test_histerese_impede_quina_inventada_pelo_ruido);
  RUN_TEST(test_sem_estado_anterior_nao_corrige_pela_lateral);
  RUN_TEST(test_sem_tof_a_deriva_cresce_e_a_pose_deixa_de_ser_confiavel);
  RUN_TEST(test_com_tof_o_erro_fica_pequeno_ate_a_parede_da_frente);
  RUN_TEST(test_reiniciar_volta_a_pose_conhecida_e_confiavel);
  RUN_TEST(test_posicao_ao_longo_do_avanco_nas_quatro_direcoes);
  RUN_TEST(test_correcao_pela_frente_virado_para_o_sul);
  return UNITY_END();
}
