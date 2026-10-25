#include "RoboSimulado.h"

#include <cmath>

#include "Configuracao.h"
#include "Direcao.h"

namespace micromouse {

namespace {

constexpr double PI = 3.14159265358979323846;
constexpr double CELULA_MM = TAMANHO_CELULA_MM;
constexpr double ALCANCE_TOF_MM = 1200.0;
constexpr uint16_t FORA_DE_ALCANCE_MM = 8190;
constexpr double PASSO_RAIO_MM = 0.5;

double rumoDaDirecaoRad(Direcao d) {
  // Norte 0°, Oeste +90°, Sul 180°, Leste −90° (positivo para a esquerda).
  switch (d) {
    case Direcao::Norte:
      return 0.0;
    case Direcao::Oeste:
      return PI / 2;
    case Direcao::Sul:
      return PI;
    case Direcao::Leste:
      return -PI / 2;
  }
  return 0.0;
}

}  // namespace

RoboSimulado::RoboSimulado(const LabirintoSimulado& labirinto, const ParametrosRoboSimulado& parametros,
                           const ConfiguracaoDeslocamento& tabela)
    : labirinto_(labirinto), parametros_(parametros), tabela_(tabela) {}

void RoboSimulado::posicionar(const Pose& pose, double desvioLateralMm, double desvioRumoGraus) {
  rumoRad_ = rumoDaDirecaoRad(pose.direcao) + desvioRumoGraus * PI / 180.0;
  const double rumoBase = rumoDaDirecaoRad(pose.direcao);
  // Direita do rumo base: (cos θ, sin θ).
  x_ = pose.celula.coluna * CELULA_MM + desvioLateralMm * std::cos(rumoBase);
  y_ = pose.celula.linha * CELULA_MM + desvioLateralMm * std::sin(rumoBase);
  potencia_ = {0, 0};
  taxaRadPorS_ = 0.0;
  colidiu_ = false;
  msEmContato_ = 0;
  rodaEsquerdaMm_ = 0.0;
  rodaDireitaMm_ = 0.0;
}

void RoboSimulado::definirPotencia(const PotenciaRodas& potencia) {
  potencia_ = {limitarPotencia(potencia.esquerda), limitarPotencia(potencia.direita)};
}

void RoboSimulado::avancar(uint32_t dtMs) {
  const double vEsq = tabela_.velocidadeMmPorS(potencia_.esquerda) * parametros_.fatorEsquerda;
  const double vDir = tabela_.velocidadeMmPorS(potencia_.direita) * parametros_.fatorDireita;
  const double v = (vEsq + vDir) / 2.0;
  taxaRadPorS_ = (vDir - vEsq) / parametros_.bitolaMm;
  // Em passos de 1 ms, para o raio e a colisão não pularem a parede.
  for (uint32_t i = 0; i < dtMs; i++) {
    const double dt = 0.001;
    const double meio = rumoRad_ + taxaRadPorS_ * dt / 2.0;
    x_ += -std::sin(meio) * v * dt;
    y_ += std::cos(meio) * v * dt;
    rodaEsquerdaMm_ += vEsq * dt;
    rodaDireitaMm_ += vDir * dt;
    rumoRad_ += taxaRadPorS_ * dt;
    conferirColisao();
  }
}

bool RoboSimulado::dentroDeParede(double x, double y) const {
  const double meia = parametros_.espessuraParedeMm / 2.0;
  const int linhas = labirinto_.obterLinhas();
  const int colunas = labirinto_.obterColunas();
  const double minX = -CELULA_MM / 2.0 + meia;
  const double minY = -CELULA_MM / 2.0 + meia;
  const double maxX = colunas * CELULA_MM - CELULA_MM / 2.0 - meia;
  const double maxY = linhas * CELULA_MM - CELULA_MM / 2.0 - meia;
  if (x <= minX || y <= minY || x >= maxX || y >= maxY) {
    return true;
  }
  // Fronteiras mais próximas: a horizontal (entre linhas) e a vertical (entre colunas).
  const int bordaH = static_cast<int>(std::floor((y + CELULA_MM / 2.0) / CELULA_MM + 0.5));
  const int bordaV = static_cast<int>(std::floor((x + CELULA_MM / 2.0) / CELULA_MM + 0.5));
  const double yBorda = bordaH * CELULA_MM - CELULA_MM / 2.0;
  const double xBorda = bordaV * CELULA_MM - CELULA_MM / 2.0;
  const bool naFaixaH = std::fabs(y - yBorda) <= meia;
  const bool naFaixaV = std::fabs(x - xBorda) <= meia;
  if (naFaixaH && naFaixaV) {
    return true;  // coluna
  }
  if (naFaixaH) {
    const int coluna = static_cast<int>(std::floor((x + CELULA_MM / 2.0) / CELULA_MM));
    const PosicaoCelula abaixo{static_cast<uint8_t>(bordaH - 1), static_cast<uint8_t>(coluna)};
    return labirinto_.temParede(abaixo, Direcao::Norte);
  }
  if (naFaixaV) {
    const int linha = static_cast<int>(std::floor((y + CELULA_MM / 2.0) / CELULA_MM));
    const PosicaoCelula aOeste{static_cast<uint8_t>(linha), static_cast<uint8_t>(bordaV - 1)};
    return labirinto_.temParede(aOeste, Direcao::Leste);
  }
  return false;
}

double RoboSimulado::raio(double ox, double oy, double ux, double uy) const {
  for (double t = 0.0; t <= ALCANCE_TOF_MM; t += PASSO_RAIO_MM) {
    if (dentroDeParede(ox + ux * t, oy + uy * t)) {
      return t;
    }
  }
  return -1.0;
}

DistanciasLaterais RoboSimulado::medirTof() const {
  const double frenteX = -std::sin(rumoRad_);
  const double frenteY = std::cos(rumoRad_);
  const double direitaX = std::cos(rumoRad_);
  const double direitaY = std::sin(rumoRad_);
  auto medir = [&](double distanciaSensor, double ux, double uy) -> uint16_t {
    const double t = raio(x_ + ux * distanciaSensor, y_ + uy * distanciaSensor, ux, uy);
    return t < 0.0 ? FORA_DE_ALCANCE_MM : static_cast<uint16_t>(std::lround(t));
  };
  return {medir(parametros_.sensorFrontalMm, frenteX, frenteY),
          medir(parametros_.sensorLateralMm, -direitaX, -direitaY),
          medir(parametros_.sensorLateralMm, direitaX, direitaY)};
}

int16_t RoboSimulado::taxaGiroDecimosDps() const {
  return static_cast<int16_t>(std::lround(taxaRadPorS_ * 180.0 / PI * 10.0));
}

double RoboSimulado::rumoGraus() const { return rumoRad_ * 180.0 / PI; }

PosicaoCelula RoboSimulado::celula() const {
  const int linha = static_cast<int>(std::floor(y_ / CELULA_MM + 0.5));
  const int coluna = static_cast<int>(std::floor(x_ / CELULA_MM + 0.5));
  return {static_cast<uint8_t>(linha < 0 ? 0 : linha), static_cast<uint8_t>(coluna < 0 ? 0 : coluna)};
}

double RoboSimulado::erroAoLongoMm() const {
  const PosicaoCelula c = celula();
  const double dx = x_ - c.coluna * CELULA_MM;
  const double dy = y_ - c.linha * CELULA_MM;
  return -std::sin(rumoRad_) * dx + std::cos(rumoRad_) * dy;
}

double RoboSimulado::erroLateralMm() const {
  const PosicaoCelula c = celula();
  const double dx = x_ - c.coluna * CELULA_MM;
  const double dy = y_ - c.linha * CELULA_MM;
  return std::cos(rumoRad_) * dx + std::sin(rumoRad_) * dy;
}

double RoboSimulado::erroRumoGraus() const {
  const double graus = rumoGraus();
  return graus - 90.0 * std::floor(graus / 90.0 + 0.5);
}

void RoboSimulado::conferirColisao() {
  const double fx = -std::sin(rumoRad_);
  const double fy = std::cos(rumoRad_);
  const double dx = std::cos(rumoRad_);
  const double dy = std::sin(rumoRad_);
  const double c = parametros_.meioComprimentoMm;
  const double l = parametros_.meiaLarguraMm;
  const double pontos[8][2] = {{c, l}, {c, -l}, {-c, l}, {-c, -l}, {c, 0}, {-c, 0}, {0, l}, {0, -l}};
  for (const auto& p : pontos) {
    if (dentroDeParede(x_ + fx * p[0] + dx * p[1], y_ + fy * p[0] + dy * p[1])) {
      if (!colidiu_) {
        xColisao_ = x_;
        yColisao_ = y_;
        rumoColisao_ = rumoGraus();
      }
      colidiu_ = true;
      msEmContato_++;
      return;
    }
  }
}

}  // namespace micromouse
