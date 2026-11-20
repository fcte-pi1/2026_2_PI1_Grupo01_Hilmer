import { StrictMode } from 'react';
import { act, renderHook, waitFor } from '@testing-library/react';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { CANAL_DO_ROBO, useCanalDoRobo } from './useCanalDoRobo';
import { criarClienteSupabaseSimulado, type ClienteSupabaseSimulado } from '../test/supabaseSimulado';
import type { ComandoRegistrado, LogRobo } from '../lib/robo';

const clienteSimulado = criarClienteSupabaseSimulado();
let clienteAtual: ClienteSupabaseSimulado | null = clienteSimulado;

vi.mock('../lib/supabase', () => ({
  get supabase() {
    return clienteAtual;
  },
}));

const log = (id: number, mensagem = `{"status":"largada_liberada"}`): LogRobo => ({
  id,
  corrida_id: null,
  criado_em: '2026-11-17T12:00:00Z',
  mensagem,
});
const comando = (id: number, entregue: string | null = null): ComandoRegistrado => ({
  id,
  comando: 'iniciar',
  criado_em: '2026-11-17T12:00:00Z',
  entregue_em: entregue,
});

describe('useCanalDoRobo (integração com Supabase simulado)', () => {
  beforeEach(() => {
    clienteSimulado.limpar();
    clienteAtual = clienteSimulado;
  });

  it('sem Supabase configurado, fica indisponível e vazio', () => {
    clienteAtual = null;
    const { result } = renderHook(() => useCanalDoRobo());
    expect(result.current).toMatchObject({ disponivel: false, logs: [], ultimoComando: null, erro: null });
  });

  it('assina logs e comandos em canais separados', () => {
    renderHook(() => useCanalDoRobo());
    const nomes = clienteSimulado.obterCanais().map((c) => c.nome);
    expect(nomes).toHaveLength(2);
    expect(nomes[0]).toMatch(new RegExp(`^${CANAL_DO_ROBO}-logs-\\d+$`));
    expect(nomes[1]).toMatch(new RegExp(`^${CANAL_DO_ROBO}-comandos-\\d+$`));
  });

  it('ao abrir, carrega os logs já gravados em ordem e o último comando', async () => {
    clienteSimulado.definirTabela('logs_robo', [log(2), log(1), log(3)]);
    clienteSimulado.definirTabela('comandos_robo', [comando(4, '2026-11-17T12:00:03Z'), comando(5)]);
    const { result } = renderHook(() => useCanalDoRobo());
    await waitFor(() => expect(result.current.logs.map((l) => l.id)).toEqual([1, 2, 3]));
    expect(result.current.ultimoComando?.id).toBe(5);
  });

  it('um log novo do robô chega pelo Realtime, sem repetir', async () => {
    const { result } = renderHook(() => useCanalDoRobo());
    await act(async () => {});
    await act(async () => {
      await clienteSimulado.from('logs_robo').insert(log(1));
      await clienteSimulado.from('logs_robo').insert(log(2));
    });
    expect(result.current.logs.map((l) => l.id)).toEqual([1, 2]);
    act(() => clienteSimulado.emitirRealtime('logs_robo', 'INSERT', log(2)));
    expect(result.current.logs).toHaveLength(2);
  });

  it('o robô buscar o comando (UPDATE) atualiza o último comando', async () => {
    const { result } = renderHook(() => useCanalDoRobo());
    await act(async () => {});
    act(() => clienteSimulado.emitirRealtime('comandos_robo', 'INSERT', comando(7)));
    expect(result.current.ultimoComando?.entregue_em).toBeNull();
    act(() => clienteSimulado.emitirRealtime('comandos_robo', 'UPDATE', comando(7, '2026-11-17T12:00:02Z')));
    expect(result.current.ultimoComando?.entregue_em).toBe('2026-11-17T12:00:02Z');
  });

  it('mostra o erro quando a leitura falha', async () => {
    clienteSimulado.definirErro('logs_robo', 'relation does not exist');
    const { result } = renderHook(() => useCanalDoRobo());
    await waitFor(() => expect(result.current.erro).toMatch(/logs do robô/));
  });

  it('sobrevive ao StrictMode e fecha os canais ao desmontar', async () => {
    const { result, unmount } = renderHook(() => useCanalDoRobo(), { wrapper: StrictMode });
    await act(async () => {});
    await act(async () => {
      await clienteSimulado.from('logs_robo').insert(log(1));
    });
    expect(result.current.logs).toHaveLength(1);
    unmount();
    expect(clienteSimulado.obterCanais().filter((c) => c.estado === 'SUBSCRIBED')).toHaveLength(0);
  });
});
