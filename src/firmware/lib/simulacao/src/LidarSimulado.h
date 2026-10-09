#pragma once

/// @file LidarSimulado.h
/// @brief Implementação falsa de `ILidar` para testes sem hardware.

#include <stdint.h>

#include "ILidar.h"
#include "LabirintoSimulado.h"

namespace micromouse {

/// LiDAR falso para testes: devolve as leituras que o teste configurar, sem hardware.
///
/// Ordem de prioridade a cada chamada de `lerDistanciasLaterais()`:
/// 1. se a falha estiver ativa (`simularFalha(true)`), a leitura falha;
/// 2. se houver leituras na fila, entrega a mais antiga e a remove;
/// 3. se o modo labirinto estiver ativo (`usarLabirinto` e `posicionar`), entrega o que o
///    `LabirintoSimulado` mostra na pose atual (falha se a pose estiver fora do labirinto);
/// 4. se houver leitura fixa, entrega a leitura fixa;
/// 5. caso contrário, a leitura falha.
///
/// A fila é circular e de tamanho fixo (`CAPACIDADE_FILA`), sem alocação dinâmica.
///
/// Exemplo:
/// @code
/// LidarSimulado lidar;
/// lidar.definirLeituraFixa({500, 500, 500});  // corredor livre
/// lidar.enfileirarLeitura({90, 90, 270});     // depois, uma célula com parede à frente e à esquerda
/// ILidar& sensor = lidar;                     // a lógica recebe só a interface
/// @endcode
///
/// No modo labirinto, a leitura vem da posição do robô:
/// @code
/// LabirintoSimulado labirinto;
/// labirinto.carregar(texto);
/// LidarSimulado lidar;
/// lidar.usarLabirinto(labirinto);                          // o labirinto precisa continuar vivo
/// lidar.posicionar({{0, 0}, Direcao::Norte});              // lê {630, 90, 90} no labirinto 4×4
/// @endcode
class LidarSimulado : public ILidar {
 public:
  /// Quantidade máxima de leituras na fila.
  static constexpr uint8_t CAPACIDADE_FILA = 16;

  /// Define a leitura devolvida sempre que a fila estiver vazia.
  /// @param leitura Distâncias, em mm, a repetir a cada chamada.
  void definirLeituraFixa(const DistanciasLaterais& leitura) {
    leituraFixa_ = leitura;
    temLeituraFixa_ = true;
  }

  /// Acrescenta uma leitura à fila. Cada leitura enfileirada é entregue uma única vez, em ordem.
  /// @param leitura Distâncias, em mm.
  /// @return `false` se a fila estiver cheia (a leitura é descartada); `true` caso contrário.
  bool enfileirarLeitura(const DistanciasLaterais& leitura) {
    if (quantidade_ == CAPACIDADE_FILA) {
      return false;
    }
    fila_[(inicio_ + quantidade_) % CAPACIDADE_FILA] = leitura;
    quantidade_++;
    return true;
  }

  /// Passa a ler as distâncias de um labirinto, a partir da pose definida em `posicionar`.
  /// O labirinto não é copiado: ele precisa continuar existindo enquanto o sensor o usar.
  /// Sem `posicionar`, a leitura continua vindo da leitura fixa.
  /// @param labirinto Labirinto de referência, já carregado.
  void usarLabirinto(const LabirintoSimulado& labirinto) { labirinto_ = &labirinto; }

  /// Define onde o robô está, para o modo labirinto. Pode ser chamada a cada movimento do teste.
  /// @param pose Célula e direção do robô.
  void posicionar(const Pose& pose) {
    pose_ = pose;
    posicionado_ = true;
  }

  /// Liga ou desliga a simulação de sensor sem resposta.
  /// Enquanto ligada, toda leitura falha e a fila não é consumida.
  /// @param falha `true` para simular a falha; `false` para voltar ao normal.
  void simularFalha(bool falha) { falha_ = falha; }

  /// @copydoc ILidar::lerDistanciasLaterais
  bool lerDistanciasLaterais(DistanciasLaterais& saida) override {
    if (falha_) {
      return false;
    }
    if (quantidade_ > 0) {
      saida = fila_[inicio_];
      inicio_ = (inicio_ + 1) % CAPACIDADE_FILA;
      quantidade_--;
      return true;
    }
    if (labirinto_ != nullptr && posicionado_) {
      return labirinto_->distancias(pose_, saida);
    }
    if (temLeituraFixa_) {
      saida = leituraFixa_;
      return true;
    }
    return false;
  }

 private:
  DistanciasLaterais fila_[CAPACIDADE_FILA] = {};  ///< Armazenamento da fila circular.
  uint8_t inicio_ = 0;                              ///< Índice da leitura mais antiga.
  uint8_t quantidade_ = 0;                          ///< Quantidade de leituras na fila.
  DistanciasLaterais leituraFixa_ = {};             ///< Leitura usada quando a fila está vazia.
  bool temLeituraFixa_ = false;                     ///< Se uma leitura fixa foi definida.
  bool falha_ = false;                              ///< Se a falha do sensor está sendo simulada.
  const LabirintoSimulado* labirinto_ = nullptr;    ///< Labirinto do modo labirinto, se houver.
  Pose pose_ = {};                                  ///< Pose do robô no modo labirinto.
  bool posicionado_ = false;                        ///< Se `posicionar` já foi chamado.
};

}  // namespace micromouse
