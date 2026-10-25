import { describe, expect, it } from 'vitest';
import {
  PAINEL_INICIAL,
  aplicarCorrida,
  aplicarMapa,
  aplicarMapaCelula,
  aplicarTelemetria,
  calcularMetricas,
  fracaoDoLimite,
  type PainelAoVivo,
} from './painelAoVivo';
import { celulaSimulada, corridaSimulada, leituraSimulada } from '../test/dadosAoVivo';

// Testes unitários com linhas simuladas (não vêm do robô).

const emExecucao = (): PainelAoVivo => aplicarCorrida(PAINEL_INICIAL, corridaSimulada());

describe('aplicarCorrida', () => {
  it('começa a acompanhar uma corrida em andamento nova, com leituras zeradas', () => {
    const painel = emExecucao();
    expect(painel.estado).toBe('em_execucao');
    expect(painel.corrida?.id).toBe(1);
    expect(painel.leituras).toEqual([]);
    expect(painel.tempo).toBe(0);
  });

  it('troca de corrida quando outra entra em andamento (nova tentativa)', () => {
    const anterior = aplicarTelemetria(emExecucao(), leituraSimulada(1));
    const nova = aplicarCorrida(anterior, corridaSimulada({ id: 2, numero_tentativa: 2 }));
    expect(nova.corrida?.id).toBe(2);
    expect(nova.leituras).toEqual([]);
    expect(nova.tempo).toBe(0);
  });

  it('atualiza a mesma corrida em andamento sem perder as leituras', () => {
    const comLeitura = aplicarTelemetria(emExecucao(), leituraSimulada(3));
    const atualizado = aplicarCorrida(comLeitura, corridaSimulada({ velocidade_pico: 0.4 }));
    expect(atualizado.leituras).toHaveLength(1);
    expect(atualizado.corrida?.velocidade_pico).toBe(0.4);
    expect(atualizado.estado).toBe('em_execucao');
  });

  it('encerra como concluído e usa o tempo_final do banco', () => {
    const painel = aplicarCorrida(
      aplicarTelemetria(emExecucao(), leituraSimulada(40)),
      corridaSimulada({ status: 'concluida', tempo_final: 41.5 }),
    );
    expect(painel.estado).toBe('concluido');
    expect(painel.tempo).toBe(41.5);
  });

  it('encerra como não concluído e mantém o tempo quando não há tempo_final', () => {
    const painel = aplicarCorrida(
      aplicarTelemetria(emExecucao(), leituraSimulada(12)),
      corridaSimulada({ status: 'nao_concluida' }),
    );
    expect(painel.estado).toBe('nao_concluido');
    expect(painel.tempo).toBe(12);
  });

  it('ignora corridas encerradas que não são a acompanhada', () => {
    expect(aplicarCorrida(PAINEL_INICIAL, corridaSimulada({ status: 'concluida' }))).toBe(PAINEL_INICIAL);
    const painel = emExecucao();
    expect(aplicarCorrida(painel, corridaSimulada({ id: 9, status: 'concluida' }))).toBe(painel);
  });
});

describe('aplicarTelemetria', () => {
  it('ignora leituras enquanto não há corrida acompanhada', () => {
    expect(aplicarTelemetria(PAINEL_INICIAL, leituraSimulada(1))).toBe(PAINEL_INICIAL);
  });

  it('ignora leituras de outra corrida', () => {
    const painel = emExecucao();
    expect(aplicarTelemetria(painel, leituraSimulada(1, { corrida_id: 2 }))).toBe(painel);
  });

  it('acrescenta a leitura e avança o tempo', () => {
    const painel = aplicarTelemetria(emExecucao(), leituraSimulada(5, { tempo_decorrido: 5.5 }));
    expect(painel.leituras.map((l) => l.numero_sequencia)).toEqual([5]);
    expect(painel.tempo).toBe(5.5);
    expect(painel.estado).toBe('em_execucao');
  });

  it('descarta leitura repetida (mesmo numero_sequencia, reenvio do buffer)', () => {
    const uma = aplicarTelemetria(emExecucao(), leituraSimulada(1));
    expect(aplicarTelemetria(uma, leituraSimulada(1, { velocidade: 9 }))).toBe(uma);
  });

  it('põe leitura atrasada na posição certa sem voltar o cronômetro', () => {
    let painel = emExecucao();
    painel = aplicarTelemetria(painel, leituraSimulada(1));
    painel = aplicarTelemetria(painel, leituraSimulada(3));
    painel = aplicarTelemetria(painel, leituraSimulada(2));
    expect(painel.leituras.map((l) => l.numero_sequencia)).toEqual([1, 2, 3]);
    expect(painel.tempo).toBe(3);
  });

  it('conclui ao receber objetivo_alcancado', () => {
    const painel = aplicarTelemetria(
      emExecucao(),
      leituraSimulada(30, { tempo_decorrido: 30.2, evento: 'objetivo_alcancado' }),
    );
    expect(painel.estado).toBe('concluido');
    expect(painel.tempo).toBe(30.2);
  });

  it('colisão não encerra a corrida', () => {
    const painel = aplicarTelemetria(emExecucao(), leituraSimulada(4, { evento: 'colisao' }));
    expect(painel.estado).toBe('em_execucao');
  });

  it('encerra como não concluído ao chegar a 10:00 e trava o cronômetro em 10:00', () => {
    const quase = aplicarTelemetria(emExecucao(), leituraSimulada(1, { tempo_decorrido: 599.9 }));
    expect(quase.estado).toBe('em_execucao');
    const esgotado = aplicarTelemetria(quase, leituraSimulada(2, { tempo_decorrido: 600.4 }));
    expect(esgotado.estado).toBe('nao_concluido');
    expect(esgotado.tempo).toBe(600);
  });

  it('exatamente 10:00 já conta como tempo esgotado', () => {
    const painel = aplicarTelemetria(emExecucao(), leituraSimulada(1, { tempo_decorrido: 600 }));
    expect(painel.estado).toBe('nao_concluido');
  });

  it('ignora leituras depois que a corrida terminou', () => {
    const fim = aplicarTelemetria(emExecucao(), leituraSimulada(1, { evento: 'objetivo_alcancado' }));
    expect(aplicarTelemetria(fim, leituraSimulada(2))).toBe(fim);
  });
});

describe('calcularMetricas', () => {
  it('sem leituras, devolve tensão nula e zeros', () => {
    expect(calcularMetricas([])).toEqual({ tensao: null, correnteMa: 0, velocidadeMedia: 0, velocidadePico: 0 });
  });

  it('usa a última leitura para bateria e todas para a velocidade', () => {
    const metricas = calcularMetricas([
      leituraSimulada(1, { velocidade: 0.1, tensao_bateria: 8.2, corrente_bateria: 0.3 }),
      leituraSimulada(2, { velocidade: 0.5, tensao_bateria: 7.9, corrente_bateria: 0.4567 }),
      leituraSimulada(3, { velocidade: 0.3, tensao_bateria: 7.8, corrente_bateria: 0.4 }),
    ]);
    expect(metricas.tensao).toBe(7.8);
    expect(metricas.correnteMa).toBe(400);
    expect(metricas.velocidadeMedia).toBeCloseTo(0.3);
    expect(metricas.velocidadePico).toBe(0.5);
  });

  it('arredonda a corrente para mA inteiros', () => {
    expect(calcularMetricas([leituraSimulada(1, { corrente_bateria: 0.4567 })]).correnteMa).toBe(457);
  });
});

describe('fracaoDoLimite', () => {
  it('é a fração de 10:00 já usada, limitada entre 0 e 1', () => {
    expect(fracaoDoLimite(0)).toBe(0);
    expect(fracaoDoLimite(150)).toBe(0.25);
    expect(fracaoDoLimite(600)).toBe(1);
    expect(fracaoDoLimite(900)).toBe(1);
    expect(fracaoDoLimite(-5)).toBe(0);
  });
});

describe('aplicarMapaCelula e aplicarMapa (#239)', () => {
  it('acrescenta células novas ao mapa do painel', () => {
    let painel = emExecucao();
    painel = aplicarMapaCelula(painel, celulaSimulada(0, 0, { parede_norte: false }));
    painel = aplicarMapaCelula(painel, celulaSimulada(0, 1, { parede_norte: true }));

    expect(painel.mapa).toHaveLength(2);
    expect(painel.mapa[0]).toEqual({
      linha: 0,
      coluna: 0,
      parede_norte: false,
      parede_sul: true,
      parede_leste: false,
      parede_oeste: true,
    });
    expect(painel.mapa[1].linha).toBe(0);
    expect(painel.mapa[1].coluna).toBe(1);
    expect(painel.mapa[1].parede_norte).toBe(true);
  });

  it('atualiza célula já existente sem duplicar', () => {
    let painel = emExecucao();
    painel = aplicarMapaCelula(painel, celulaSimulada(1, 1, { parede_norte: null }));
    expect(painel.mapa).toHaveLength(1);
    expect(painel.mapa[0].parede_norte).toBeNull();

    // Reenvio ou atualização com parede descoberta
    painel = aplicarMapaCelula(painel, celulaSimulada(1, 1, { parede_norte: true }));
    expect(painel.mapa).toHaveLength(1);
    expect(painel.mapa[0].parede_norte).toBe(true);
  });

  it('ignora células de outra corrida', () => {
    const painel = emExecucao();
    const atualizado = aplicarMapaCelula(painel, celulaSimulada(0, 0, { corrida_id: 99 }));
    expect(atualizado.mapa).toHaveLength(0);
  });

  it('aceita células do mapa final após encerramento da corrida (HU-04)', () => {
    let painel = aplicarTelemetria(emExecucao(), leituraSimulada(10, { evento: 'objetivo_alcancado' }));
    expect(painel.estado).toBe('concluido');

    painel = aplicarMapaCelula(painel, celulaSimulada(3, 3, { corrida_id: 1, parede_norte: true }));
    expect(painel.mapa).toHaveLength(1);
    expect(painel.mapa[0].linha).toBe(3);
  });

  it('aplicarMapa aplica um lote inteiro de células', () => {
    const painel = emExecucao();
    const celulas = [
      celulaSimulada(0, 0, { parede_norte: false }),
      celulaSimulada(0, 1, { parede_norte: true }),
      celulaSimulada(1, 0, { parede_leste: true }),
    ];
    const comMapa = aplicarMapa(painel, celulas);
    expect(comMapa.mapa).toHaveLength(3);
  });
});
