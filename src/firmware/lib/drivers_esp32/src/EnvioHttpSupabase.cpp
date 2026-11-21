#include "EnvioHttpSupabase.h"

#include <stdio.h>

#include "ContratoTelemetria.h"

namespace micromouse {

namespace {

/// Código devolvido quando a chamada nem sai do robô (URL grande demais, corpo que não cabe): conta
/// como "sem resposta" para a `PoliticaReenvio`.
constexpr int SEM_CHAMADA = -1000;

constexpr bool sucesso(int codigo) { return codigo >= 200 && codigo < 300; }

}  // namespace

EnvioHttpSupabase::EnvioHttpSupabase(const CredenciaisSupabase& credenciais, uint32_t tempoLimiteMs)
    : credenciais_(credenciais), tempoLimiteMs_(tempoLimiteMs) {}

void EnvioHttpSupabase::iniciar() {
  // Sem certificado para configurar (projeto de faculdade): o HTTPS é cifrado, mas não confere quem é o
  // servidor. Também evita a falha de validar datas, já que o ESP32 não tem relógio ao ligar.
  cliente_.setInsecure();
  http_.setReuse(true);  // mantém a conexão TLS entre um lote e outro, quando o servidor deixa
}

int EnvioHttpSupabase::postar(const char* caminho, const char* corpo, size_t tamanho, String* resposta) {
  char url[192];
  if (montarUrlFuncao(credenciais_.urlProjeto, caminho, url, sizeof(url)) == 0) {
    return SEM_CHAMADA;
  }
  if (!http_.begin(cliente_, url)) {
    return SEM_CHAMADA;
  }
  const uint16_t limite = static_cast<uint16_t>(tempoLimiteMs_ > UINT16_MAX ? UINT16_MAX : tempoLimiteMs_);
  http_.setConnectTimeout(static_cast<int32_t>(limite));
  http_.setTimeout(limite);
  http_.addHeader("Content-Type", "application/json");
  // Chave e token são opcionais: só viajam se existirem (as funções abertas não os pedem).
  if (credenciais_.chaveAnon != nullptr && credenciais_.chaveAnon[0] != '\0') {
    http_.addHeader("apikey", credenciais_.chaveAnon);
    http_.addHeader("Authorization", String("Bearer ") + credenciais_.chaveAnon);
  }
  if (credenciais_.tokenDispositivo != nullptr && credenciais_.tokenDispositivo[0] != '\0') {
    http_.addHeader("x-device-token", credenciais_.tokenDispositivo);
  }

  const int codigo = http_.POST(reinterpret_cast<uint8_t*>(const_cast<char*>(corpo)), tamanho);
  // O corpo só é lido quando alguém precisa dele: o chamador (`resposta`) ou o registro de um erro.
  ultimaResposta_ = "";
  if (codigo > 0 && (resposta != nullptr || codigo >= 400)) {
    String texto = http_.getString();
    if (codigo >= 400) {
      ultimaResposta_ = texto.substring(0, 160);
    }
    if (resposta != nullptr) {
      *resposta = texto;
    }
  }
  http_.end();
  return codigo;
}

ResultadoEnvio EnvioHttpSupabase::criarCorrida(const char* labirinto, uint32_t& corridaId) {
  ResultadoEnvio resultado;
  char corpo[48];
  const size_t tamanho = serializarCriacaoCorrida(labirinto, corpo, sizeof(corpo));
  if (tamanho == 0) {
    resultado.codigoHttp = SEM_CHAMADA;
    return resultado;
  }
  String resposta;
  resultado.codigoHttp = postar("corridas", corpo, tamanho, &resposta);
  if (sucesso(resultado.codigoHttp) && !lerIdCorrida(resposta.c_str(), corridaId)) {
    resultado.codigoHttp = 0;  // respondeu, mas sem `id`: não dá para seguir com esta resposta
  }
  return resultado;
}

ResultadoEnvio EnvioHttpSupabase::enviarLote(uint32_t corridaId, const AmostraTelemetria* amostras, uint16_t quantidade) {
  ResultadoEnvio resultado;
  const size_t tamanho = serializarLote(corridaId, amostras, quantidade, lote_, sizeof(lote_));
  if (tamanho == 0) {
    resultado.codigoHttp = SEM_CHAMADA;
    return resultado;
  }
  String resposta;
  resultado.codigoHttp = postar("telemetria", lote_, tamanho, &resposta);
  if (sucesso(resultado.codigoHttp) && !lerRespostaLote(resposta.c_str(), resultado.gravadas, resultado.existentes)) {
    // `2xx` sem as contagens: o `ControladorEnvio` vê que não fecham com o lote e tenta de novo.
    resultado.gravadas = 0;
    resultado.existentes = 0;
  }
  return resultado;
}

ResultadoEnvio EnvioHttpSupabase::definirLabirintoCorrida(uint32_t corridaId, const char* labirinto) {
  ResultadoEnvio resultado;
  char caminho[48];
  const int escritos = snprintf(caminho, sizeof(caminho), "corridas/%lu/labirinto", static_cast<unsigned long>(corridaId));
  char corpo[48];
  const size_t tamanho = serializarCriacaoCorrida(labirinto, corpo, sizeof(corpo));  // o mesmo corpo da criação
  if (escritos <= 0 || static_cast<size_t>(escritos) >= sizeof(caminho) || tamanho == 0) {
    resultado.codigoHttp = SEM_CHAMADA;
    return resultado;
  }
  resultado.codigoHttp = postar(caminho, corpo, tamanho, nullptr);
  return resultado;
}

ResultadoEnvio EnvioHttpSupabase::buscarComando(ComandoRobo& comando) {
  ResultadoEnvio resultado;
  String resposta;
  resultado.codigoHttp = postar("robo/comandos/proximo", "", 0, &resposta);
  if (sucesso(resultado.codigoHttp) && !lerComandoRobo(resposta.c_str(), comando)) {
    resultado.codigoHttp = 0;  // respondeu, mas sem um comando que o robô entenda: ignora
  }
  return resultado;
}

ResultadoEnvio EnvioHttpSupabase::enviarLogs(uint32_t corridaId, const char* const* linhas, size_t quantidade) {
  ResultadoEnvio resultado;
  const size_t tamanho = serializarLogs(corridaId, linhas, quantidade, logs_, sizeof(logs_));
  if (tamanho == 0) {
    resultado.codigoHttp = SEM_CHAMADA;
    return resultado;
  }
  resultado.codigoHttp = postar("robo/logs", logs_, tamanho, nullptr);
  return resultado;
}

ResultadoEnvio EnvioHttpSupabase::encerrarCorrida(uint32_t corridaId) {
  ResultadoEnvio resultado;
  char caminho[48];
  const int escritos = snprintf(caminho, sizeof(caminho), "corridas/%lu/encerrar", static_cast<unsigned long>(corridaId));
  char corpo[40];
  const size_t tamanho = serializarEncerramento(concluida_, corpo, sizeof(corpo));
  if (escritos <= 0 || static_cast<size_t>(escritos) >= sizeof(caminho) || tamanho == 0) {
    resultado.codigoHttp = SEM_CHAMADA;
    return resultado;
  }
  resultado.codigoHttp = postar(caminho, corpo, tamanho, nullptr);
  return resultado;
}

}  // namespace micromouse
