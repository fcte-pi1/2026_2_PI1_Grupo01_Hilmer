#pragma once

/// @file BufferCircular.h
/// @brief Buffer circular de tamanho fixo, sem alocação dinâmica (#238).

#include <stdint.h>

namespace micromouse {

/// Guarda até `CAPACIDADE` itens, do mais antigo para o mais novo. Cheio, o item novo **sobrescreve
/// o mais antigo** e conta uma perda.
///
/// Cada item recebe um **índice absoluto** (0, 1, 2... na ordem em que entrou) que nunca se repete.
/// É por ele que o envio confirma o que o servidor aceitou: a tarefa de rede copia um lote, solta a
/// trava e faz a chamada de rede; enquanto isso a coleta pode encher o buffer e sobrescrever os itens
/// mais antigos. `removerAte(índice)` remove só o que ainda está lá, e a confirmação do lote nunca
/// apaga itens mais novos por engano.
///
/// Não é seguro entre tarefas por si só: proteja cada chamada com uma trava curta (`ITrava`).
template <typename T, uint16_t CAPACIDADE>
class BufferCircular {
  static_assert(CAPACIDADE > 0, "a capacidade deve ser positiva");

 public:
  /// Acrescenta um item no fim.
  /// @return `true` se o buffer estava cheio e o item mais antigo foi sobrescrito (uma perda).
  bool adicionar(const T& item) {
    const bool cheio = tamanho_ == CAPACIDADE;
    if (cheio) {
      itens_[inicio_] = item;
      inicio_ = proximo(inicio_);
      ++perdidas_;
    } else {
      itens_[(inicio_ + tamanho_) % CAPACIDADE] = item;
      ++tamanho_;
    }
    ++totalAdicionados_;
    return cheio;
  }

  /// Copia, sem remover, até `maximo` itens, do mais antigo para o mais novo.
  /// @param[out] destino Recebe os itens.
  /// @param[out] indiceInicial Índice absoluto do primeiro item copiado.
  /// @return Quantos itens foram copiados.
  uint16_t copiar(T* destino, uint16_t maximo, uint64_t& indiceInicial) const {
    indiceInicial = indiceDoMaisAntigo();
    const uint16_t quantidade = tamanho_ < maximo ? tamanho_ : maximo;
    for (uint16_t i = 0; i < quantidade; ++i) {
      destino[i] = itens_[(inicio_ + i) % CAPACIDADE];
    }
    return quantidade;
  }

  /// Remove os itens de índice absoluto menor que `indiceFim` que ainda estão no buffer.
  /// @return Quantos foram removidos (os que já tinham sido sobrescritos não contam).
  uint16_t removerAte(uint64_t indiceFim) {
    const uint64_t antigo = indiceDoMaisAntigo();
    if (indiceFim <= antigo) {
      return 0;
    }
    const uint64_t pedidos = indiceFim - antigo;
    const uint16_t remover = pedidos > tamanho_ ? tamanho_ : static_cast<uint16_t>(pedidos);
    inicio_ = static_cast<uint16_t>((inicio_ + remover) % CAPACIDADE);
    tamanho_ = static_cast<uint16_t>(tamanho_ - remover);
    return remover;
  }

  /// Esvazia o buffer e zera os contadores (nova corrida).
  void limpar() {
    inicio_ = 0;
    tamanho_ = 0;
    totalAdicionados_ = 0;
    perdidas_ = 0;
  }

  uint16_t tamanho() const { return tamanho_; }
  bool vazio() const { return tamanho_ == 0; }
  bool cheio() const { return tamanho_ == CAPACIDADE; }
  static constexpr uint16_t capacidade() { return CAPACIDADE; }

  /// Itens sobrescritos antes de serem confirmados, desde `limpar()`.
  uint32_t perdidas() const { return perdidas_; }

  /// Quantos itens já entraram, desde `limpar()`. É também o índice absoluto do próximo item.
  uint64_t totalAdicionados() const { return totalAdicionados_; }

  /// Índice absoluto do item mais antigo que ainda está no buffer.
  uint64_t indiceDoMaisAntigo() const { return totalAdicionados_ - tamanho_; }

 private:
  static uint16_t proximo(uint16_t i) { return static_cast<uint16_t>((i + 1) % CAPACIDADE); }

  T itens_[CAPACIDADE];
  uint16_t inicio_ = 0;
  uint16_t tamanho_ = 0;
  uint64_t totalAdicionados_ = 0;
  uint32_t perdidas_ = 0;
};

}  // namespace micromouse
