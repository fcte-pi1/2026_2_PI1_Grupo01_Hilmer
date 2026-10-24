#pragma once

/// @file PoliticaReenvio.h
/// @brief Classificação das respostas do servidor e espera crescente entre tentativas (#238).

#include <stdint.h>

#include "ConfiguracaoReenvio.h"

namespace micromouse {

/// O que fazer com um lote depois da resposta do servidor.
enum class ClasseResposta : uint8_t {
  Sucesso,        ///< `2xx`: o servidor aceitou o lote.
  Repetir,        ///< Sem resposta, `408`, `429` ou `5xx`: o lote fica e é tentado de novo.
  DescartarLote,  ///< `400`, `404`, `413` ou `422`: o lote não presta e travaria a fila; é descartado.
  Pausar          ///< `401`, `403` e o que não se conhece: o envio para até alguém resolver.
};

/// Decide o destino das respostas e controla a espera entre tentativas.
///
/// Espera: depois da 1ª falha seguida, `esperaMinimaMs`; a cada nova falha, a anterior vezes
/// `fatorEspera`, até `esperaMaximaMs`. Um sucesso volta à espera mínima.
class PoliticaReenvio {
 public:
  explicit PoliticaReenvio(const ConfiguracaoReenvio& config = ConfiguracaoReenvio{});

  /// Classifica um código HTTP (zero ou negativo: sem resposta).
  static ClasseResposta classificar(int codigoHttp);

  /// Registra uma falha e devolve a espera, em ms, até a próxima tentativa.
  uint32_t registrarFalha();

  /// Registra um sucesso: a próxima falha volta a esperar o mínimo.
  void registrarSucesso();

  /// Espera que a próxima falha vai gerar, em ms.
  uint32_t proximaEsperaMs() const { return proximaEsperaMs_; }

  /// Falhas seguidas desde o último sucesso.
  uint32_t falhasSeguidas() const { return falhasSeguidas_; }

 private:
  ConfiguracaoReenvio config_;
  uint32_t proximaEsperaMs_;
  uint32_t falhasSeguidas_ = 0;
};

}  // namespace micromouse
