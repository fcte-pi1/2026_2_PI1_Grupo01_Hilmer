#pragma once

/// @file ConfiguracaoBoot.h
/// @brief Parâmetros do autoteste do boot (#50): o que o barramento I²C precisa ter e em quanto tempo.
///
/// ATENÇÃO: os valores marcados como PROVISÓRIOS ainda não foram conferidos com o hardware montado.
/// Para mudar um valor, altere só este arquivo. Os endereços dos dispositivos não ficam aqui: vêm de
/// `Pinos.h`, a única fonte da pinagem e dos endereços I²C.

#include <stdint.h>

namespace micromouse {

/// Tempo máximo do boot, em ms, até o robô ficar pronto (RNF-09: "sistema pronto em até 2 s").
/// Passar do prazo **não** trava os motores: só gera um aviso no relatório do boot.
///
/// O firmware mede com o `millis()` no fim do `setup()`, que conta desde o início do programa; a ROM
/// e o bootloader do ESP32 rodam antes e não entram na conta. Para o tempo desde o ligar, meça
/// na bancada (cronômetro ou osciloscópio) e registre a diferença em `docs/07-testes/software.md`.
constexpr uint32_t TEMPO_MAXIMO_BOOT_MS = 2000;

/// Registrador `WHO_AM_I` do MPU6050 (mapa de registradores do MPU-6000/6050, seção 4.32).
constexpr uint8_t REGISTRADOR_WHO_AM_I_MPU6050 = 0x75;

/// Valor esperado no `WHO_AM_I` do MPU6050 original: 0x68, com qualquer nível no pino AD0.
///
/// PROVISÓRIO: módulos GY-521 baratos às vezes trazem clones que respondem outro valor (0x70 no
/// MPU6500, 0x72, 0x98). Se a bancada mostrar um desses (o relatório do boot traz o valor lido em
/// `mpu6050_who_am_i`), confirme com a eletrônica e troque aqui.
constexpr uint8_t WHO_AM_I_MPU6050 = 0x68;

/// Faixa de endereços varrida: 0x08 a 0x77. Abaixo e acima ficam os endereços reservados do I²C.
constexpr uint8_t PRIMEIRO_ENDERECO_VARRIDO = 0x08;
constexpr uint8_t ULTIMO_ENDERECO_VARRIDO = 0x77;

/// Quantos endereços inesperados o relatório guarda. Os seguintes só entram na contagem.
constexpr uint8_t MAXIMO_ENDERECOS_INESPERADOS = 8;

/// Parâmetros em uso pelo `AutotesteBoot`. Os valores padrão vêm das constantes acima; os testes
/// alteram os campos para cobrir outros cenários.
struct ConfiguracaoBoot {
  uint32_t tempoMaximoMs = TEMPO_MAXIMO_BOOT_MS;
  uint8_t whoAmIMpu6050 = WHO_AM_I_MPU6050;
};

}  // namespace micromouse
