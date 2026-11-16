/// @file test_pinos.cpp
/// @brief Testes da pinagem do ESP32 DevKit V1 (`Pinos.h`, #377): conferem as constantes contra a
/// tabela de fiação da bancada integrada e protegem contra GPIO repetido, GPIO inexistente na placa e
/// uso dos pinos reservados (UART0 do log e flash).
/// Roda no computador: `pio test -e native -f test_pinos`.

#include <unity.h>

#include "Pinos.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

/// Quantas vezes `pino` aparece na pinagem.
int vezesEmUso(int pino) {
  int vezes = 0;
  for (size_t i = 0; i < pinos::QUANTIDADE_DE_PINOS; i++) {
    if (pinos::TODOS_OS_PINOS[i] == pino) {
      vezes++;
    }
  }
  return vezes;
}

}  // namespace

void test_pinagem_confere_com_a_fiacao_da_bancada_integrada() {
  TEST_ASSERT_EQUAL_INT(21, pinos::I2C_SDA);
  TEST_ASSERT_EQUAL_INT(22, pinos::I2C_SCL);
  TEST_ASSERT_EQUAL_INT(16, pinos::XSHUT_ESQ);
  TEST_ASSERT_EQUAL_INT(17, pinos::XSHUT_FRONTAL);
  TEST_ASSERT_EQUAL_INT(4, pinos::XSHUT_DIR);
  TEST_ASSERT_EQUAL_INT(25, pinos::MOTOR_ESQ_IN1);
  TEST_ASSERT_EQUAL_INT(26, pinos::MOTOR_ESQ_IN2);
  TEST_ASSERT_EQUAL_INT(27, pinos::MOTOR_DIR_IN3);
  TEST_ASSERT_EQUAL_INT(14, pinos::MOTOR_DIR_IN4);
  TEST_ASSERT_EQUAL_INT(32, pinos::ENCODER_ESQ_A);
  TEST_ASSERT_EQUAL_INT(33, pinos::ENCODER_ESQ_B);
  TEST_ASSERT_EQUAL_INT(18, pinos::ENCODER_DIR_A);
  TEST_ASSERT_EQUAL_INT(19, pinos::ENCODER_DIR_B);
  TEST_ASSERT_EQUAL_INT(23, pinos::BOTAO_START);
  TEST_ASSERT_EQUAL_INT(34, pinos::BATERIA_ADC);
}

void test_nenhum_gpio_e_usado_duas_vezes() {
  TEST_ASSERT_TRUE(pinos::pinosSemRepeticao());
  for (size_t i = 0; i < pinos::QUANTIDADE_DE_PINOS; i++) {
    TEST_ASSERT_EQUAL_INT(1, vezesEmUso(pinos::TODOS_OS_PINOS[i]));
  }
}

void test_todos_os_gpios_existem_no_devkit() {
  TEST_ASSERT_EQUAL_UINT32(15, static_cast<uint32_t>(pinos::QUANTIDADE_DE_PINOS));
  TEST_ASSERT_TRUE(pinos::pinosExistemNoEsp32DevKit());
}

void test_detecta_gpio_repetido() {
  const int com_repeticao[] = {0, 1, 2, 1};
  const int sem_repeticao[] = {0, 1, 2, 3};
  TEST_ASSERT_FALSE(pinos::semRepeticao(com_repeticao, 4));
  TEST_ASSERT_TRUE(pinos::semRepeticao(sem_repeticao, 4));
  TEST_ASSERT_TRUE(pinos::semRepeticao(sem_repeticao, 0));
}

void test_uart0_e_flash_ficam_livres() {
  // GPIO1/GPIO3 são a UART0, por onde sai o log; GPIO6 a GPIO11 são a flash do módulo.
  TEST_ASSERT_EQUAL_INT(0, vezesEmUso(1));
  TEST_ASSERT_EQUAL_INT(0, vezesEmUso(3));
  for (int pino = 6; pino <= 11; pino++) {
    TEST_ASSERT_EQUAL_INT(0, vezesEmUso(pino));
  }
}

void test_entradas_sem_pull_up_so_na_bateria() {
  // GPIO34 a GPIO39 só têm entrada e nenhum pull-up interno: servem para o ADC da bateria, nunca para
  // o botão (INPUT_PULLUP), os encoders (pull-up) ou as saídas dos motores e dos XSHUT.
  for (size_t i = 0; i < pinos::QUANTIDADE_DE_PINOS; i++) {
    const int pino = pinos::TODOS_OS_PINOS[i];
    if (pino >= 34 && pino <= 39) {
      TEST_ASSERT_EQUAL_INT(pinos::BATERIA_ADC, pino);
    }
  }
}

void test_sensores_i2c_e_encoders_nao_dividem_pino() {
  // O conflito de GPIO4/5 que existia entre encoder e driver na versão anterior não pode voltar.
  const int encoders[] = {pinos::ENCODER_ESQ_A, pinos::ENCODER_ESQ_B, pinos::ENCODER_DIR_A,
                          pinos::ENCODER_DIR_B};
  const int motores[] = {pinos::MOTOR_ESQ_IN1, pinos::MOTOR_ESQ_IN2, pinos::MOTOR_DIR_IN3,
                         pinos::MOTOR_DIR_IN4};
  for (int encoder : encoders) {
    for (int motor : motores) {
      TEST_ASSERT_NOT_EQUAL_INT(encoder, motor);
    }
    TEST_ASSERT_NOT_EQUAL_INT(pinos::I2C_SDA, encoder);
    TEST_ASSERT_NOT_EQUAL_INT(pinos::I2C_SCL, encoder);
  }
}

void test_enderecos_dos_vl53l0x_conferem_com_a_issue_344() {
  TEST_ASSERT_EQUAL_HEX8(0x30, pinos::ENDERECO_TOF_ESQ);
  TEST_ASSERT_EQUAL_HEX8(0x31, pinos::ENDERECO_TOF_FRONTAL);
  TEST_ASSERT_EQUAL_HEX8(0x32, pinos::ENDERECO_TOF_DIR);
  TEST_ASSERT_TRUE(pinos::enderecosTofDistintos());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_pinagem_confere_com_a_fiacao_da_bancada_integrada);
  RUN_TEST(test_nenhum_gpio_e_usado_duas_vezes);
  RUN_TEST(test_todos_os_gpios_existem_no_devkit);
  RUN_TEST(test_detecta_gpio_repetido);
  RUN_TEST(test_uart0_e_flash_ficam_livres);
  RUN_TEST(test_entradas_sem_pull_up_so_na_bateria);
  RUN_TEST(test_sensores_i2c_e_encoders_nao_dividem_pino);
  RUN_TEST(test_enderecos_dos_vl53l0x_conferem_com_a_issue_344);
  return UNITY_END();
}
