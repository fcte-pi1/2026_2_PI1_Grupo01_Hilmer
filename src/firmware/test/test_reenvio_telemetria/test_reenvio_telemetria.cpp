/// @file test_reenvio_telemetria.cpp
/// @brief Testes do buffer de telemetria e do reenvio sem perda nem duplicação (#238): a amostra, o
/// buffer circular, a política de reenvio e o `ControladorEnvio` com um servidor simulado que imita
/// a idempotência do backend.
/// Roda no computador: `pio test -e native -f test_reenvio_telemetria`.

#include <unity.h>

#include <cstdint>
#include <initializer_list>

#include "AmostraTelemetria.h"
#include "BufferCircular.h"
#include "ConfiguracaoReenvio.h"
#include "ControladorEnvio.h"
#include "EnvioSimulado.h"
#include "IEnvioTelemetria.h"
#include "PoliticaReenvio.h"
#include "TravaNula.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint32_t CORRIDA = 7;

AmostraTelemetria amostra(uint8_t linha = 0) {
  AmostraTelemetria a;
  a.posicaoLinha = linha;
  a.velocidadeMmPorS = 120;
  a.estado = EstadoCorrida::Explorando;
  return a;
}

/// Servidor falso, trava e controlador juntos, como na montagem real.
struct Cenario {
  EnvioSimulado servidor;
  TravaNula trava;
  ControladorEnvio controlador;

  explicit Cenario(const ConfiguracaoReenvio& config = ConfiguracaoReenvio{}) : controlador(servidor, trava, config) {
    servidor.vigiarTrava(&trava);
    controlador.iniciarCorrida(CORRIDA);
  }

  void registrar(uint32_t quantidade) {
    for (uint32_t i = 0; i < quantidade; ++i) {
      controlador.registrar(amostra());
    }
  }

  /// Roda ciclos de 100 ms até o buffer esvaziar ou acabar o tempo; devolve o instante final.
  uint32_t drenar(uint32_t inicioMs, uint32_t limiteMs = 600000) {
    uint32_t t = inicioMs;
    while (controlador.ocupacao() > 0 && t - inicioMs < limiteMs) {
      controlador.ciclo(t);
      t += 100;
    }
    return t;
  }
};

void conferirSequenciasDoServidor(const EnvioSimulado& servidor, uint32_t primeira, uint16_t quantidade) {
  TEST_ASSERT_EQUAL_UINT16(quantidade, servidor.amostrasArmazenadas());
  for (uint16_t i = 0; i < quantidade; ++i) {
    TEST_ASSERT_EQUAL_UINT32(primeira + i, servidor.armazenada(i).numeroSequencia);
  }
}

}  // namespace

// ---------------------------------------------------------------- AmostraTelemetria

void test_nomes_do_estado_e_do_evento_batem_com_o_banco() {
  TEST_ASSERT_EQUAL_STRING("aguardando", nomeDoEstado(EstadoCorrida::Aguardando));
  TEST_ASSERT_EQUAL_STRING("explorando", nomeDoEstado(EstadoCorrida::Explorando));
  TEST_ASSERT_EQUAL_STRING("retornando", nomeDoEstado(EstadoCorrida::Retornando));
  TEST_ASSERT_EQUAL_STRING("parado", nomeDoEstado(EstadoCorrida::Parado));
  TEST_ASSERT_EQUAL_STRING("colisao", nomeDoEvento(EventoTelemetria::Colisao));
  TEST_ASSERT_EQUAL_STRING("objetivo_alcancado", nomeDoEvento(EventoTelemetria::ObjetivoAlcancado));
  TEST_ASSERT_NULL(nomeDoEvento(EventoTelemetria::Nenhum));
}

void test_amostra_ocupa_pouco_e_o_buffer_cabe_dez_minutos() {
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(24, sizeof(AmostraTelemetria));
  TEST_ASSERT_EQUAL_UINT16(600, CAPACIDADE_BUFFER_AMOSTRAS);  // 10 min a 1 amostra por segundo
  TEST_ASSERT_LESS_THAN_UINT32(15000, CAPACIDADE_BUFFER_AMOSTRAS * sizeof(AmostraTelemetria));
}

// ---------------------------------------------------------------- BufferCircular

void test_buffer_guarda_na_ordem_de_chegada() {
  BufferCircular<int, 4> buffer;
  TEST_ASSERT_TRUE(buffer.vazio());
  buffer.adicionar(10);
  buffer.adicionar(20);
  buffer.adicionar(30);
  int copia[4];
  uint64_t inicio = 99;
  TEST_ASSERT_EQUAL_UINT16(3, buffer.copiar(copia, 4, inicio));
  TEST_ASSERT_EQUAL_UINT64(0, inicio);
  TEST_ASSERT_EQUAL_INT(10, copia[0]);
  TEST_ASSERT_EQUAL_INT(30, copia[2]);
  TEST_ASSERT_EQUAL_UINT16(3, buffer.tamanho());  // copiar não remove
}

void test_buffer_copia_no_maximo_o_pedido() {
  BufferCircular<int, 8> buffer;
  for (int i = 1; i <= 6; ++i) buffer.adicionar(i);
  int copia[8];
  uint64_t inicio = 0;
  TEST_ASSERT_EQUAL_UINT16(4, buffer.copiar(copia, 4, inicio));
  TEST_ASSERT_EQUAL_INT(4, copia[3]);
  TEST_ASSERT_EQUAL_UINT16(0, buffer.copiar(copia, 0, inicio));
}

void test_buffer_cheio_sobrescreve_o_mais_antigo_e_conta_a_perda() {
  BufferCircular<int, 3> buffer;
  TEST_ASSERT_FALSE(buffer.adicionar(1));
  TEST_ASSERT_FALSE(buffer.adicionar(2));
  TEST_ASSERT_FALSE(buffer.adicionar(3));
  TEST_ASSERT_TRUE(buffer.cheio());
  TEST_ASSERT_TRUE(buffer.adicionar(4));  // sobrescreve o 1
  TEST_ASSERT_TRUE(buffer.adicionar(5));  // sobrescreve o 2
  TEST_ASSERT_EQUAL_UINT32(2, buffer.perdidas());
  int copia[3];
  uint64_t inicio = 0;
  TEST_ASSERT_EQUAL_UINT16(3, buffer.copiar(copia, 3, inicio));
  TEST_ASSERT_EQUAL_INT(3, copia[0]);
  TEST_ASSERT_EQUAL_INT(4, copia[1]);
  TEST_ASSERT_EQUAL_INT(5, copia[2]);
  TEST_ASSERT_EQUAL_UINT64(2, inicio);  // índice absoluto do 3
}

void test_buffer_da_a_volta_no_vetor_varias_vezes() {
  BufferCircular<int, 4> buffer;
  for (int i = 1; i <= 50; ++i) buffer.adicionar(i);
  int copia[4];
  uint64_t inicio = 0;
  buffer.copiar(copia, 4, inicio);
  TEST_ASSERT_EQUAL_INT(47, copia[0]);
  TEST_ASSERT_EQUAL_INT(50, copia[3]);
  TEST_ASSERT_EQUAL_UINT64(46, inicio);
  TEST_ASSERT_EQUAL_UINT32(46, buffer.perdidas());
  TEST_ASSERT_EQUAL_UINT64(50, buffer.totalAdicionados());
}

void test_buffer_remove_so_o_prefixo_confirmado() {
  BufferCircular<int, 8> buffer;
  for (int i = 1; i <= 5; ++i) buffer.adicionar(i);
  uint64_t inicio = 0;
  int copia[8];
  buffer.copiar(copia, 2, inicio);
  TEST_ASSERT_EQUAL_UINT16(2, buffer.removerAte(inicio + 2));
  TEST_ASSERT_EQUAL_UINT16(3, buffer.tamanho());
  buffer.copiar(copia, 8, inicio);
  TEST_ASSERT_EQUAL_INT(3, copia[0]);
  TEST_ASSERT_EQUAL_UINT64(2, inicio);
}

/// Confirmar um lote que já foi sobrescrito não apaga itens mais novos por engano.
void test_buffer_confirmacao_de_item_ja_sobrescrito_nao_remove_nada() {
  BufferCircular<int, 3> buffer;
  for (int i = 1; i <= 3; ++i) buffer.adicionar(i);
  uint64_t inicio = 0;
  int copia[3];
  buffer.copiar(copia, 2, inicio);  // lote de índices 0 e 1
  buffer.adicionar(4);              // sobrescreve o índice 0
  buffer.adicionar(5);              // sobrescreve o índice 1
  TEST_ASSERT_EQUAL_UINT16(0, buffer.removerAte(inicio + 2));  // o lote confirmado já não está lá
  TEST_ASSERT_EQUAL_UINT16(3, buffer.tamanho());
  buffer.copiar(copia, 3, inicio);
  TEST_ASSERT_EQUAL_INT(3, copia[0]);
}

void test_buffer_remover_alem_do_tamanho_limita_e_limpar_zera() {
  BufferCircular<int, 4> buffer;
  buffer.adicionar(1);
  buffer.adicionar(2);
  TEST_ASSERT_EQUAL_UINT16(2, buffer.removerAte(1000));
  TEST_ASSERT_TRUE(buffer.vazio());
  for (int i = 0; i < 10; ++i) buffer.adicionar(i);
  buffer.limpar();
  TEST_ASSERT_TRUE(buffer.vazio());
  TEST_ASSERT_EQUAL_UINT32(0, buffer.perdidas());
  TEST_ASSERT_EQUAL_UINT64(0, buffer.totalAdicionados());
  TEST_ASSERT_EQUAL_UINT16(4, (BufferCircular<int, 4>::capacidade()));
}

// ---------------------------------------------------------------- PoliticaReenvio

void test_classificacao_das_respostas() {
  for (int codigo : {200, 201, 204, 299}) {
    TEST_ASSERT_EQUAL(ClasseResposta::Sucesso, PoliticaReenvio::classificar(codigo));
  }
  for (int codigo : {0, -1, -11, 408, 429, 500, 502, 503, 599}) {
    TEST_ASSERT_EQUAL(ClasseResposta::Repetir, PoliticaReenvio::classificar(codigo));
  }
  for (int codigo : {400, 404, 413, 422}) {
    TEST_ASSERT_EQUAL(ClasseResposta::DescartarLote, PoliticaReenvio::classificar(codigo));
  }
  for (int codigo : {100, 301, 401, 403, 405, 409, 418}) {
    TEST_ASSERT_EQUAL(ClasseResposta::Pausar, PoliticaReenvio::classificar(codigo));
  }
}

void test_espera_dobra_ate_o_teto_e_volta_ao_minimo_no_sucesso() {
  PoliticaReenvio politica;
  TEST_ASSERT_EQUAL_UINT32(1000, politica.proximaEsperaMs());
  const uint32_t esperadas[] = {1000, 2000, 4000, 8000, 16000, 30000, 30000, 30000};
  for (uint32_t esperada : esperadas) {
    TEST_ASSERT_EQUAL_UINT32(esperada, politica.registrarFalha());
  }
  TEST_ASSERT_EQUAL_UINT32(8, politica.falhasSeguidas());
  politica.registrarSucesso();
  TEST_ASSERT_EQUAL_UINT32(0, politica.falhasSeguidas());
  TEST_ASSERT_EQUAL_UINT32(1000, politica.registrarFalha());
}

void test_espera_vem_da_configuracao() {
  ConfiguracaoReenvio config;
  config.esperaMinimaMs = 200;
  config.esperaMaximaMs = 500;
  config.fatorEspera = 3;
  PoliticaReenvio politica(config);
  TEST_ASSERT_EQUAL_UINT32(200, politica.registrarFalha());
  TEST_ASSERT_EQUAL_UINT32(500, politica.registrarFalha());  // 600 passa do teto de 500
  TEST_ASSERT_EQUAL_UINT32(500, politica.registrarFalha());
}

// ---------------------------------------------------------------- ControladorEnvio: coleta

/// A coleta dá os números de sequência, 1, 2, 3..., e nunca usa a rede.
void test_registrar_numera_na_coleta_e_nunca_chama_a_rede() {
  Cenario c;
  TEST_ASSERT_EQUAL_UINT32(1, c.controlador.registrar(amostra()));
  AmostraTelemetria com_numero_errado = amostra();
  com_numero_errado.numeroSequencia = 999;
  TEST_ASSERT_EQUAL_UINT32(2, c.controlador.registrar(com_numero_errado));  // o número vem do controlador
  c.registrar(10);
  TEST_ASSERT_EQUAL_UINT16(12, c.controlador.ocupacao());
  TEST_ASSERT_EQUAL_UINT32(0, c.servidor.chamadasLote());
  TEST_ASSERT_EQUAL_UINT32(0, c.servidor.chamadasEncerramento());
}

void test_sem_corrida_ou_sem_amostras_o_ciclo_fica_ocioso() {
  EnvioSimulado servidor;
  TravaNula trava;
  ControladorEnvio sem_corrida(servidor, trava);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Ocioso, sem_corrida.ciclo(0));

  Cenario c;
  TEST_ASSERT_EQUAL(ResultadoCiclo::Ocioso, c.controlador.ciclo(0));
  TEST_ASSERT_EQUAL_UINT32(0, c.servidor.chamadasLote());
}

// ---------------------------------------------------------------- ControladorEnvio: envio

void test_lote_confirmado_sai_do_buffer_na_ordem() {
  Cenario c;
  c.registrar(5);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(0));
  TEST_ASSERT_EQUAL_UINT16(0, c.controlador.ocupacao());
  conferirSequenciasDoServidor(c.servidor, 1, 5);
  TEST_ASSERT_EQUAL_UINT32(5, c.controlador.contadores().amostrasConfirmadas);
  TEST_ASSERT_EQUAL_UINT32(0, c.controlador.contadores().falhas);
}

void test_lotes_de_no_maximo_100_amostras() {
  Cenario c;
  c.registrar(250);
  uint32_t t = 0;
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(t));
  TEST_ASSERT_EQUAL_UINT16(100, c.servidor.ultimoTamanhoLote());
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(t));  // sem esperar: há atraso a recuperar
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(t));
  TEST_ASSERT_EQUAL_UINT16(50, c.servidor.ultimoTamanhoLote());
  TEST_ASSERT_EQUAL_UINT16(100, c.servidor.maiorLote());
  TEST_ASSERT_EQUAL(ResultadoCiclo::Ocioso, c.controlador.ciclo(t));
  conferirSequenciasDoServidor(c.servidor, 1, 250);
  TEST_ASSERT_EQUAL_UINT32(3, c.servidor.chamadasLote());
}

void test_o_tamanho_do_lote_vem_da_configuracao() {
  ConfiguracaoReenvio config;
  config.tamanhoMaximoLote = 30;
  Cenario c(config);
  c.registrar(70);
  c.drenar(0);
  TEST_ASSERT_EQUAL_UINT16(30, c.servidor.maiorLote());
  TEST_ASSERT_EQUAL_UINT32(3, c.servidor.chamadasLote());  // 30 + 30 + 10
}

// ---------------------------------------------------------------- ControladorEnvio: falhas de rede

/// Espera crescente: antes da hora, o ciclo não chama a rede; um sucesso volta a espera ao mínimo.
void test_espera_crescente_entre_tentativas_e_volta_ao_minimo() {
  Cenario c;
  c.servidor.definirOffline(true);
  c.registrar(1);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Falhou, c.controlador.ciclo(0));          // próxima em 1 000
  TEST_ASSERT_EQUAL(ResultadoCiclo::Aguardando, c.controlador.ciclo(500));
  TEST_ASSERT_EQUAL_UINT32(1, c.servidor.chamadasLote());                       // não chamou a rede
  TEST_ASSERT_EQUAL(ResultadoCiclo::Falhou, c.controlador.ciclo(1000));       // próxima em 3 000
  TEST_ASSERT_EQUAL(ResultadoCiclo::Aguardando, c.controlador.ciclo(2999));
  TEST_ASSERT_EQUAL(ResultadoCiclo::Falhou, c.controlador.ciclo(3000));       // próxima em 7 000
  TEST_ASSERT_EQUAL(ResultadoCiclo::Aguardando, c.controlador.ciclo(6999));
  TEST_ASSERT_EQUAL_UINT32(3, c.servidor.chamadasLote());

  c.servidor.definirOffline(false);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(7000));

  // Nova falha depois do sucesso: a espera volta a 1 s.
  c.servidor.definirOffline(true);
  c.registrar(1);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Falhou, c.controlador.ciclo(7000));
  TEST_ASSERT_EQUAL(ResultadoCiclo::Aguardando, c.controlador.ciclo(7999));
  TEST_ASSERT_EQUAL(ResultadoCiclo::Falhou, c.controlador.ciclo(8000));
}

void test_o_relogio_que_da_a_volta_nao_trava_a_espera() {
  Cenario c;
  c.servidor.definirOffline(true);
  c.registrar(1);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Falhou, c.controlador.ciclo(0xFFFFFF00u));    // próxima em 0x2E8 (depois da volta)
  TEST_ASSERT_EQUAL(ResultadoCiclo::Aguardando, c.controlador.ciclo(0xFFFFFFF0u));
  c.servidor.definirOffline(false);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Aguardando, c.controlador.ciclo(0x2E7));
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(0x2E8));
}

/// Com o robô ligado há muito tempo (relógio acima de 2^31 ms, cerca de 25 dias), a primeira tentativa
/// não pode esperar: nenhuma falha anterior impõe espera.
void test_primeira_tentativa_com_o_relogio_alto_nao_espera() {
  Cenario c;
  c.registrar(3);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(0xC0000000u));
}

/// Integração: a corrida de 120 amostras, com a rede fora do ar no meio. Depois da volta, o servidor
/// tem exatamente as 120, na ordem e sem duplicata, e a coleta nunca esperou pela rede.
void test_integracao_rede_fora_no_meio_da_corrida_nao_perde_nem_duplica() {
  Cenario c;
  uint32_t registradas = 0;
  for (uint32_t t = 0; t <= 200000; t += 100) {
    if (t % 1000 == 0 && registradas < 120) {
      const uint32_t chamadasAntes = c.servidor.chamadasLote();
      c.controlador.registrar(amostra());
      ++registradas;
      TEST_ASSERT_EQUAL_UINT32(chamadasAntes, c.servidor.chamadasLote());  // a coleta nunca usa a rede
    }
    c.servidor.definirOffline(t >= 40000 && t < 80000);
    c.controlador.ciclo(t);
  }
  TEST_ASSERT_EQUAL_UINT16(0, c.controlador.ocupacao());
  conferirSequenciasDoServidor(c.servidor, 1, 120);
  TEST_ASSERT_EQUAL_UINT32(0, c.servidor.duplicatasIgnoradas());
  TEST_ASSERT_EQUAL_UINT32(120, c.controlador.contadores().amostrasConfirmadas);
  TEST_ASSERT_EQUAL_UINT32(0, c.controlador.amostrasPerdidas());
  TEST_ASSERT_GREATER_THAN_UINT32(0, c.controlador.contadores().falhas);
  TEST_ASSERT_GREATER_THAN_UINT32(0, c.controlador.contadores().reenvios);
}

/// Integração, o caso difícil: o servidor grava o lote, mas a resposta se perde. O reenvio leva os
/// mesmos números de sequência e o servidor ignora tudo: nenhuma duplicata.
void test_integracao_resposta_perdida_nao_duplica() {
  Cenario c;
  c.registrar(40);
  c.servidor.perderProximasRespostas(1);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Falhou, c.controlador.ciclo(0));
  TEST_ASSERT_EQUAL_UINT16(40, c.servidor.amostrasArmazenadas());  // o servidor já tem
  TEST_ASSERT_EQUAL_UINT16(40, c.controlador.ocupacao());           // o robô não sabe: ainda guarda

  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(1000));
  TEST_ASSERT_EQUAL_UINT16(0, c.controlador.ocupacao());
  conferirSequenciasDoServidor(c.servidor, 1, 40);
  TEST_ASSERT_EQUAL_UINT32(40, c.servidor.duplicatasIgnoradas());
  TEST_ASSERT_EQUAL_UINT32(40, c.controlador.contadores().amostrasJaExistentes);
  TEST_ASSERT_EQUAL_UINT32(0, c.controlador.contadores().amostrasConfirmadas);
  TEST_ASSERT_EQUAL_UINT32(1, c.controlador.contadores().reenvios);
}

/// Buffer cheio durante a falta de rede: perdem-se só as mais antigas, contadas, e o resto chega
/// em ordem.
void test_integracao_buffer_cheio_perde_so_as_mais_antigas() {
  Cenario c;
  c.servidor.definirOffline(true);
  c.registrar(700);  // 100 a mais que os 600 da capacidade
  TEST_ASSERT_EQUAL_UINT16(600, c.controlador.ocupacao());
  TEST_ASSERT_EQUAL_UINT32(100, c.controlador.amostrasPerdidas());
  c.servidor.definirOffline(false);
  c.drenar(0);
  conferirSequenciasDoServidor(c.servidor, 101, 600);  // as sequências 101 a 700
  TEST_ASSERT_EQUAL_UINT32(600, c.controlador.contadores().amostrasConfirmadas);
}

/// A coleta continua enquanto a chamada de rede está em andamento e sobrescreve as mais antigas:
/// a confirmação do lote não pode apagar as amostras novas.
class EnvioQueColetaDuranteAChamada : public IEnvioTelemetria {
 public:
  ControladorEnvio* controlador = nullptr;
  uint32_t coletarDurante = 0;

  ResultadoEnvio enviarLote(uint32_t, const AmostraTelemetria*, uint16_t quantidade) override {
    for (uint32_t i = 0; i < coletarDurante; ++i) {
      controlador->registrar(amostra());
    }
    return ResultadoEnvio{200, quantidade, 0};
  }
  ResultadoEnvio encerrarCorrida(uint32_t) override { return ResultadoEnvio{200, 0, 0}; }
};

void test_integracao_confirmacao_nao_apaga_amostras_novas() {
  EnvioQueColetaDuranteAChamada envio;
  TravaNula trava;
  ControladorEnvio controlador(envio, trava);
  envio.controlador = &controlador;
  controlador.iniciarCorrida(CORRIDA);
  for (int i = 0; i < 600; ++i) controlador.registrar(amostra());  // buffer cheio

  envio.coletarDurante = 150;  // enquanto o lote de 100 segue, entram 150 amostras novas
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, controlador.ciclo(0));
  // As 150 mais antigas já tinham sido sobrescritas, inclusive o lote enviado: nada mais sai.
  TEST_ASSERT_EQUAL_UINT16(600, controlador.ocupacao());
  TEST_ASSERT_EQUAL_UINT32(150, controlador.amostrasPerdidas());
}

// ---------------------------------------------------------------- ControladorEnvio: respostas do servidor

void test_lote_recusado_e_descartado_e_a_fila_segue() {
  Cenario c;
  c.registrar(3);
  c.servidor.responderProximasCom(400, 1);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Descartado, c.controlador.ciclo(0));
  TEST_ASSERT_EQUAL_UINT16(0, c.controlador.ocupacao());
  TEST_ASSERT_EQUAL_UINT32(3, c.controlador.contadores().amostrasDescartadas);

  c.registrar(2);  // a fila não ficou presa: o próximo lote sai sem esperar
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(0));
  conferirSequenciasDoServidor(c.servidor, 4, 2);
}

void test_erro_do_servidor_e_limite_de_requisicoes_sao_tentados_de_novo() {
  for (int codigo : {500, 503, 429, 408}) {
    Cenario c;
    c.registrar(3);
    c.servidor.responderProximasCom(codigo, 2);
    TEST_ASSERT_EQUAL(ResultadoCiclo::Falhou, c.controlador.ciclo(0));
    TEST_ASSERT_EQUAL_UINT16(3, c.controlador.ocupacao());
    TEST_ASSERT_EQUAL(ResultadoCiclo::Falhou, c.controlador.ciclo(1000));
    TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(3000));
    conferirSequenciasDoServidor(c.servidor, 1, 3);
  }
}

void test_nao_autorizado_pausa_o_envio_ate_retomar() {
  Cenario c;
  c.registrar(3);
  c.servidor.responderProximasCom(401, 1);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Pausado, c.controlador.ciclo(0));
  TEST_ASSERT_TRUE(c.controlador.pausado());
  TEST_ASSERT_EQUAL_UINT16(3, c.controlador.ocupacao());  // nada se perde

  TEST_ASSERT_EQUAL(ResultadoCiclo::Pausado, c.controlador.ciclo(100000));
  TEST_ASSERT_EQUAL_UINT32(1, c.servidor.chamadasLote());  // parado: não insiste

  c.controlador.retomar();
  TEST_ASSERT_FALSE(c.controlador.pausado());
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(100000));
  conferirSequenciasDoServidor(c.servidor, 1, 3);
}

/// Resposta `2xx` cujas contagens não fecham com o tamanho do lote: não dá para saber o que o servidor
/// guardou, então o lote fica e é reenviado.
class EnvioInconsistente : public IEnvioTelemetria {
 public:
  ResultadoEnvio enviarLote(uint32_t, const AmostraTelemetria*, uint16_t) override { return ResultadoEnvio{200, 1, 0}; }
  ResultadoEnvio encerrarCorrida(uint32_t) override { return ResultadoEnvio{200, 0, 0}; }
};

void test_resposta_que_nao_fecha_com_o_tamanho_do_lote_nao_remove_nada() {
  EnvioInconsistente envio;
  TravaNula trava;
  ControladorEnvio controlador(envio, trava);
  controlador.iniciarCorrida(CORRIDA);
  for (int i = 0; i < 3; ++i) controlador.registrar(amostra());
  TEST_ASSERT_EQUAL(ResultadoCiclo::Falhou, controlador.ciclo(0));
  TEST_ASSERT_EQUAL_UINT16(3, controlador.ocupacao());
  TEST_ASSERT_EQUAL_UINT32(1, controlador.contadores().respostasInconsistentes);
}

// ---------------------------------------------------------------- ControladorEnvio: encerramento

/// Integração: a corrida só é encerrada depois do último lote confirmado.
void test_integracao_encerramento_so_depois_do_ultimo_lote() {
  Cenario c;
  c.registrar(250);
  c.controlador.solicitarEncerramento(0);

  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(0));
  TEST_ASSERT_EQUAL_UINT32(0, c.servidor.chamadasEncerramento());  // ainda há amostras
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(0));
  TEST_ASSERT_EQUAL(ResultadoCiclo::Enviado, c.controlador.ciclo(0));
  TEST_ASSERT_EQUAL_UINT32(0, c.servidor.chamadasEncerramento());
  TEST_ASSERT_EQUAL(ResultadoCiclo::Encerrada, c.controlador.ciclo(0));

  TEST_ASSERT_TRUE(c.controlador.encerrada());
  TEST_ASSERT_TRUE(c.servidor.corridaEncerrada());
  TEST_ASSERT_EQUAL_UINT16(250, c.servidor.amostrasAoEncerrar());  // o servidor já tinha tudo
  TEST_ASSERT_EQUAL_UINT32(1, c.servidor.chamadasEncerramento());

  TEST_ASSERT_EQUAL(ResultadoCiclo::Encerrada, c.controlador.ciclo(1000));
  TEST_ASSERT_EQUAL_UINT32(1, c.servidor.chamadasEncerramento());  // não repete
}

void test_encerramento_sem_amostras_pendentes_sai_na_hora() {
  Cenario c;
  c.registrar(3);
  c.controlador.ciclo(0);
  c.controlador.solicitarEncerramento(0);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Encerrada, c.controlador.ciclo(0));
}

/// A chamada de encerramento também tenta de novo, com a mesma espera crescente, se a rede falhar.
void test_encerramento_sem_rede_tenta_de_novo_depois_da_espera() {
  Cenario c;
  c.servidor.definirOffline(true);
  c.controlador.solicitarEncerramento(0);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Falhou, c.controlador.ciclo(0));
  TEST_ASSERT_EQUAL(ResultadoCiclo::Aguardando, c.controlador.ciclo(999));
  c.servidor.definirOffline(false);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Encerrada, c.controlador.ciclo(1000));
  TEST_ASSERT_EQUAL_UINT32(2, c.servidor.chamadasEncerramento());
}

/// Pedir o encerramento duas vezes não reinicia a contagem do atraso.
void test_pedir_o_encerramento_de_novo_nao_reinicia_o_atraso() {
  Cenario c;
  c.servidor.definirOffline(true);
  c.controlador.solicitarEncerramento(0);
  c.controlador.solicitarEncerramento(9000);
  TEST_ASSERT_TRUE(c.controlador.encerramentoAtrasado(10001));  // conta a partir do primeiro pedido
}

/// Sem rede, o encerramento fica pendente e marca o atraso depois do tempo limite, mas as tentativas
/// continuam e terminam quando a rede volta.
void test_encerramento_atrasado_continua_tentando_ate_conseguir() {
  Cenario c;
  c.servidor.definirOffline(true);
  c.registrar(5);
  c.controlador.solicitarEncerramento(0);
  c.drenar(0, 12000);
  TEST_ASSERT_FALSE(c.controlador.encerrada());
  TEST_ASSERT_FALSE(c.controlador.encerramentoAtrasado(10000));  // exatamente no limite
  TEST_ASSERT_TRUE(c.controlador.encerramentoAtrasado(10001));

  c.servidor.definirOffline(false);
  uint32_t t = 40000;  // depois do teto da espera
  while (!c.controlador.encerrada() && t < 200000) {
    c.controlador.ciclo(t);
    t += 100;
  }
  TEST_ASSERT_TRUE(c.controlador.encerrada());
  TEST_ASSERT_FALSE(c.controlador.encerramentoAtrasado(t));
  conferirSequenciasDoServidor(c.servidor, 1, 5);
  TEST_ASSERT_EQUAL_UINT16(5, c.servidor.amostrasAoEncerrar());
}

/// Corrida que o servidor não conhece: o encerramento não adianta insistir.
void test_encerramento_de_corrida_inexistente_nao_insiste() {
  Cenario c;
  c.servidor.marcarCorridaInexistente(CORRIDA);
  c.controlador.solicitarEncerramento(0);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Descartado, c.controlador.ciclo(0));
  TEST_ASSERT_TRUE(c.controlador.encerrada());
  c.controlador.ciclo(1000);
  TEST_ASSERT_EQUAL_UINT32(1, c.servidor.chamadasEncerramento());
}

void test_encerramento_com_credencial_recusada_pausa() {
  Cenario c;
  c.servidor.responderProximasCom(401, 1);
  c.controlador.solicitarEncerramento(0);
  TEST_ASSERT_EQUAL(ResultadoCiclo::Pausado, c.controlador.ciclo(0));
  c.controlador.retomar();
  TEST_ASSERT_EQUAL(ResultadoCiclo::Encerrada, c.controlador.ciclo(0));
}

// ---------------------------------------------------------------- ControladorEnvio: trava e nova corrida

/// A trava só é segurada em cópias curtas: nunca durante a chamada de rede, nunca aninhada, e sempre
/// liberada.
void test_a_trava_nunca_e_segurada_durante_a_rede() {
  Cenario c;
  c.servidor.definirOffline(true);
  c.registrar(30);
  c.drenar(0, 20000);
  c.servidor.definirOffline(false);
  c.controlador.solicitarEncerramento(0);
  c.drenar(30000);
  for (uint32_t t = 100000; !c.controlador.encerrada() && t < 130000; t += 100) {
    c.controlador.ciclo(t);
  }
  TEST_ASSERT_TRUE(c.controlador.encerrada());
  TEST_ASSERT_GREATER_THAN_UINT32(0, c.servidor.chamadasLote());
  TEST_ASSERT_EQUAL_UINT32(0, c.servidor.chamadasComTravaSegura());
  TEST_ASSERT_EQUAL_INT32(0, c.trava.profundidade());
  TEST_ASSERT_EQUAL_INT32(1, c.trava.maiorProfundidade());
}

void test_nova_corrida_limpa_buffer_numeracao_e_contadores() {
  Cenario c;
  c.registrar(20);
  c.servidor.definirOffline(true);
  c.controlador.ciclo(0);
  c.controlador.solicitarEncerramento(0);

  c.controlador.iniciarCorrida(8);
  TEST_ASSERT_EQUAL_UINT16(0, c.controlador.ocupacao());
  TEST_ASSERT_EQUAL_UINT32(0, c.controlador.contadores().tentativas);
  TEST_ASSERT_FALSE(c.controlador.encerramentoSolicitado());
  TEST_ASSERT_FALSE(c.controlador.encerrada());
  TEST_ASSERT_FALSE(c.controlador.pausado());
  TEST_ASSERT_EQUAL_UINT32(1, c.controlador.registrar(amostra()));  // a numeração recomeça em 1
  TEST_ASSERT_EQUAL_UINT32(1000, c.controlador.proximaEsperaMs());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_nomes_do_estado_e_do_evento_batem_com_o_banco);
  RUN_TEST(test_amostra_ocupa_pouco_e_o_buffer_cabe_dez_minutos);
  RUN_TEST(test_buffer_guarda_na_ordem_de_chegada);
  RUN_TEST(test_buffer_copia_no_maximo_o_pedido);
  RUN_TEST(test_buffer_cheio_sobrescreve_o_mais_antigo_e_conta_a_perda);
  RUN_TEST(test_buffer_da_a_volta_no_vetor_varias_vezes);
  RUN_TEST(test_buffer_remove_so_o_prefixo_confirmado);
  RUN_TEST(test_buffer_confirmacao_de_item_ja_sobrescrito_nao_remove_nada);
  RUN_TEST(test_buffer_remover_alem_do_tamanho_limita_e_limpar_zera);
  RUN_TEST(test_classificacao_das_respostas);
  RUN_TEST(test_espera_dobra_ate_o_teto_e_volta_ao_minimo_no_sucesso);
  RUN_TEST(test_espera_vem_da_configuracao);
  RUN_TEST(test_registrar_numera_na_coleta_e_nunca_chama_a_rede);
  RUN_TEST(test_sem_corrida_ou_sem_amostras_o_ciclo_fica_ocioso);
  RUN_TEST(test_lote_confirmado_sai_do_buffer_na_ordem);
  RUN_TEST(test_lotes_de_no_maximo_100_amostras);
  RUN_TEST(test_o_tamanho_do_lote_vem_da_configuracao);
  RUN_TEST(test_espera_crescente_entre_tentativas_e_volta_ao_minimo);
  RUN_TEST(test_o_relogio_que_da_a_volta_nao_trava_a_espera);
  RUN_TEST(test_primeira_tentativa_com_o_relogio_alto_nao_espera);
  RUN_TEST(test_integracao_rede_fora_no_meio_da_corrida_nao_perde_nem_duplica);
  RUN_TEST(test_integracao_resposta_perdida_nao_duplica);
  RUN_TEST(test_integracao_buffer_cheio_perde_so_as_mais_antigas);
  RUN_TEST(test_integracao_confirmacao_nao_apaga_amostras_novas);
  RUN_TEST(test_lote_recusado_e_descartado_e_a_fila_segue);
  RUN_TEST(test_erro_do_servidor_e_limite_de_requisicoes_sao_tentados_de_novo);
  RUN_TEST(test_nao_autorizado_pausa_o_envio_ate_retomar);
  RUN_TEST(test_resposta_que_nao_fecha_com_o_tamanho_do_lote_nao_remove_nada);
  RUN_TEST(test_integracao_encerramento_so_depois_do_ultimo_lote);
  RUN_TEST(test_encerramento_sem_amostras_pendentes_sai_na_hora);
  RUN_TEST(test_encerramento_sem_rede_tenta_de_novo_depois_da_espera);
  RUN_TEST(test_pedir_o_encerramento_de_novo_nao_reinicia_o_atraso);
  RUN_TEST(test_encerramento_atrasado_continua_tentando_ate_conseguir);
  RUN_TEST(test_encerramento_de_corrida_inexistente_nao_insiste);
  RUN_TEST(test_encerramento_com_credencial_recusada_pausa);
  RUN_TEST(test_a_trava_nunca_e_segurada_durante_a_rede);
  RUN_TEST(test_nova_corrida_limpa_buffer_numeracao_e_contadores);
  return UNITY_END();
}
