#pragma once

/// @file ConfiguracaoCalibracao.h
/// @brief Parâmetros da rotina de calibração dos VL53L0X e do giroscópio (#240, HU-13 / RF-13).
///
/// Orçamento total: **1 s** (RNF-08). A metade deste arquivo cobre os ToF; a parte do giroscópio e
/// a orquestração que mede a duração ficam para a outra metade da issue (Cláudio).
///
/// Valores **provisórios**: as distâncias de referência dependem da montagem e da célula de
/// partida. Confirme na bancada (#175) antes de confiar. Para mudar um valor, altere só este
/// arquivo.

#include <stdint.h>

#include "Tipos.h"

namespace micromouse {

/// Orçamento total da autocalibração, em ms (RNF-08).
constexpr uint32_t ORCAMENTO_CALIBRACAO_MS = 1000;

/// Fatia prevista para os ToF dentro do orçamento (deixando margem para o giroscópio).
constexpr uint32_t ORCAMENTO_CALIBRACAO_TOF_MS = 400;

/// Amostras da calibração de cada ToF. Com período de medição de ~33 ms (#344), 10 amostras cabem
/// em cerca de 330 ms, dentro de `ORCAMENTO_CALIBRACAO_TOF_MS`.
constexpr uint16_t AMOSTRAS_CALIBRACAO_TOF = 10;

/// Maior diferença entre a maior e a menor amostra de um lado, em mm. Acima dela, o robô
/// provavelmente se mexeu (ou o sensor oscilou demais) e a calibração daquele lado é recusada.
constexpr uint16_t VARIACAO_MAXIMA_CALIBRACAO_TOF_MM = 8;

/// Distância nominal esperada na face de cada sensor com o robô parado e centralizado na célula
/// de partida, em mm: `DISTANCIA_CENTRO_A_PAREDE_MM` (84) − offset provisório de montagem (30) =
/// 54 mm, o mesmo da percepção (#379). Ajuste se a referência da bancada for outra.
constexpr uint16_t DISTANCIA_REFERENCIA_TOF_MM = 54;

/// Referência `0`: o lado não tem distância conhecida e fica fora da calibração (desvio zero).
constexpr uint16_t SEM_REFERENCIA_TOF_MM = 0;

/// Maior desvio aceito de um sensor, em mm. O offset de um VL53L0X fica em dezenas de mm; um
/// desvio maior quase sempre é referência errada (por exemplo, o robô não estava diante da parede
/// esperada), e corrigir por ele estragaria as leituras. Acima disso a calibração é descartada.
constexpr uint16_t DESVIO_MAXIMO_TOF_MM = 30;

/// Distâncias de referência usadas na calibração (frente, esquerda, direita), em mm.
/// `SEM_REFERENCIA_TOF_MM` (0) tira o lado da calibração.
struct ReferenciasTof {
  uint16_t frenteMm = DISTANCIA_REFERENCIA_TOF_MM;
  uint16_t esquerdaMm = DISTANCIA_REFERENCIA_TOF_MM;
  uint16_t direitaMm = DISTANCIA_REFERENCIA_TOF_MM;
};

/// Referências da calibração no boot, com o robô na célula de partida: paredes dos dois lados
/// (54 mm da face de cada sensor) e **frente sem referência**, porque a frente da célula de partida
/// é o caminho do labirinto, sem parede a uma distância conhecida.
constexpr ReferenciasTof REFERENCIAS_TOF_NA_LARGADA = {SEM_REFERENCIA_TOF_MM, DISTANCIA_REFERENCIA_TOF_MM,
                                                       DISTANCIA_REFERENCIA_TOF_MM};

/// Referências da calibração com o robô numa célula qualquer (largada em qualquer célula): nenhuma
/// parede a uma distância conhecida, então nenhum ToF entra na conta e os desvios ficam em zero.
constexpr ReferenciasTof REFERENCIAS_TOF_SEM_PAREDE = {SEM_REFERENCIA_TOF_MM, SEM_REFERENCIA_TOF_MM,
                                                       SEM_REFERENCIA_TOF_MM};

/// Parâmetros do `CalibradorTof`. Os testes alteram os campos para cobrir outros cenários.
struct ConfiguracaoCalibracaoTof {
  uint16_t amostras = AMOSTRAS_CALIBRACAO_TOF;
  uint16_t variacaoMaximaMm = VARIACAO_MAXIMA_CALIBRACAO_TOF_MM;
  uint16_t desvioMaximoMm = DESVIO_MAXIMO_TOF_MM;
  ReferenciasTof referencias = {};
};

/// Fatia prevista para o giroscópio dentro do orçamento (RNF-08).
constexpr uint32_t ORCAMENTO_CALIBRACAO_GIRO_MS = 500;

/// Amostras da calibração do giroscópio dentro da rotina de 1 s. Com intervalo de 10 ms
/// (o mesmo ciclo da IMU), 50 amostras levam 500 ms, cabendo com folga em
/// `ORCAMENTO_CALIBRACAO_MS` (1 s).
constexpr uint16_t AMOSTRAS_CALIBRACAO_BOOT_GIRO = 50;

/// Maior diferença entre a maior e a menor amostra da taxa de giro, em décimos de °/s.
/// Acima dela, o robô provavelmente se mexeu e a calibração é recusada.
constexpr uint16_t VARIACAO_MAXIMA_CALIBRACAO_GIRO_DECIMOS_DPS = 50;

/// Período nominal entre amostras do giroscópio, em ms (o mesmo ciclo da IMU / navegação).
constexpr uint32_t PERIODO_AMOSTRAGEM_GIRO_MS = 10;

/// Período nominal entre leituras dos ToF, em ms (~30 Hz do VL53L0X).
constexpr uint32_t PERIODO_AMOSTRAGEM_TOF_MS = 33;

/// Parâmetros do `CalibradorGiro`.
struct ConfiguracaoCalibracaoGiro {
  uint16_t amostras = AMOSTRAS_CALIBRACAO_BOOT_GIRO;
  uint16_t variacaoMaximaDecimosDps = VARIACAO_MAXIMA_CALIBRACAO_GIRO_DECIMOS_DPS;
};

/// Parâmetros da `RotinaCalibracao` unificada (RNF-08 / RF-13).
struct ConfiguracaoRotinaCalibracao {
  uint32_t orcamentoTotalMs = ORCAMENTO_CALIBRACAO_MS;
  ConfiguracaoCalibracaoTof tof{};
  ConfiguracaoCalibracaoGiro giro{};
};

}  // namespace micromouse
