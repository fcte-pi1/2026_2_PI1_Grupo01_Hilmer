import { StrictMode } from 'react';
import { act, renderHook, waitFor } from '@testing-library/react';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { CANAL_AO_VIVO, useTelemetriaAoVivo } from './useTelemetriaAoVivo';
import { criarClienteSupabaseSimulado, type ClienteSupabaseSimulado } from '../test/supabaseSimulado';
import { celulaSimulada, corridaSimulada, leituraSimulada } from '../test/dadosAoVivo';
import type { Corrida, MapaCelula, Telemetria } from '../tipos';

/*
 * Integração do hook com o Supabase simulado em memória (src/test/supabaseSimulado.ts).
 * O "robô" aqui é o próprio teste: ele grava linhas inventadas com `insert`, e o cliente
 * simulado dispara o evento Realtime de INSERT, como o banco real faria.
 */
const clienteSimulado = criarClienteSupabaseSimulado();
let clienteAtual: ClienteSupabaseSimulado | null = clienteSimulado;

vi.mock('../lib/supabase', () => ({
  get supabase() {
    return clienteAtual;
  },
}));

/** Corrida criada pela Edge Function: entra no banco sem evento Realtime (`corridas` não está na publicação). */
function largar(corrida: Corrida) {
  clienteSimulado.definirTabela('corridas', [...clienteSimulado.obterDados<Corrida>('corridas'), corrida]);
}

async function enviar(...leituras: Telemetria[]) {
  await act(async () => {
    for (const leitura of leituras) await clienteSimulado.from<Telemetria>('telemetria').insert(leitura);
  });
}

describe('useTelemetriaAoVivo (integração com Supabase simulado)', () => {
  beforeEach(() => {
    clienteSimulado.limpar();
    clienteAtual = clienteSimulado;
  });

  it('sem Supabase configurado, fica indisponível e aguardando', () => {
    clienteAtual = null;
    const { result } = renderHook(() => useTelemetriaAoVivo());
    expect(result.current.disponivel).toBe(false);
    expect(result.current.estado).toBe('aguardando');
  });

  it('assina telemetria, corridas e mapa em canais separados e aguarda quando não há corrida em andamento', async () => {
    clienteSimulado.definirTabela('corridas', [corridaSimulada({ id: 7, status: 'concluida', tempo_final: 50 })]);
    const { result } = renderHook(() => useTelemetriaAoVivo());

    expect(result.current.disponivel).toBe(true);
    // Canais separados para isolamento e resiliência Realtime (#231, #239).
    const nomes = clienteSimulado.obterCanais().map((c) => c.nome);
    expect(nomes).toHaveLength(3);
    expect(nomes[0]).toMatch(new RegExp(`^${CANAL_AO_VIVO}-telemetria-\\d+$`));
    expect(nomes[1]).toMatch(new RegExp(`^${CANAL_AO_VIVO}-corridas-\\d+$`));
    expect(nomes[2]).toMatch(new RegExp(`^${CANAL_AO_VIVO}-mapa-\\d+$`));
    await act(async () => {});
    expect(result.current.estado).toBe('aguardando');
    expect(result.current.corrida).toBeNull();
  });

  it('ao abrir no meio da corrida, carrega a corrida em andamento e as leituras já gravadas', async () => {
    clienteSimulado.definirTabela('corridas', [
      corridaSimulada({ id: 1, status: 'concluida', data_hora_inicio: '2026-10-24T13:00:00Z' }),
      corridaSimulada({ id: 2, numero_tentativa: 2, data_hora_inicio: '2026-10-24T14:00:00Z' }),
    ]);
    clienteSimulado.definirTabela('telemetria', [
      leituraSimulada(2, { corrida_id: 2 }),
      leituraSimulada(1, { corrida_id: 2 }),
      leituraSimulada(1, { corrida_id: 1, id: 99 }),
    ]);
    const { result } = renderHook(() => useTelemetriaAoVivo());

    await waitFor(() => expect(result.current.leituras).toHaveLength(2));
    expect(result.current.estado).toBe('em_execucao');
    expect(result.current.corrida?.id).toBe(2);
    expect(result.current.leituras.map((l) => l.numero_sequencia)).toEqual([1, 2]);
    expect(result.current.tempo).toBe(2);
  });

  it('percebe a largada pela primeira leitura e atualiza a cada linha inserida', async () => {
    const { result } = renderHook(() => useTelemetriaAoVivo());
    await act(async () => {});
    expect(result.current.estado).toBe('aguardando');

    largar(corridaSimulada({ id: 3 }));
    await enviar(leituraSimulada(1, { corrida_id: 3, velocidade: 0.1 }));
    await waitFor(() => expect(result.current.estado).toBe('em_execucao'));
    expect(result.current.leituras).toHaveLength(1);

    await enviar(
      leituraSimulada(2, { corrida_id: 3, velocidade: 0.3, tempo_decorrido: 2.5 }),
      leituraSimulada(3, { corrida_id: 3, velocidade: 0.2, tempo_decorrido: 3.5 }),
    );
    expect(result.current.leituras.map((l) => l.velocidade)).toEqual([0.1, 0.3, 0.2]);
    expect(result.current.tempo).toBe(3.5);
  });

  it('não duplica leitura reenviada e ignora leituras de corridas encerradas', async () => {
    largar(corridaSimulada({ id: 4 }));
    largar(corridaSimulada({ id: 5, status: 'concluida', numero_tentativa: 2 }));
    const { result } = renderHook(() => useTelemetriaAoVivo());
    await waitFor(() => expect(result.current.corrida?.id).toBe(4));

    await enviar(leituraSimulada(1, { corrida_id: 4 }), leituraSimulada(1, { corrida_id: 4, id: 50 }));
    await enviar(leituraSimulada(9, { corrida_id: 5 }));
    await act(async () => {});

    expect(result.current.corrida?.id).toBe(4);
    expect(result.current.leituras.map((l) => l.numero_sequencia)).toEqual([1]);
  });

  it('busca cada corrida desconhecida uma vez só e não lê leituras de corrida encerrada', async () => {
    const { result } = renderHook(() => useTelemetriaAoVivo());
    await act(async () => {});
    const consultas = vi.spyOn(clienteSimulado, 'from');

    largar(corridaSimulada({ id: 30, status: 'concluida' }));
    await enviar(leituraSimulada(1, { corrida_id: 30 }), leituraSimulada(2, { corrida_id: 30 }));
    await act(async () => {});

    const tabelas = consultas.mock.calls.map(([tabela]) => tabela);
    expect(tabelas.filter((t) => t === 'corridas')).toHaveLength(1);
    // As duas chamadas a 'telemetria' são os inserts do próprio teste; nenhuma consulta de leituras.
    expect(consultas.mock.calls.filter(([t]) => t === 'telemetria')).toHaveLength(2);
    expect(result.current.estado).toBe('aguardando');
    consultas.mockRestore();
  });

  it('conclui quando o robô envia objetivo_alcancado', async () => {
    largar(corridaSimulada({ id: 6 }));
    const { result } = renderHook(() => useTelemetriaAoVivo());
    await waitFor(() => expect(result.current.estado).toBe('em_execucao'));

    await enviar(leituraSimulada(1, { corrida_id: 6, tempo_decorrido: 42.3, evento: 'objetivo_alcancado' }));
    expect(result.current.estado).toBe('concluido');
    expect(result.current.tempo).toBe(42.3);
  });

  it('encerra como não concluído ao passar de 10:00', async () => {
    largar(corridaSimulada({ id: 8 }));
    const { result } = renderHook(() => useTelemetriaAoVivo());
    await waitFor(() => expect(result.current.estado).toBe('em_execucao'));

    await enviar(leituraSimulada(1, { corrida_id: 8, tempo_decorrido: 601 }));
    expect(result.current.estado).toBe('nao_concluido');
    expect(result.current.tempo).toBe(600);
  });

  it('acompanha eventos da tabela corridas, se ela entrar na publicação do Realtime', async () => {
    const { result } = renderHook(() => useTelemetriaAoVivo());
    await act(async () => {});

    await act(async () => {
      await clienteSimulado.from<Corrida>('corridas').insert(corridaSimulada({ id: 10 }));
    });
    expect(result.current.estado).toBe('em_execucao');

    await act(async () => {
      clienteSimulado.emitirRealtime('corridas', 'UPDATE', corridaSimulada({ id: 10, status: 'nao_concluida', tempo_final: 75 }));
    });
    expect(result.current.estado).toBe('nao_concluido');
    expect(result.current.tempo).toBe(75);
  });

  it('mostra erro se a consulta da corrida em andamento falhar', async () => {
    clienteSimulado.definirErro('corridas', 'falha simulada');
    const { result } = renderHook(() => useTelemetriaAoVivo());
    await waitFor(() => expect(result.current.erro).toMatch(/corrida em andamento/));
    expect(result.current.estado).toBe('aguardando');
  });

  it('mostra erro se a consulta das leituras falhar', async () => {
    largar(corridaSimulada({ id: 11 }));
    clienteSimulado.definirErro('telemetria', 'falha simulada');
    const { result } = renderHook(() => useTelemetriaAoVivo());
    await waitFor(() => expect(result.current.erro).toMatch(/telemetria/));
    expect(result.current.estado).toBe('em_execucao');
  });

  it('mostra erro se a busca da corrida de uma leitura nova falhar, e tenta de novo na próxima', async () => {
    const { result } = renderHook(() => useTelemetriaAoVivo());
    await act(async () => {});

    largar(corridaSimulada({ id: 12 }));
    clienteSimulado.definirErro('corridas', 'falha simulada');
    await enviar(leituraSimulada(1, { corrida_id: 12 }));
    await waitFor(() => expect(result.current.erro).toMatch(/corrida em andamento/));
    expect(result.current.estado).toBe('aguardando');

    clienteSimulado.definirErro('corridas', null);
    await enviar(leituraSimulada(2, { corrida_id: 12 }));
    await waitFor(() => expect(result.current.leituras).toHaveLength(2));
  });

  it('ao abrir no meio da corrida, carrega também as células do mapa já mapeadas (#239)', async () => {
    largar(corridaSimulada({ id: 25 }));
    clienteSimulado.definirTabela('telemetria', [leituraSimulada(1, { corrida_id: 25 })]);
    clienteSimulado.definirTabela('mapa_celulas', [
      celulaSimulada(0, 0, { corrida_id: 25, parede_norte: false, parede_leste: true }),
      celulaSimulada(0, 1, { corrida_id: 25, parede_oeste: true, parede_norte: true }),
    ]);

    const { result } = renderHook(() => useTelemetriaAoVivo());
    await waitFor(() => expect(result.current.mapa).toHaveLength(2));
    expect(result.current.mapa).toEqual([
      { linha: 0, coluna: 0, parede_norte: false, parede_sul: true, parede_leste: true, parede_oeste: true },
      { linha: 0, coluna: 1, parede_norte: true, parede_sul: true, parede_leste: false, parede_oeste: true },
    ]);
  });

  it('recebe atualizações de células do mapa em tempo real via Realtime (#239)', async () => {
    largar(corridaSimulada({ id: 26 }));
    const { result } = renderHook(() => useTelemetriaAoVivo());
    await waitFor(() => expect(result.current.estado).toBe('em_execucao'));

    await act(async () => {
      await clienteSimulado
        .from<MapaCelula & { corrida_id: number }>('mapa_celulas')
        .insert(celulaSimulada(1, 0, { corrida_id: 26, parede_norte: true, parede_leste: false }));
    });

    expect(result.current.mapa).toHaveLength(1);
    expect(result.current.mapa[0].linha).toBe(1);
    expect(result.current.mapa[0].coluna).toBe(0);
    expect(result.current.mapa[0].parede_norte).toBe(true);
    expect(result.current.mapa[0].parede_leste).toBe(false);
  });

  it('mostra erro se a consulta do mapa de células falhar', async () => {
    largar(corridaSimulada({ id: 27 }));
    clienteSimulado.definirErro('mapa_celulas', 'falha simulada ao ler mapa');
    const { result } = renderHook(() => useTelemetriaAoVivo());
    await waitFor(() => expect(result.current.erro).toMatch(/mapa da corrida/));
    expect(result.current.estado).toBe('em_execucao');
  });

  it('cada montagem usa canais com nome próprio (o supabase-js reaproveita canal de mesmo nome)', () => {
    renderHook(() => useTelemetriaAoVivo());
    renderHook(() => useTelemetriaAoVivo());
    const nomes = clienteSimulado.obterCanais().map((c) => c.nome);
    expect(nomes).toHaveLength(6);
    expect(new Set(nomes).size).toBe(6);
  });

  it('no StrictMode (montar, desmontar, montar) continua recebendo as leituras', async () => {
    largar(corridaSimulada({ id: 14 }));
    const { result } = renderHook(() => useTelemetriaAoVivo(), { wrapper: StrictMode });
    await waitFor(() => expect(result.current.estado).toBe('em_execucao'));

    await enviar(leituraSimulada(1, { corrida_id: 14 }));
    expect(result.current.leituras).toHaveLength(1);
    expect(clienteSimulado.obterCanais()).toHaveLength(3);
  });

  it('ao desmontar, remove o canal e para de reagir', async () => {
    largar(corridaSimulada({ id: 13 }));
    const { result, unmount } = renderHook(() => useTelemetriaAoVivo());
    await waitFor(() => expect(result.current.estado).toBe('em_execucao'));

    unmount();
    await act(async () => {});
    expect(clienteSimulado.obterCanais()).toHaveLength(0);
    await enviar(leituraSimulada(1, { corrida_id: 13 }));
    expect(result.current.leituras).toHaveLength(0);
  });
});
