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

/// Pulsos contados por volta da roda, medidos no robô (decodificação x4): o motor com encoder gera
/// ~1383 pulsos por volta (o valor de projeto de 400 estava 3,45 vezes abaixo e a odometria achava
/// que o robô andava 3 células quando andava menos de uma).
constexpr uint32_t PULSOS_POR_VOLTA_DA_RODA = 1383;

/// Diâmetro da roda de projeto, em mm (até 32 mm, segundo a eletrônica). Com roda de 25 mm,
/// a distância por pulso cai para cerca de 196 µm.
constexpr uint32_t DIAMETRO_DA_RODA_MM = 32;

/// Distância percorrida pela roda por pulso contado (decodificação x4), em micrômetros. Medido:
/// o robô foi empurrado do centro da 1ª ao centro da 3ª célula (2 × 180 = 360 mm) e as rodas contaram
/// 4949 e 4961 pulsos, ou seja, 360 mm / 4955 = 72,7 µm (π × 32 mm / 1383). Em inteiros, como o resto
/// da odometria: as contas dão o mesmo resultado no computador (testes `native`) e na placa.
constexpr uint32_t MICROMETROS_POR_PULSO_PROVISORIO = 73;

}  // namespace micromouse
