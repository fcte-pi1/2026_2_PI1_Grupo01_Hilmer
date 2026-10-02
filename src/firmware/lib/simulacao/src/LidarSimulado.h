#pragma once

#include <stdint.h>

#include "ILidar.h"

namespace micromouse {

// LiDAR falso para testes: devolve as leituras que o teste configurar, sem hardware.
class LidarSimulado : public ILidar {
 public:
  static constexpr uint8_t CAPACIDADE_FILA = 16;

  // Leitura devolvida sempre que a fila estiver vazia.
  void definirLeituraFixa(const DistanciasLaterais& leitura) {
    leituraFixa_ = leitura;
    temLeituraFixa_ = true;
  }

  // Leituras entregues uma vez cada, em ordem. Retorna false se a fila estiver cheia.
  bool enfileirarLeitura(const DistanciasLaterais& leitura) {
    if (quantidade_ == CAPACIDADE_FILA) {
      return false;
    }
    fila_[(inicio_ + quantidade_) % CAPACIDADE_FILA] = leitura;
    quantidade_++;
    return true;
  }

  // Enquanto verdadeiro, toda leitura falha (sensor sem resposta) e a fila não é consumida.
  void simularFalha(bool falha) { falha_ = falha; }

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
    if (temLeituraFixa_) {
      saida = leituraFixa_;
      return true;
    }
    return false;
  }

 private:
  DistanciasLaterais fila_[CAPACIDADE_FILA] = {};
  uint8_t inicio_ = 0;
  uint8_t quantidade_ = 0;
  DistanciasLaterais leituraFixa_ = {};
  bool temLeituraFixa_ = false;
  bool falha_ = false;
};

}  // namespace micromouse
