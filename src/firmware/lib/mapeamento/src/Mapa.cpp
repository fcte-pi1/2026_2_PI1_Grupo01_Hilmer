#include "Mapa.h"

#include "Direcao.h"

namespace micromouse {

namespace {

// A célula guarda as paredes em campos nomeados; esta função escolhe o campo da direção dada.
template <typename CelulaT>
auto& referenciaParede(CelulaT& celula, Direcao direcao) {
  switch (direcao) {
    case Direcao::Norte:
      return celula.norte;
    case Direcao::Leste:
      return celula.leste;
    case Direcao::Sul:
      return celula.sul;
    default:
      return celula.oeste;
  }
}

}  // namespace

Mapa::Mapa(DimensaoMapa dimensao) : dimensao(dimensao), linhas(0), colunas(0) {
  switch (dimensao) {
    case DimensaoMapa::Labirinto4x4:
      linhas = 4;
      colunas = 4;
      break;
    case DimensaoMapa::Labirinto8x4:
      linhas = 4;
      colunas = 8;
      break;
    case DimensaoMapa::Labirinto12x4:
      linhas = 4;
      colunas = 12;
      break;
  }
  preencherPerimetro();
}

bool Mapa::posValida(PosicaoCelula posicao) const {
  return posicao.linha < linhas && posicao.coluna < colunas;
}

Mapa::Celula Mapa::obterCelula(PosicaoCelula posicao) const {
  if (!posValida(posicao)) {
    return Celula{};
  }
  return celulas[posicao.linha][posicao.coluna];
}

EstadoParede Mapa::obterParede(PosicaoCelula posicao, Direcao direcao) const {
  if (!posValida(posicao)) {
    return EstadoParede::Desconhecido;
  }
  return referenciaParede(celulas[posicao.linha][posicao.coluna], direcao);
}

void Mapa::preencherPerimetro() {
  for (uint8_t coluna = 0; coluna < colunas; coluna++) {
    referenciaParede(celulas[0][coluna], Direcao::Sul) = EstadoParede::Parede;
    referenciaParede(celulas[linhas - 1][coluna], Direcao::Norte) = EstadoParede::Parede;
  }
  for (uint8_t linha = 0; linha < linhas; linha++) {
    referenciaParede(celulas[linha][0], Direcao::Oeste) = EstadoParede::Parede;
    referenciaParede(celulas[linha][colunas - 1], Direcao::Leste) = EstadoParede::Parede;
  }
}

ResultadoRegistro Mapa::registrarParede(PosicaoCelula posicao, Direcao direcao, EstadoParede estado) {
  if (!posValida(posicao) || estado == EstadoParede::Desconhecido) {
    return ResultadoRegistro::Invalida;
  }
  EstadoParede& parede = referenciaParede(celulas[posicao.linha][posicao.coluna], direcao);
  if (parede == estado) {
    return ResultadoRegistro::JaConhecida;
  }
  if (parede != EstadoParede::Desconhecido) {
    return ResultadoRegistro::Conflito;
  }
  parede = estado;
  PosicaoCelula vizinhaPosicao = {0, 0};
  if (vizinha(posicao, direcao, vizinhaPosicao) && posValida(vizinhaPosicao)) {
    referenciaParede(celulas[vizinhaPosicao.linha][vizinhaPosicao.coluna], oposta(direcao)) = estado;
  }
  return ResultadoRegistro::Registrada;
}

ResultadoRegistro Mapa::corrigirParede(PosicaoCelula posicao, Direcao direcao, EstadoParede estado) {
  if (!posValida(posicao) || estado == EstadoParede::Desconhecido) {
    return ResultadoRegistro::Invalida;
  }
  PosicaoCelula vizinhaPosicao = {0, 0};
  const bool temVizinha = vizinha(posicao, direcao, vizinhaPosicao) && posValida(vizinhaPosicao);
  if (!temVizinha && estado == EstadoParede::Livre) {
    return ResultadoRegistro::Invalida;  // o perímetro é sempre parede
  }
  EstadoParede& parede = referenciaParede(celulas[posicao.linha][posicao.coluna], direcao);
  if (parede == estado) {
    return ResultadoRegistro::JaConhecida;
  }
  parede = estado;
  if (temVizinha) {
    referenciaParede(celulas[vizinhaPosicao.linha][vizinhaPosicao.coluna], oposta(direcao)) = estado;
  }
  return ResultadoRegistro::Registrada;
}

namespace {

bool celulasIguais(const Mapa::Celula& a, const Mapa::Celula& b) {
  return a.norte == b.norte && a.sul == b.sul && a.leste == b.leste && a.oeste == b.oeste;
}

}  // namespace

bool Mapa::operator==(const Mapa& outro) const {
  if (dimensao != outro.dimensao) {
    return false;
  }
  for (uint8_t linha = 0; linha < linhas; linha++) {
    for (uint8_t coluna = 0; coluna < colunas; coluna++) {
      if (!celulasIguais(celulas[linha][coluna], outro.celulas[linha][coluna])) {
        return false;
      }
    }
  }
  return true;
}

bool Mapa::operator!=(const Mapa& outro) const {
  return !(*this == outro);
}

}  // namespace micromouse
