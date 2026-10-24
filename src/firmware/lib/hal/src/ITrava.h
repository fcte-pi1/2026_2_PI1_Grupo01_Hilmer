#pragma once

/// @file ITrava.h
/// @brief Exclusão mútua entre as tarefas, sem depender do FreeRTOS na lógica (camada `hal`).

namespace micromouse {

/// Trava curta, usada em volta de acessos rápidos a dados compartilhados entre tarefas. Nunca
/// segure a trava durante uma chamada de rede ou qualquer espera.
///
/// Implementações:
/// - `TravaNula` (`lib/simulacao`): não trava nada, só conta os pares, para os testes.
/// - Seção crítica do FreeRTOS (`portENTER_CRITICAL`): a ser criada na ligação com o `main.cpp`.
class ITrava {
 public:
  virtual ~ITrava() = default;
  virtual void travar() = 0;
  virtual void destravar() = 0;
};

}  // namespace micromouse
