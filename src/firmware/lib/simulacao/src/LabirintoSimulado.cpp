#include "LabirintoSimulado.h"

#include "Direcao.h"

namespace micromouse {

namespace {

/// Colunas de texto por célula: o canto `+` e os três caracteres do trecho de parede.
constexpr uint8_t LARGURA_CELULA_TEXTO = 4;

/// Linhas de texto necessárias para o maior labirinto: uma borda por linha de células, mais uma.
constexpr uint8_t MAX_LINHAS_TEXTO = 2 * MAX_LINHAS_LABIRINTO + 1;

/// Maior largura de uma linha de texto, em caracteres.
constexpr uint8_t MAX_LARGURA_TEXTO = LARGURA_CELULA_TEXTO * MAX_COLUNAS_LABIRINTO + 1;

/// Divide o texto em linhas, sem copiá-lo.
/// @param texto Texto terminado em `\0`; o `\n` final é opcional.
/// @param[out] inicio Início de cada linha dentro do texto.
/// @param[out] quantidade Número de linhas encontradas.
/// @param[out] largura Número de caracteres de cada linha.
/// @return `false` se há linhas demais, linhas de larguras diferentes ou largura acima do máximo.
bool separarLinhas(const char* texto, const char* inicio[MAX_LINHAS_TEXTO], uint8_t& quantidade,
                   uint8_t& largura) {
  quantidade = 0;
  largura = 0;
  if (texto == nullptr) {
    return false;
  }
  const char* atual = texto;
  while (*atual != '\0') {
    const char* fim = atual;
    while (*fim != '\0' && *fim != '\n') {
      fim++;
    }
    const int tamanho = static_cast<int>(fim - atual);
    if (quantidade == MAX_LINHAS_TEXTO || tamanho > MAX_LARGURA_TEXTO) {
      return false;
    }
    if (quantidade == 0) {
      largura = static_cast<uint8_t>(tamanho);
    } else if (tamanho != largura) {
      return false;
    }
    inicio[quantidade++] = atual;
    atual = (*fim == '\n') ? fim + 1 : fim;
  }
  return true;
}

/// Converte o tamanho do texto em linhas e colunas de células, se for um dos três da competição.
bool tamanhoDoLabirinto(uint8_t linhasTexto, uint8_t largura, DimensaoMapa& dimensao,
                        uint8_t& linhas, uint8_t& colunas) {
  if (linhasTexto < 3 || linhasTexto % 2 == 0 || largura < 5 ||
      (largura - 1) % LARGURA_CELULA_TEXTO != 0) {
    return false;
  }
  linhas = (linhasTexto - 1) / 2;
  colunas = (largura - 1) / LARGURA_CELULA_TEXTO;
  if (linhas != 4) {
    return false;
  }
  switch (colunas) {
    case 4:
      dimensao = DimensaoMapa::Labirinto4x4;
      return true;
    case 8:
      dimensao = DimensaoMapa::Labirinto8x4;
      return true;
    case 12:
      dimensao = DimensaoMapa::Labirinto12x4;
      return true;
    default:
      return false;
  }
}

/// `true` se o caractere é permitido naquela posição da linha de texto.
/// Linha de borda: `+` nos cantos e `-` ou espaço entre eles.
/// Linha de células: `|` ou espaço onde há parede vertical e espaço dentro da célula.
bool caractereValido(bool linhaDeBorda, uint8_t posicao, char c) {
  const bool noCanto = posicao % LARGURA_CELULA_TEXTO == 0;
  if (linhaDeBorda) {
    return noCanto ? c == '+' : (c == '-' || c == ' ');
  }
  return noCanto ? (c == '|' || c == ' ') : c == ' ';
}

}  // namespace

ResultadoCarga LabirintoSimulado::carregar(const char* texto) {
  const char* inicio[MAX_LINHAS_TEXTO];
  uint8_t quantidade = 0;
  uint8_t largura = 0;
  LabirintoSimulado novo;
  if (!separarLinhas(texto, inicio, quantidade, largura) ||
      !tamanhoDoLabirinto(quantidade, largura, novo.dimensao_, novo.linhas_, novo.colunas_)) {
    return ResultadoCarga::TamanhoInvalido;
  }

  for (uint8_t i = 0; i < quantidade; i++) {
    for (uint8_t posicao = 0; posicao < largura; posicao++) {
      if (!caractereValido(i % 2 == 0, posicao, inicio[i][posicao])) {
        return ResultadoCarga::CaractereInvalido;
      }
    }
  }

  // A linha de texto 0 é a borda Norte; a borda 0 do labirinto é a Sul.
  for (uint8_t i = 0; i < quantidade; i++) {
    if (i % 2 == 0) {
      const uint8_t borda = novo.linhas_ - i / 2;
      for (uint8_t coluna = 0; coluna < novo.colunas_; coluna++) {
        const char* trecho = inicio[i] + coluna * LARGURA_CELULA_TEXTO + 1;
        const uint8_t hifens = (trecho[0] == '-') + (trecho[1] == '-') + (trecho[2] == '-');
        if (hifens != 0 && hifens != 3) {
          return ResultadoCarga::ParedeIncompleta;
        }
        novo.paredesHorizontais_[borda][coluna] = (hifens == 3);
      }
    } else {
      const uint8_t linha = novo.linhas_ - 1 - i / 2;
      for (uint8_t borda = 0; borda <= novo.colunas_; borda++) {
        novo.paredesVerticais_[linha][borda] = (inicio[i][borda * LARGURA_CELULA_TEXTO] == '|');
      }
    }
  }

  for (uint8_t coluna = 0; coluna < novo.colunas_; coluna++) {
    if (!novo.paredesHorizontais_[0][coluna] || !novo.paredesHorizontais_[novo.linhas_][coluna]) {
      return ResultadoCarga::PerimetroAberto;
    }
  }
  for (uint8_t linha = 0; linha < novo.linhas_; linha++) {
    if (!novo.paredesVerticais_[linha][0] || !novo.paredesVerticais_[linha][novo.colunas_]) {
      return ResultadoCarga::PerimetroAberto;
    }
  }

  *this = novo;
  return ResultadoCarga::Carregado;
}

bool LabirintoSimulado::carregado() const { return linhas_ != 0; }

DimensaoMapa LabirintoSimulado::obterDimensao() const { return dimensao_; }

uint8_t LabirintoSimulado::obterLinhas() const { return linhas_; }

uint8_t LabirintoSimulado::obterColunas() const { return colunas_; }

bool LabirintoSimulado::posValida(PosicaoCelula celula) const {
  return celula.linha < linhas_ && celula.coluna < colunas_;
}

bool LabirintoSimulado::temParede(PosicaoCelula celula, Direcao direcao) const {
  if (!posValida(celula)) {
    return true;
  }
  switch (direcao) {
    case Direcao::Norte:
      return paredesHorizontais_[celula.linha + 1][celula.coluna];
    case Direcao::Sul:
      return paredesHorizontais_[celula.linha][celula.coluna];
    case Direcao::Leste:
      return paredesVerticais_[celula.linha][celula.coluna + 1];
    case Direcao::Oeste:
      return paredesVerticais_[celula.linha][celula.coluna];
  }
  return true;
}

bool LabirintoSimulado::distancias(const Pose& pose, DistanciasLaterais& saida) const {
  if (!posValida(pose.celula)) {
    return false;
  }
  uint16_t medidas[3];
  const Lado lados[3] = {Lado::Frente, Lado::Esquerda, Lado::Direita};
  for (uint8_t i = 0; i < 3; i++) {
    const Direcao direcao = direcaoAbsoluta(pose.direcao, lados[i]);
    uint16_t distancia = DISTANCIA_PAREDE_IMEDIATA_MM;
    PosicaoCelula atual = pose.celula;
    // Sem parede, a vizinha existe e está dentro do labirinto; como o perímetro é fechado
    // (garantido por `carregar`), o laço sempre termina numa parede.
    while (!temParede(atual, direcao)) {
      vizinha(atual, direcao, atual);
      distancia += TAMANHO_CELULA_MM;
    }
    medidas[i] = distancia;
  }
  saida = DistanciasLaterais{medidas[0], medidas[1], medidas[2]};
  return true;
}

}  // namespace micromouse
