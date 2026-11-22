#pragma once

/// @file ServicoOta.h
/// @brief Gravação do firmware pelo Wi-Fi (ArduinoOTA), para regravar o robô sem cabo.
///
/// Usado pelo robô e pela bancada geral. Depois que o Wi-Fi conecta, `iniciar()` publica a placa na
/// rede com o nome `micromouse` (mDNS) e `ciclo()` atende o computador. A senha só protege contra
/// gravação acidental na rede do laboratório; troque com `-DMICROMOUSE_SENHA_OTA=\"outra\"`.

#include <Arduino.h>

namespace micromouse {

#if !defined(MICROMOUSE_SENHA_OTA)
#define MICROMOUSE_SENHA_OTA "micromouse"
#endif

class ServicoOta {
 public:
  /// Nome da placa na rede (`<nome>.local`) quando a rede deixa o mDNS passar.
  static constexpr const char* NOME_PADRAO = "micromouse";

  /// Liga o serviço, uma vez só (chamadas seguintes não fazem nada).
  /// @param aoIniciar Chamada quando uma gravação começa: freie os motores aqui. A placa reinicia
  ///        quando a gravação termina, então não precisa desfazer nada.
  void iniciar(void (*aoIniciar)(), const char* nome = NOME_PADRAO);

  /// Atende o computador que quer gravar. Chamar com frequência (a cada 100 ms ou menos) enquanto
  /// o Wi-Fi estiver conectado; durante a gravação, a chamada só volta no fim.
  void ciclo();

  bool ativo() const { return ativo_; }

 private:
  bool ativo_ = false;
};

}  // namespace micromouse
