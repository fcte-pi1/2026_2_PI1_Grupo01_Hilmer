#pragma once

/// @file LabirintosExemplo.h
/// @brief Um labirinto de exemplo de cada tamanho da competição, no formato de `LabirintoSimulado`.
///
/// Os três têm o perímetro fechado e todas as células alcançáveis a partir da partida (0, 0).

namespace exemplos {

/// Labirinto 4×4 (72 × 72 cm).
///
/// Leituras esperadas (frente, esquerda, direita, em mm), conferidas à mão:
/// | Pose (linha, coluna, direção) | Leitura          |
/// |-------------------------------|------------------|
/// | (0, 0) Norte                  | {630, 90, 90}    |
/// | (1, 0) Norte                  | {450, 90, 270}   |
/// | (3, 0) Leste                  | {270, 90, 630}   |
/// | (2, 2) Norte                  | {270, 270, 90}   |
/// | (0, 1) Leste                  | {450, 90, 90}    |
constexpr const char* LABIRINTO_4X4 = R"(+---+---+---+---+
|       |       |
+   +---+   +   +
|   |       |   |
+   +   +---+   +
|       |       |
+   +---+   +---+
|   |           |
+---+---+---+---+)";

/// Labirinto 8×4 (144 × 72 cm): 8 colunas e 4 linhas.
constexpr const char* LABIRINTO_8X4 = R"(+---+---+---+---+---+---+---+---+
|               |               |
+   +---+---+   +   +---+---+   +
|   |       |       |       |   |
+   +   +   +---+---+   +   +   +
|   |   |                   |   |
+   +   +---+---+---+---+   +   +
|       |                       |
+---+---+---+---+---+---+---+---+)";

/// Labirinto 12×4 (216 × 72 cm): 12 colunas e 4 linhas.
constexpr const char* LABIRINTO_12X4 = R"(+---+---+---+---+---+---+---+---+---+---+---+---+
|                       |                       |
+   +---+---+---+---+   +   +---+---+---+---+   +
|   |               |       |               |   |
+   +   +---+---+   +---+---+   +---+---+   +   +
|   |   |                           |   |   |   |
+   +   +   +---+---+---+---+---+   +   +   +   +
|       |                       |       |       |
+---+---+---+---+---+---+---+---+---+---+---+---+)";

}  // namespace exemplos
