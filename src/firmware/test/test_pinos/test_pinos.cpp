/// @file test_pinos.cpp
/// @brief Testes da pinagem Rev 2.0 (`Pinos.h`, #377): conferem as constantes contra o esquemático
/// e protegem contra GPIO repetido, GPIO inexistente e uso dos pinos reservados.
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

void test_pinagem_confere_com_o_esquematico_rev2() {
  TEST_ASSERT_EQUAL_INT(9, pinos::I2C_SDA);
  TEST_ASSERT_EQUAL_INT(8, pinos::I2C_SCL);
  TEST_ASSERT_EQUAL_INT(0, pinos::XSHUT_ESQ);
  TEST_ASSERT_EQUAL_INT(1, pinos::XSHUT_FRONTAL);
  TEST_ASSERT_EQUAL_INT(2, pinos::XSHUT_DIR);
  TEST_ASSERT_EQUAL_INT(10, pinos::MOTOR_ESQ_IN1);
  TEST_ASSERT_EQUAL_INT(3, pinos::MOTOR_ESQ_IN2);
  TEST_ASSERT_EQUAL_INT(20, pinos::MOTOR_DIR_IN3);
  TEST_ASSERT_EQUAL_INT(21, pinos::MOTOR_DIR_IN4);
  TEST_ASSERT_EQUAL_INT(4, pinos::ENCODER_ESQ_A);
  TEST_ASSERT_EQUAL_INT(5, pinos::ENCODER_ESQ_B);
  TEST_ASSERT_EQUAL_INT(6, pinos::ENCODER_DIR_A);
  TEST_ASSERT_EQUAL_INT(7, pinos::ENCODER_DIR_B);
  TEST_ASSERT_EQUAL_INT(18, pinos::USB_DM);
  TEST_ASSERT_EQUAL_INT(19, pinos::USB_DP);
}

void test_nenhum_gpio_e_usado_duas_vezes() {
  TEST_ASSERT_TRUE(pinos::pinosSemRepeticao());
  for (size_t i = 0; i < pinos::QUANTIDADE_DE_PINOS; i++) {
    TEST_ASSERT_EQUAL_INT(1, vezesEmUso(pinos::TODOS_OS_PINOS[i]));
  }
}

void test_a_lista_cobre_os_15_gpios_do_esp32c3() {
  // Todos os GPIOs do ESP32-C3 (0 a 10 e 18 a 21) estão em uso na Rev 2.0: não sobra nenhum livre.
  TEST_ASSERT_EQUAL_UINT32(15, static_cast<uint32_t>(pinos::QUANTIDADE_DE_PINOS));
  TEST_ASSERT_TRUE(pinos::pinosExistemNoEsp32C3());
}

void test_detecta_gpio_repetido() {
  const int com_repeticao[] = {0, 1, 2, 1};
  const int sem_repeticao[] = {0, 1, 2, 3};
  TEST_ASSERT_FALSE(pinos::semRepeticao(com_repeticao, 4));
  TEST_ASSERT_TRUE(pinos::semRepeticao(sem_repeticao, 4));
  TEST_ASSERT_TRUE(pinos::semRepeticao(sem_repeticao, 0));
}

void test_usb_nativa_fica_reservada_para_o_log() {
  // GPIO18 e GPIO19 só podem ser a USB: o Serial sai por eles.
  TEST_ASSERT_EQUAL_INT(1, vezesEmUso(18));
  TEST_ASSERT_EQUAL_INT(1, vezesEmUso(19));
  TEST_ASSERT_EQUAL_INT(18, pinos::USB_DM);
  TEST_ASSERT_EQUAL_INT(19, pinos::USB_DP);
}

void test_uart0_so_alimenta_o_driver_dos_motores() {
  // GPIO20/21 (UART0) são as entradas IN3/IN4 do driver; nada de log neles.
  TEST_ASSERT_EQUAL_INT(pinos::MOTOR_DIR_IN3, 20);
  TEST_ASSERT_EQUAL_INT(pinos::MOTOR_DIR_IN4, 21);
  TEST_ASSERT_EQUAL_INT(1, vezesEmUso(20));
  TEST_ASSERT_EQUAL_INT(1, vezesEmUso(21));
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
  RUN_TEST(test_pinagem_confere_com_o_esquematico_rev2);
  RUN_TEST(test_nenhum_gpio_e_usado_duas_vezes);
  RUN_TEST(test_a_lista_cobre_os_15_gpios_do_esp32c3);
  RUN_TEST(test_detecta_gpio_repetido);
  RUN_TEST(test_usb_nativa_fica_reservada_para_o_log);
  RUN_TEST(test_uart0_so_alimenta_o_driver_dos_motores);
  RUN_TEST(test_sensores_i2c_e_encoders_nao_dividem_pino);
  RUN_TEST(test_enderecos_dos_vl53l0x_conferem_com_a_issue_344);
  return UNITY_END();
}
