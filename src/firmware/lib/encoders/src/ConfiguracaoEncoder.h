#pragma once

/// @file ConfiguracaoEncoder.h
/// @brief Parâmetros de calibração dos encoders de roda.
///
/// Valores do documento "Parâmetros de Hardware" da equipe de eletrônica (17/10/2026): encoder
/// óptico de fenda com disco de 100 linhas no eixo da roda e roda de até 32 mm de diâmetro.
/// Ainda falta confirmar na bancada: o diâmetro final da roda e a distância real por pulso
/// (issue #186, critério 2). Ande uma distância conhecida, leia os pulsos e use
/// `ConversorDeslocamento::calcularMicrometrosPorPulso`.
/// Para mudar o valor, altere só este arquivo.

#include <stdint.h>

namespace micromouse {

/// Pulsos contados por volta da roda: 100 linhas do disco × 4 bordas (decodificação x4).
constexpr uint32_t PULSOS_POR_VOLTA_DA_RODA = 400;

/// Diâmetro da roda de projeto, em mm (até 32 mm, segundo a eletrônica). Com roda de 25 mm,
/// a distância por pulso cai para cerca de 196 µm.
constexpr uint32_t DIAMETRO_DA_RODA_MM = 32;

/// Distância percorrida pela roda por pulso contado (decodificação x4), em micrômetros:
/// π × 32 mm / 400 pulsos = 251,3 µm. Em inteiros, para não depender de ponto flutuante no
/// ESP32-C3, que não tem unidade de ponto flutuante.
constexpr uint32_t MICROMETROS_POR_PULSO_PROVISORIO = 251;

}  // namespace micromouse
