#pragma once

/// @file BarramentoI2c.h
/// @brief O barramento I²C compartilhado (3 VL53L0X e o MPU6050), com exclusão mútua entre as
/// tarefas (#378).
///
/// Duas tarefas usam o mesmo barramento: a navegação (os ToF) e a tarefa da IMU (o MPU6050). Uma
/// transação I²C não pode ser interrompida por outra no meio, senão os bytes de um sensor se
/// misturam com os do outro. Por isso toda transação passa por um **mutex do FreeRTOS**.
///
/// Por que mutex, e não seção crítica (`portENTER_CRITICAL`): a seção crítica desliga as
/// interrupções, e o driver I²C do ESP32 depende delas e pode esperar dentro da transação. Com o
/// mutex, quem espera dorme, e a herança de prioridade evita que uma tarefa de prioridade alta
/// fique presa atrás de uma de prioridade baixa.

#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "Configuracao.h"

namespace micromouse {

/// Dono do `TwoWire` e do mutex do barramento.
///
/// Uso, em qualquer tarefa que fale com um dispositivo I²C:
/// @code
/// BarramentoI2c::Uso uso(barramento);
/// if (!uso.conseguiu()) { return false; }   // outra tarefa segurou o barramento por tempo demais
/// barramento.wire().beginTransmission(endereco);
/// // ...
/// @endcode
/// O barramento é liberado quando `uso` sai do escopo.
class BarramentoI2c {
 public:
  /// Segura o barramento enquanto existir.
  class Uso {
   public:
    /// @param barramento Barramento a usar.
    /// @param esperaMaximaMs Quanto esperar se outra tarefa estiver usando.
    explicit Uso(BarramentoI2c& barramento, uint32_t esperaMaximaMs = ESPERA_MAXIMA_I2C_MS)
        : barramento_(barramento), conseguiu_(barramento.travar(esperaMaximaMs)) {}
    ~Uso() {
      if (conseguiu_) {
        barramento_.destravar();
      }
    }
    Uso(const Uso&) = delete;
    Uso& operator=(const Uso&) = delete;

    /// `false` se o barramento não foi conseguido a tempo (ou não foi iniciado): não use o `Wire`.
    bool conseguiu() const { return conseguiu_; }

   private:
    BarramentoI2c& barramento_;
    bool conseguiu_;
  };

  /// @param wire A interface I²C do ESP32 (normalmente `Wire`).
  explicit BarramentoI2c(TwoWire& wire) : wire_(wire) {}

  /// Cria o mutex e inicia o `Wire` nos pinos dados. Chame uma vez no `setup()`, antes de iniciar
  /// qualquer dispositivo do barramento.
  /// @return `false` se o mutex não pôde ser criado ou o `Wire` não iniciou.
  bool iniciar(int pinoSda, int pinoScl, uint32_t frequenciaHz = FREQUENCIA_I2C_HZ);

  /// `true` depois de um `iniciar()` bem-sucedido.
  bool iniciado() const { return mutex_ != nullptr; }

  /// O `TwoWire` do barramento. Só use dentro de um `Uso` bem-sucedido.
  TwoWire& wire() { return wire_; }

  /// Vezes em que uma tarefa desistiu de esperar o barramento.
  uint32_t esperasEsgotadas() const { return esperasEsgotadas_; }

  /// Segura o barramento, esperando até `esperaMaximaMs`. Prefira o `Uso`, que libera sozinho.
  bool travar(uint32_t esperaMaximaMs);

  /// Libera o barramento segurado por `travar()`.
  void destravar();

 private:
  TwoWire& wire_;
  SemaphoreHandle_t mutex_ = nullptr;
  volatile uint32_t esperasEsgotadas_ = 0;
};

}  // namespace micromouse
