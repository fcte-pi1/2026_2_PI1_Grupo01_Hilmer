// Lookup Table de Calibracao SoC vs Tensao LiPo 2S
// Gerado automaticamente pelo script curva_descarga_lipo.py (Issue #139)
#pragma once

#include <stdint.h>

struct PontoCalibracaoBateria {
    uint8_t soc_percentual;
    float tensao_pack_v;
};

static const PontoCalibracaoBateria TABELA_SOC_LIPO_2S[] = {
    { 100, 8.346f },
    {  90, 7.775f },
    {  80, 7.567f },
    {  70, 7.483f },
    {  60, 7.434f },
    {  50, 7.386f },
    {  40, 7.314f },
    {  30, 7.189f },
    {  20, 6.922f },
    {  10, 6.089f },
    {   0, 5.600f },
};

static const uint8_t TAMANHO_TABELA_SOC = sizeof(TABELA_SOC_LIPO_2S) / sizeof(TABELA_SOC_LIPO_2S[0]);
