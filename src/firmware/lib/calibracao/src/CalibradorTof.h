#pragma once

/// @file CalibradorTof.h
/// @brief Desvio de cada VL53L0X contra uma distância conhecida, aplicado nas leituras (#240).

#include <stdint.h>

#include "ConfiguracaoCalibracao.h"
#include "Tipos.h"

namespace micromouse {

/// Situação da calibração dos três ToF.
enum class EstadoCalibracaoTof : uint8_t {
  NaoCalibrado,  ///< Nenhuma calibração feita: os desvios usados são zero.
  Calibrando,    ///< Recebendo amostras; ainda faltam algumas.
  Calibrado,     ///< Desvios medidos e em uso.
  Recusada,      ///< Algum lado variou demais (robô se mexeu): os desvios anteriores foram mantidos.
  ForaDaFaixa    ///< Algum desvio passou de `desvioMaximoMm` (referência provavelmente errada): os desvios anteriores foram mantidos.
};

/// Desvio de cada sensor, em mm: `média das amostras − distância de referência`.
/// Positivo = o sensor lê a mais do que a referência; `corrigir` subtrai o desvio.
struct DesviosTof {
  int16_t frenteMm = 0;
  int16_t esquerdaMm = 0;
  int16_t direitaMm = 0;
};

/// Estima e aplica o desvio de cada ToF com o robô parado diante de distâncias conhecidas.
///
/// 1. **Calibração:** `iniciar()` e `adicionarAmostra()` a cada leitura válida. Ao fim de
///    `amostras` leituras, o desvio de cada lado é a média arredondada menos a referência. Se em
///    qualquer lado a maior e a menor amostra diferirem mais que `variacaoMaximaMm`, a calibração
///    é recusada e os desvios anteriores continuam. Um desvio acima de `desvioMaximoMm` também
///    descarta a calibração (`ForaDaFaixa`): é sinal de referência errada. Lado com referência
///    `SEM_REFERENCIA_TOF_MM` fica fora da conta e mantém desvio zero.
/// 2. **Aplicação:** `corrigir(bruta)` devolve `bruta − desvio` por lado, saturado em `uint16_t`.
///
/// Só usa inteiros: as contas dão o mesmo resultado no computador (testes `native`) e na placa. Não depende do driver: quem chama alimenta as amostras
/// (pela `ISensorDistancia` / `ILidar` ou por leituras simuladas nos testes).
///
/// Exemplo:
/// @code
/// CalibradorTof calibrador;
/// calibrador.iniciar();
/// // robô parado, parede conhecida à frente/lados:
/// while (calibrador.estado() == EstadoCalibracaoTof::Calibrando) {
///   DistanciasLaterais leitura;
///   if (sensor.lerDistanciasLaterais(leitura)) {
///     calibrador.adicionarAmostra(leitura);
///   }
/// }
/// DistanciasLaterais corrigida = calibrador.corrigir(leituraBruta);
/// @endcode
class CalibradorTof {
 public:
  explicit CalibradorTof(const ConfiguracaoCalibracaoTof& config = ConfiguracaoCalibracaoTof{});

  /// Começa uma calibração nova, descartando as amostras anteriores. Os desvios em uso só mudam
  /// quando a calibração terminar bem.
  void iniciar();

  /// Começa a calibração com distâncias de referência explícitas (sobrescreve as da config).
  void iniciar(const ReferenciasTof& referencias);

  /// Acrescenta uma amostra (robô parado).
  /// @return O estado depois da amostra. Fora de `Calibrando`, a amostra é ignorada.
  EstadoCalibracaoTof adicionarAmostra(const DistanciasLaterais& leitura);

  /// Estado da última calibração.
  EstadoCalibracaoTof estado() const { return estado_; }

  /// Desvios em uso, em mm.
  DesviosTof desvios() const { return desvios_; }

  /// Aplica os desvios a uma leitura bruta: `corrigida = bruta − desvio` por lado.
  DistanciasLaterais corrigir(const DistanciasLaterais& bruta) const;

  /// Referências usadas na calibração em andamento (ou as da config, se ainda não iniciou).
  ReferenciasTof referencias() const { return referencias_; }

  /// Quantidade de amostras recebidas na calibração atual.
  uint16_t amostrasRecebidas() const { return amostrasRecebidas_; }

 private:
  struct AcumuladorLado {
    uint16_t amostras = 0;
    uint32_t soma = 0;
    uint16_t menor = 0;
    uint16_t maior = 0;
  };

  static uint16_t mediaArredondada(uint32_t soma, uint16_t quantidade);
  static uint16_t subtrairSaturado(uint16_t bruta, int16_t desvioMm);
  void reiniciarAcumuladores();
  bool foraDaFaixa(int16_t desvioMm) const {
    return desvioMm > static_cast<int16_t>(config_.desvioMaximoMm) || desvioMm < -static_cast<int16_t>(config_.desvioMaximoMm);
  }
  bool ladoVariouDemais(const AcumuladorLado& lado, uint16_t referenciaMm) const;
  int16_t desvioDoLado(const AcumuladorLado& lado, uint16_t referenciaMm) const;

  ConfiguracaoCalibracaoTof config_;
  ReferenciasTof referencias_{};
  EstadoCalibracaoTof estado_ = EstadoCalibracaoTof::NaoCalibrado;
  DesviosTof desvios_{};
  AcumuladorLado frente_{};
  AcumuladorLado esquerda_{};
  AcumuladorLado direita_{};
  uint16_t amostrasRecebidas_ = 0;
};

}  // namespace micromouse
