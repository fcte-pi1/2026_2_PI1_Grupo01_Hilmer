#pragma once

/// @file ReconexaoWifi.h
/// @brief Conecta e reconecta o Wi-Fi com espera crescente, sem travar a tarefa de rede (#238).

#include <stdint.h>

#include "ConfiguracaoRede.h"
#include "IRedeWifi.h"
#include "PoliticaReenvio.h"

namespace micromouse {

/// Situação da conexão depois do último `ciclo()`.
enum class EstadoWifi : uint8_t {
  Aguardando,  ///< Desconectado, esperando a hora da próxima tentativa.
  Conectando,  ///< Tentativa em andamento (até `tempoLimiteConexaoMs`).
  Conectado    ///< Conectado: pode chamar o servidor.
};

/// Máquina de estados da conexão Wi-Fi, chamada a cada ciclo da tarefa de rede.
///
/// Desconectada, começa uma tentativa (`iniciarConexao()`) e volta na hora. Nos ciclos seguintes,
/// confere se conectou. Se passar `tempoLimiteConexaoMs` sem conectar, desiste dessa tentativa e
/// espera antes da próxima: 1 s, 2 s, 4 s... até 30 s (`PoliticaReenvio`). Conectar volta a espera
/// para 1 s. Se a conexão cair, a próxima tentativa começa no ciclo seguinte, na mesma rede.
///
/// Rede reserva (#491): com duas redes (`IRedeWifi::quantidadeRedes()`), a tentativa que esgota passa
/// para a outra rede e tenta de novo na hora; a espera crescente só entra depois de as duas falharem
/// seguidas. Conectado na reserva, o robô fica nela: voltar à principal derrubaria uma conexão boa.
class ReconexaoWifi {
 public:
  /// @param rede Wi-Fi da placa (ou a rede falsa dos testes).
  /// @param tempoLimiteConexaoMs Tempo máximo de uma tentativa.
  /// @param esperas Espera mínima, máxima e fator entre tentativas.
  explicit ReconexaoWifi(IRedeWifi& rede, uint32_t tempoLimiteConexaoMs = TEMPO_LIMITE_CONEXAO_WIFI_MS,
                         const ConfiguracaoReenvio& esperas = configuracaoEsperaWifi());

  /// Avança a máquina de estados. Nunca espera.
  /// @param agoraMs Relógio em ms (`millis()`), com a volta do contador tratada.
  /// @return `true` se está conectado.
  bool ciclo(uint32_t agoraMs);

  /// Recomeça do zero, pela rede principal: usado quando a rede muda em tempo de execução (a escolha
  /// de rede da bancada). Desfaz a conexão atual e zera a espera; os contadores continuam.
  void reiniciar();

  EstadoWifi estado() const { return estado_; }
  /// Tentativas de conexão começadas.
  uint32_t tentativas() const { return tentativas_; }
  /// Vezes em que a conexão caiu depois de feita.
  uint32_t quedas() const { return quedas_; }
  /// Espera que a próxima falha vai impor, em ms.
  uint32_t proximaEsperaMs() const { return esperas_.proximaEsperaMs(); }
  /// Rede da tentativa atual ou da conexão: 0 é a principal, 1 a reserva.
  uint8_t redeAtual() const { return redeAtual_; }
  /// Vezes em que a reconexão passou de uma rede para a outra.
  uint32_t trocasDeRede() const { return trocasDeRede_; }

 private:
  void comecarTentativa(uint32_t agoraMs);
  void registrarTentativaEsgotada(uint32_t agoraMs);
  void registrarConexao();

  IRedeWifi& rede_;
  uint32_t tempoLimiteConexaoMs_;
  PoliticaReenvio esperas_;
  EstadoWifi estado_ = EstadoWifi::Aguardando;
  uint32_t instanteMs_ = 0;  ///< Início da tentativa (Conectando) ou hora da próxima (Aguardando).
  bool primeiraVez_ = true;
  uint32_t tentativas_ = 0;
  uint32_t quedas_ = 0;
  uint8_t redeAtual_ = 0;
  uint8_t falhasNaRodada_ = 0;  ///< Tentativas esgotadas seguidas desde a última espera.
  uint32_t trocasDeRede_ = 0;
};

}  // namespace micromouse
