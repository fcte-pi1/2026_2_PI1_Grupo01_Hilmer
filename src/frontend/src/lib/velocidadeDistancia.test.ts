import { describe, expect, it } from 'vitest';
import {
  LADO_CELULA_M,
  calcularMetricas,
  distanciaPercorrida,
  serieDeVelocidade,
  velocidadePelaDistancia,
} from './painelAoVivo';
import { TRACO, bateriaRestante, desafioCumprido, formatarVelocidadeOuTraco } from './formatacao';
import { leituraSimulada } from '../test/dadosAoVivo';

/*
 * Velocidade média pela distância (#557). Leituras inventadas (dados simulados), com o robô mandando
 * `velocidade = 0`, como o firmware faz hoje.
 */
const parado = (n: number, linha: number, coluna: number, tempo: number, estado = 'explorando') =>
  leituraSimulada(n, { posicao_linha: linha, posicao_coluna: coluna, velocidade: 0, tempo_decorrido: tempo, estado_corrida: estado });

describe('distanciaPercorrida', () => {
  it('cada troca de célula vale 0,18 m', () => {
    const leituras = [parado(1, 0, 0, 0), parado(2, 1, 0, 1), parado(3, 1, 0, 1.5), parado(4, 1, 1, 2)];
    expect(distanciaPercorrida(leituras)).toBeCloseTo(2 * LADO_CELULA_M);
  });

  it('leitura que pula células conta todas, e a ordem é a do numero_sequencia', () => {
    const leituras = [parado(3, 2, 1, 3), parado(1, 0, 0, 0)]; // chegou fora de ordem e pulou 3 células
    expect(distanciaPercorrida(leituras)).toBeCloseTo(3 * 0.18);
  });

  it('sem movimento, a distância é zero', () => {
    expect(distanciaPercorrida([])).toBe(0);
    expect(distanciaPercorrida([parado(1, 0, 0, 0), parado(2, 0, 0, 5)])).toBe(0);
  });
});

describe('velocidadePelaDistancia', () => {
  it('é a distância dividida pelo tempo da corrida', () => {
    // 10 células (1,8 m) em 9 s → 0,2 m/s
    const leituras = Array.from({ length: 11 }, (_, i) => parado(i + 1, i, 0, i * 0.9));
    expect(velocidadePelaDistancia(leituras, 9)).toBeCloseTo(0.2);
  });

  it('sem tempo informado, usa o tempo_decorrido da última leitura da corrida', () => {
    const leituras = [parado(1, 0, 0, 0, 'aguardando'), parado(2, 1, 0, 0.9), parado(3, 2, 0, 1.8)];
    expect(velocidadePelaDistancia(leituras)).toBeCloseTo(0.36 / 1.8);
  });

  it('sem movimento ou sem tempo, devolve null', () => {
    expect(velocidadePelaDistancia([parado(1, 0, 0, 0), parado(2, 0, 0, 3)], 3)).toBeNull();
    expect(velocidadePelaDistancia([parado(1, 0, 0, 0), parado(2, 1, 0, 0)], 0)).toBeNull();
    expect(velocidadePelaDistancia([], 10)).toBeNull();
  });
});

describe('calcularMetricas com velocidade zerada (#557)', () => {
  it('usa a distância quando o robô manda velocidade 0', () => {
    const leituras = [parado(1, 0, 0, 0), parado(2, 1, 0, 1), parado(3, 2, 0, 2)];
    const m = calcularMetricas(leituras, 2);
    expect(m.velocidadeMedia).toBeCloseTo(0.18);
    expect(m.mediaPelaDistancia).toBe(true);
    expect(m.velocidadePico).toBeNull();
  });

  it('com velocidade enviada pelo robô, continua a média das instantâneas', () => {
    const leituras = [leituraSimulada(1, { velocidade: 0.2 }), leituraSimulada(2, { velocidade: 0.4, posicao_linha: 3 })];
    const m = calcularMetricas(leituras, 10);
    expect(m.velocidadeMedia).toBeCloseTo(0.3);
    expect(m.mediaPelaDistancia).toBe(false);
    expect(m.velocidadePico).toBe(0.4);
  });

  it('sem movimento e sem velocidade, a média é null (nunca 0)', () => {
    expect(calcularMetricas([parado(1, 0, 0, 0), parado(2, 0, 0, 4)], 4).velocidadeMedia).toBeNull();
  });
});

describe('serieDeVelocidade', () => {
  it('com velocidade zerada, cada ponto é a média acumulada pela distância', () => {
    const { leituras, estimada } = serieDeVelocidade([parado(1, 0, 0, 0, 'aguardando'), parado(2, 1, 0, 1), parado(3, 2, 0, 3)]);
    expect(estimada).toBe(true);
    expect(leituras.map((l) => l.numero_sequencia)).toEqual([2, 3]); // sem a leitura da largada
    expect(leituras[0].velocidade).toBeCloseTo(0.18);
    expect(leituras[1].velocidade).toBeCloseTo(0.36 / 3);
  });

  it('dá o mesmo resultado que calcular ponto a ponto, mesmo com leituras fora de ordem', () => {
    // Referência ingênua: a média pela distância até cada leitura, recalculada do zero.
    const leituras = [
      parado(1, 0, 0, 0, 'aguardando'),
      parado(4, 2, 1, 3),
      parado(2, 1, 0, 1),
      parado(6, 3, 3, 5),
      parado(3, 2, 0, 2),
      parado(5, 2, 2, 4),
    ];
    const { leituras: serie } = serieDeVelocidade(leituras);
    const esperado = [2, 3, 4, 5, 6].map((n) => {
      const ate = leituras.filter((l) => l.numero_sequencia <= n);
      const tempo = leituras.find((l) => l.numero_sequencia === n)!.tempo_decorrido;
      return velocidadePelaDistancia(ate, tempo) ?? 0;
    });
    expect(serie.map((l) => l.numero_sequencia)).toEqual([2, 3, 4, 5, 6]);
    serie.forEach((l, i) => expect(l.velocidade).toBeCloseTo(esperado[i]));
  });

  it('com velocidade real, devolve as leituras como vieram', () => {
    const reais = [leituraSimulada(1, { velocidade: 0.3 })];
    expect(serieDeVelocidade(reais)).toEqual({ leituras: reais, estimada: false });
  });
});

describe('formatarVelocidadeOuTraco e desafioCumprido', () => {
  it('nunca mostra 0.00 m/s: nulo ou zero viram traço', () => {
    expect(formatarVelocidadeOuTraco(null)).toBe(TRACO);
    expect(formatarVelocidadeOuTraco(undefined)).toBe(TRACO);
    expect(formatarVelocidadeOuTraco(0)).toBe(TRACO);
    expect(formatarVelocidadeOuTraco(0.256)).toBe('0.26 m/s');
    expect(formatarVelocidadeOuTraco(Number.NaN)).toBe(TRACO);
  });

  it('com movimento, um valor que arredondaria para 0.00 aparece como "< 0.01 m/s"', () => {
    // Corrida real de 21/11: o robô andou 4 células e ficou parado até as 10:00; a média gravada é ~0,004.
    expect(formatarVelocidadeOuTraco(0.004)).toBe('< 0.01 m/s');
    expect(formatarVelocidadeOuTraco(0.0049)).toBe('< 0.01 m/s');
    expect(formatarVelocidadeOuTraco(0.005)).toBe('0.01 m/s');
  });

  it('bateria restante a partir do percentual consumido gravado pelo banco', () => {
    expect(bateriaRestante(12)).toBe('88%');
    expect(bateriaRestante(0)).toBe('100%');
    expect(bateriaRestante(100)).toBe('0%');
    expect(bateriaRestante(null)).toBe(TRACO);
    expect(bateriaRestante(850)).toBe(TRACO); // mAh ou valor inválido: não vira porcentagem errada
    expect(bateriaRestante(-3)).toBe(TRACO);
  });

  it('desafio cumprido em Sim, Não ou traço', () => {
    expect(desafioCumprido('concluida')).toBe('Sim');
    expect(desafioCumprido('nao_concluida')).toBe('Não');
    expect(desafioCumprido('em_andamento')).toBe(TRACO);
  });
});
