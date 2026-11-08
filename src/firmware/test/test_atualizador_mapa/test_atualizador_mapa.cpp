#include "../test_labirinto_simulado/LabirintosExemplo.h"
#include "AtualizadorMapa.h"
#include "LabirintoSimulado.h"
#include <unity.h>

using namespace micromouse;

LabirintoSimulado labirinto;

void setUp() { labirinto.carregar(exemplos::LABIRINTO_4X4); }
void tearDown() {}

// ─── Auxiliares ──────────────────────────────────────────────────────────────

/// Simula uma leitura dos sensores a partir do labirinto e atualiza o mapa.
static ResultadoAtualizacao atualizarComSimulacao(Mapa &mapa, const Pose &pose, const ClassificadorParede &classificador) {
  DistanciasLaterais dist;
  bool ok = labirinto.distancias(pose, dist);
  TEST_ASSERT_TRUE_MESSAGE(ok, "distancias() retornou false para pose valida");
  return AtualizadorMapa::atualizar(mapa, pose, classificador, &dist, 1);
}

// ─── Testes ──────────────────────────────────────────────────────────────────

/// @brief Cenário 1: Registrar uma parede interior (não-borda) deve retornar
/// ResultadoRegistro::Registrada. O Mapa preenche o perímetro no construtor
/// (paredes de borda já começam como Parede), por isso usamos uma pose que
/// observa uma parede interior desconhecida.
///
/// Como funciona:
/// 1. Posiciona o robô na célula (1,0) orientado para o Norte.
/// 2. Executa uma leitura simulada. A parede à esquerda (Oeste) é a borda do labirinto.
///    A parede à direita (Leste) é uma parede interior que ainda não foi registada.
/// 3. Verifica se a atualização à esquerda devolve 'JaConhecida' (borda pré-preenchida).
/// 4. Verifica se a atualização à direita devolve 'Registrada' (nova parede descoberta).
/// 5. Assegura que não ocorreram conflitos durante o processo.
void test_registro_nova_parede() {
  Mapa mapa;
  ClassificadorParede classificador;

  // (1,0) Norte: frente=450→Livre, esquerda=90→Parede(borda Oeste→JaConhecida),
  //              direita=270→Livre (parede Leste de (1,0): interior,
  //              desconhecida → Registrada)
  Pose pose = {{1, 0}, Direcao::Norte};
  ResultadoAtualizacao res = atualizarComSimulacao(mapa, pose, classificador);

  // A borda Oeste já é conhecida (preenchida pelo construtor do Mapa)
  TEST_ASSERT_EQUAL(ResultadoRegistro::JaConhecida, res.esquerda);
  // A parede Leste de (1,0) é interior: deve ser registrada agora
  TEST_ASSERT_EQUAL(ResultadoRegistro::Registrada, res.direita);
  TEST_ASSERT_FALSE(res.temConflito);
}

/// @brief Cenário 2: Registrar a mesma parede duas vezes deve retornar JaConhecida na segunda vez.
///
/// Como funciona:
/// 1. Posiciona o robô virtualmente na célula (0,0) virado para o Norte.
/// 2. Executa uma primeira leitura do sensor simulado, forçando o registro inicial da parede.
/// 3. Executa uma segunda leitura exatamente na mesma pose.
/// 4. Verifica (Assert) se o resultado devolve 'JaConhecida' para a esquerda e para a direita,
///    e garante que a flag de conflito se mantém falsa.
void test_registro_parede_ja_conhecida() {
  Mapa mapa;
  ClassificadorParede classificador;

  Pose pose = {{0, 0}, Direcao::Norte};
  atualizarComSimulacao(mapa, pose, classificador); // 1ª vez: Registrada
  ResultadoAtualizacao res = atualizarComSimulacao(mapa, pose, classificador); // 2ª vez: JaConhecida

  TEST_ASSERT_EQUAL(ResultadoRegistro::JaConhecida, res.esquerda);
  TEST_ASSERT_EQUAL(ResultadoRegistro::JaConhecida, res.direita);
  TEST_ASSERT_FALSE(res.temConflito);
}

/// @brief Cenário 3: Verificar que não há conflito ao percorrer múltiplas células do labirinto.
///
/// Como funciona:
/// 1. Define um percurso de 5 posições distintas dentro do labirinto (0,0 N; 1,0 N; 3,0 L; 2,2 N; 0,1 L).
/// 2. Itera sobre cada uma destas posições.
/// 3. Em cada iteração, realiza uma simulação de leitura e atualização do mapa.
/// 4. Verifica se a propriedade 'temConflito' do resultado de cada atualização é sempre Falsa,
///    garantindo que o mapa permanece consistente ao longo de um trajeto válido.
void test_sem_conflito_ao_percorrer_labirinto() {
  Mapa mapa;
  ClassificadorParede classificador;

  // Poses do labirinto 4×4 das leituras documentadas em LabirintosExemplo.h
  const Pose poses[] = {
      {{0, 0}, Direcao::Norte}, {{1, 0}, Direcao::Norte},
      {{3, 0}, Direcao::Leste}, {{2, 2}, Direcao::Norte},
      {{0, 1}, Direcao::Leste},
  };

  for (const Pose& poseAtual : poses) {
    ResultadoAtualizacao res = atualizarComSimulacao(mapa, poseAtual, classificador);
    TEST_ASSERT_FALSE_MESSAGE(res.temConflito, "Conflito detectado ao percorrer labirinto consistente");
  }
}

/// @brief Cenário 4: Verificar que o mapa não é atualizado com estado Desconhecido
/// (leitura acima do limiar quando classificador usa limiar zero).
///
/// Como funciona:
/// 1. Cria um objeto `ResultadoClassificacao` sem o inicializar com distâncias válidas.
///    Por omissão, isto deixa os estados de frente, esquerda e direita como 'Desconhecido'.
/// 2. Tenta atualizar o mapa (na pose 0,0 Norte) injetando este resultado forçosamente.
/// 3. O comportamento esperado do `AtualizadorMapa` é rejeitar estas classificações inválidas.
/// 4. Verifica se os registos de frente, esquerda e direita retornam o enum `Invalida`.
void test_parede_desconhecida_nao_registrada() {
  Mapa mapa;
  // Limiar 0 → qualquer distância > 0 é classificada como Livre ou Desconhecido
  // Usamos a sobrecarga que recebe ResultadoClassificacao diretamente.
  ClassificadorParede::ResultadoClassificacao classif;
  // Deixa tudo como Desconhecido (padrão da struct)

  Pose pose = {{0, 0}, Direcao::Norte};
    ResultadoAtualizacao res = AtualizadorMapa::atualizar(mapa, pose, classif);

  TEST_ASSERT_EQUAL(ResultadoRegistro::Invalida, res.frente);
  TEST_ASSERT_EQUAL(ResultadoRegistro::Invalida, res.esquerda);
  TEST_ASSERT_EQUAL(ResultadoRegistro::Invalida, res.direita);
  TEST_ASSERT_FALSE(res.temConflito);
}

/// @brief Cenário 5: Verificar que paredes registradas a partir de células vizinhas coincidem (sem conflito).
///
/// Como funciona:
/// 1. Prepara duas posições vizinhas lado a lado: poseA em (0,1) Norte e poseB em (0,0) Norte.
/// 2. Ambas as posições partilham a mesma parede vertical divisória: a parede Oeste de (0,1) 
///    é fisicamente a mesma que a parede Leste de (0,0).
/// 3. Executa a leitura simulada a partir da poseA (que regista a parede à sua esquerda) e atualiza o mapa.
/// 4. Executa a leitura simulada a partir da poseB (que regista a parede à sua direita) e atualiza o mapa.
/// 5. Verifica se nenhuma das duas atualizações gerou a flag de conflito, confirmando
///    que o algoritmo lida bem com a leitura da mesma barreira a partir de células diferentes.
void test_parede_compartilhada_sem_conflito() {
  Mapa mapa;
  ClassificadorParede classificador;

  // Célula (0,1) Norte: a parede à esquerda (Oeste de 0,1) é a mesma barreira
  // que a parede à direita da Célula (0,0) Norte (Leste de 0,0).
  // Aqui verificamos que registar a mesma parede partilhada a partir de
  // duas poses vizinhas distintas não gera conflito.
  Pose poseA = {{0, 1}, Direcao::Norte}; // esquerda -> regista a parede Oeste de (0,1)
  Pose poseB = {{0, 0}, Direcao::Norte}; // direita  -> regista a parede Leste de (0,0)

  ResultadoAtualizacao resA = atualizarComSimulacao(mapa, poseA, classificador);
  ResultadoAtualizacao resB = atualizarComSimulacao(mapa, poseB, classificador);

  TEST_ASSERT_FALSE(resA.temConflito);
  TEST_ASSERT_FALSE(resB.temConflito);
}

/// @brief Cenário 6: Percorre todas as células do labirinto 4×4 nas 4 orientações e verifica
/// que o mapa final é idêntico ao labirinto simulado (critério de pronto da issue #177).
///
/// Como funciona:
/// 1. Inicia um loop pelas 'linhas' e 'colunas' de todo o labirinto (dimensão 4x4).
/// 2. Em cada célula, itera através das 4 orientações cardeais (N, S, E, O).
/// 3. Em cada orientação, o robô faz uma leitura e atualiza o seu mapa interno. Isto força
///    a exploração total (100% de cobertura).
/// 4. Após o mapeamento concluído, inicia um novo duplo loop (linhas x colunas x 4 direções).
/// 5. Compara a existência de cada parede registada no mapa interno (`mapa.obterParede`) com a parede
///    real do labirinto simulado (`labirinto.temParede`).
/// 6. O teste passa apenas se o mapa construído for um reflexo exato do ambiente simulado.
void test_mapa_final_identico_ao_labirinto_4x4() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  ClassificadorParede classificador;

  const uint8_t linhas = labirinto.obterLinhas();
  const uint8_t colunas = labirinto.obterColunas();

  // Visita cada célula nas 4 orientações para garantir que o robô "enxerga"
  // as paredes de todos os ângulos possíveis.
  const Direcao orientacoes[] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};
  for (uint8_t l = 0; l < linhas; l++) {
    for (uint8_t c = 0; c < colunas; c++) {
      for (Direcao dir : orientacoes) {
        atualizarComSimulacao(mapa, {{l, c}, dir}, classificador);
      }
    }
  }

  // Compara cada parede do mapa com o labirinto de referência.
  const Direcao direcoes[] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};
  for (uint8_t l = 0; l < linhas; l++) {
    for (uint8_t c = 0; c < colunas; c++) {
      for (Direcao dir : direcoes) {
        EstadoParede esperado = labirinto.temParede({l, c}, dir)
                                    ? EstadoParede::Parede
                                    : EstadoParede::Livre;
        TEST_ASSERT_EQUAL_MESSAGE(esperado, mapa.obterParede({l, c}, dir), "Parede diverge entre o mapa e o labirinto simulado");
      }
    }
  }
}

/// @brief Cenário 7: Registrar uma parede conflitante deve retornar Conflito,
/// marcar temConflito como verdadeiro e preservar o valor anterior do mapa.
///
/// Como funciona:
/// 1. Posiciona o robô na célula interior (1,1) virado para o Norte.
/// 2. Executa a primeira atualização gravando a parede Norte como 'Parede'.
/// 3. Prepara uma segunda classificação contraditória indicando que a frente agora está 'Livre'.
/// 4. Executa a atualização e verifica se o retorno da frente devolve 'Conflito'.
/// 5. Garante que 'temConflito' é verdadeiro e que o mapa não foi sobrescrito (continua 'Parede').
void test_conflito_preserva_mapa_e_sinaliza_tem_conflito() {
  Mapa mapa;
  Pose pose = {{1, 1}, Direcao::Norte};

  // 1ª vez: registra a parede Norte como Parede
  ClassificadorParede::ResultadoClassificacao classif1;
  classif1.frente = EstadoParede::Parede;
  ResultadoAtualizacao res1 = AtualizadorMapa::atualizar(mapa, pose, classif1);
  TEST_ASSERT_EQUAL(ResultadoRegistro::Registrada, res1.frente);
  TEST_ASSERT_FALSE(res1.temConflito);
  TEST_ASSERT_EQUAL(EstadoParede::Parede, mapa.obterParede({1, 1}, Direcao::Norte));

  // 2ª vez: leitura contraditória indica que a mesma parede é Livre
  ClassificadorParede::ResultadoClassificacao classif2;
  classif2.frente = EstadoParede::Livre;
  ResultadoAtualizacao res2 = AtualizadorMapa::atualizar(mapa, pose, classif2);

  TEST_ASSERT_EQUAL(ResultadoRegistro::Conflito, res2.frente);
  TEST_ASSERT_TRUE(res2.temConflito);
  // O mapa deve preservar a informação anterior
  TEST_ASSERT_EQUAL(EstadoParede::Parede, mapa.obterParede({1, 1}, Direcao::Norte));
}

/// @brief Cenário 8: Validar que as direções relativas (frente, esquerda, direita)
/// são convertidas corretamente para direções cardeais absolutas (N, L, S, O)
/// nas 4 orientações do robô.
///
/// Como funciona:
/// 1. Itera pelas 4 orientações possíveis (Norte, Leste, Sul, Oeste) em uma célula interior (1,1).
/// 2. Em cada orientação, injeta uma classificação onde apenas a Frente é 'Parede' e as laterais são 'Livre'.
/// 3. Atualiza um mapa isolado para cada orientação.
/// 4. Valida se a 'Parede' e as passagens 'Livre' foram registradas nas direções absolutas esperadas:
///    - Robô virado para Norte: Frente=Norte, Esquerda=Oeste, Direita=Leste.
///    - Robô virado para Leste: Frente=Leste, Esquerda=Norte, Direita=Sul.
///    - Robô virado para Sul:   Frente=Sul,   Esquerda=Leste, Direita=Oeste.
///    - Robô virado para Oeste: Frente=Oeste, Esquerda=Sul,   Direita=Norte.
void test_conversao_direcoes_nas_quatro_orientacoes() {
  ClassificadorParede::ResultadoClassificacao classif;
  classif.frente = EstadoParede::Parede;
  classif.esquerda = EstadoParede::Livre;
  classif.direita = EstadoParede::Livre;

  // Robô virado para o Norte
  {
    Mapa mapa;
    AtualizadorMapa::atualizar(mapa, {{1, 1}, Direcao::Norte}, classif);
    TEST_ASSERT_EQUAL(EstadoParede::Parede, mapa.obterParede({1, 1}, Direcao::Norte));
    TEST_ASSERT_EQUAL(EstadoParede::Livre, mapa.obterParede({1, 1}, Direcao::Oeste));
    TEST_ASSERT_EQUAL(EstadoParede::Livre, mapa.obterParede({1, 1}, Direcao::Leste));
  }

  // Robô virado para o Leste
  {
    Mapa mapa;
    AtualizadorMapa::atualizar(mapa, {{1, 1}, Direcao::Leste}, classif);
    TEST_ASSERT_EQUAL(EstadoParede::Parede, mapa.obterParede({1, 1}, Direcao::Leste));
    TEST_ASSERT_EQUAL(EstadoParede::Livre, mapa.obterParede({1, 1}, Direcao::Norte));
    TEST_ASSERT_EQUAL(EstadoParede::Livre, mapa.obterParede({1, 1}, Direcao::Sul));
  }

  // Robô virado para o Sul
  {
    Mapa mapa;
    AtualizadorMapa::atualizar(mapa, {{1, 1}, Direcao::Sul}, classif);
    TEST_ASSERT_EQUAL(EstadoParede::Parede, mapa.obterParede({1, 1}, Direcao::Sul));
    TEST_ASSERT_EQUAL(EstadoParede::Livre, mapa.obterParede({1, 1}, Direcao::Leste));
    TEST_ASSERT_EQUAL(EstadoParede::Livre, mapa.obterParede({1, 1}, Direcao::Oeste));
  }

  // Robô virado para o Oeste
  {
    Mapa mapa;
    AtualizadorMapa::atualizar(mapa, {{1, 1}, Direcao::Oeste}, classif);
    TEST_ASSERT_EQUAL(EstadoParede::Parede, mapa.obterParede({1, 1}, Direcao::Oeste));
    TEST_ASSERT_EQUAL(EstadoParede::Livre, mapa.obterParede({1, 1}, Direcao::Sul));
    TEST_ASSERT_EQUAL(EstadoParede::Livre, mapa.obterParede({1, 1}, Direcao::Norte));
  }
}

/// @brief Cenário 9: Atualizar o mapa com pose fora das dimensões deve retornar
/// ResultadoRegistro::Invalida sem gerar exceções ou corromper a memória.
///
/// Como funciona:
/// 1. Cria um mapa com dimensões 4x4.
/// 2. Define uma pose com célula inválida fora do mapa (ex: linha 10, coluna 10).
/// 3. Tenta realizar a atualização com classificações válidas de parede.
/// 4. Assegura que todas as direções retornam 'Invalida' e 'temConflito' permanece falso.
void test_pose_fora_do_mapa_retorna_invalida() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Pose poseInvalida = {{10, 10}, Direcao::Norte};

  ClassificadorParede::ResultadoClassificacao classif;
  classif.frente = EstadoParede::Parede;
  classif.esquerda = EstadoParede::Parede;
  classif.direita = EstadoParede::Parede;

  ResultadoAtualizacao res = AtualizadorMapa::atualizar(mapa, poseInvalida, classif);

  TEST_ASSERT_EQUAL(ResultadoRegistro::Invalida, res.frente);
  TEST_ASSERT_EQUAL(ResultadoRegistro::Invalida, res.esquerda);
  TEST_ASSERT_EQUAL(ResultadoRegistro::Invalida, res.direita);
  TEST_ASSERT_FALSE(res.temConflito);
}

/// @brief Cenário 10: Verificar a sobrecarga que recebe múltiplas amostras de distâncias
/// e utiliza o ClassificadorParede para inferir o estado antes de atualizar.
///
/// Como funciona:
/// 1. Prepara um array com 3 amostras contendo ruído artificial de medição.
///    A distância frontal indica parede próxima (~85 mm) e as laterais indicam caminho livre (~400 mm).
/// 2. Executa a sobrecarga de 'AtualizadorMapa::atualizar' passando as 3 amostras e o classificador.
/// 3. Confirma que os registros foram efetuados com sucesso ('Registrada').
/// 4. Valida se o mapa armazenou 'Parede' ao Norte e 'Livre' ao Oeste e Leste da célula (1,1).
void test_sobrecarga_multiplas_amostras_com_classificador() {
  Mapa mapa;
  ClassificadorParede classificador;
  Pose pose = {{1, 1}, Direcao::Norte};

  // 3 amostras com pequenas variações de leitura (frente próxima, laterais livres)
  DistanciasLaterais amostras[3] = {
      {80, 400, 420},
      {85, 390, 410},
      {90, 410, 400}
  };

  ResultadoAtualizacao res = AtualizadorMapa::atualizar(mapa, pose, classificador, amostras, 3);

  TEST_ASSERT_EQUAL(ResultadoRegistro::Registrada, res.frente);
  TEST_ASSERT_EQUAL(ResultadoRegistro::Registrada, res.esquerda);
  TEST_ASSERT_EQUAL(ResultadoRegistro::Registrada, res.direita);
  TEST_ASSERT_FALSE(res.temConflito);

  TEST_ASSERT_EQUAL(EstadoParede::Parede, mapa.obterParede({1, 1}, Direcao::Norte));
  TEST_ASSERT_EQUAL(EstadoParede::Livre, mapa.obterParede({1, 1}, Direcao::Leste));
  TEST_ASSERT_EQUAL(EstadoParede::Livre, mapa.obterParede({1, 1}, Direcao::Oeste));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_registro_nova_parede);
  RUN_TEST(test_registro_parede_ja_conhecida);
  RUN_TEST(test_sem_conflito_ao_percorrer_labirinto);
  RUN_TEST(test_parede_desconhecida_nao_registrada);
  RUN_TEST(test_parede_compartilhada_sem_conflito);
  RUN_TEST(test_mapa_final_identico_ao_labirinto_4x4);
  RUN_TEST(test_conflito_preserva_mapa_e_sinaliza_tem_conflito);
  RUN_TEST(test_conversao_direcoes_nas_quatro_orientacoes);
  RUN_TEST(test_pose_fora_do_mapa_retorna_invalida);
  RUN_TEST(test_sobrecarga_multiplas_amostras_com_classificador);
  return UNITY_END();
}