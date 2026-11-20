import { act, render, screen, waitFor } from '@testing-library/react';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { App } from './App';
import { criarClienteSupabaseSimulado } from './test/supabaseSimulado';
import { corridaSimulada, leituraSimulada } from './test/dadosAoVivo';
import type { Labirinto } from './tipos';

/*
 * Integração do App com o Supabase simulado (#231): dados inventados, não vêm do robô.
 */
const clienteSimulado = criarClienteSupabaseSimulado();

vi.mock('./lib/supabase', () => ({
  get supabase() {
    return clienteSimulado;
  },
}));

vi.mock('./componentes/GraficoVelocidade', () => ({
  GraficoVelocidade: () => <p>gráfico</p>,
}));

const labirintos: Labirinto[] = [
  { id: 1, tipo: '4x4', linhas: 4, colunas: 4 },
  { id: 2, tipo: '8x4', linhas: 8, colunas: 4 },
  { id: 3, tipo: '12x4', linhas: 12, colunas: 4 },
];

describe('App no modo ao vivo (integração com Supabase simulado)', () => {
  beforeEach(() => {
    clienteSimulado.limpar();
    clienteSimulado.definirTabela('labirintos', labirintos);
  });

  it('sem corrida, aguarda o robô e não deixa escolher o labirinto (ele é surpresa)', async () => {
    render(<App />);
    await waitFor(() => expect(screen.getByText(/labirintos lidos do banco/)).toBeInTheDocument());

    expect(screen.getByText('Telemetria ao vivo (Supabase Realtime)')).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'AGUARDANDO LARGADA DO ROBÔ' })).toBeDisabled();
    expect(screen.queryByRole('combobox', { name: 'Labirinto' })).not.toBeInTheDocument();
    expect(screen.getByRole('status', { name: 'Labirinto' })).toHaveTextContent('O robô ainda está descobrindo o labirinto');
  });

  it('com corrida real, só mostra o labirinto depois de o robô descobri-lo e informá-lo', async () => {
    clienteSimulado.definirTabela('corridas', [corridaSimulada({ id: 40, labirinto_id: 1, numero_tentativa: 3 })]);
    render(<App />);
    await waitFor(() => expect(screen.getByRole('button', { name: 'AGUARDANDO LARGADA DO ROBÔ' })).toBeInTheDocument());

    // A corrida nasce como 4x4 (o padrão do firmware), mas o tamanho real ainda não é conhecido: sem
    // seletor, sem grade e com o aviso de que o robô está descobrindo.
    expect(screen.queryByRole('combobox', { name: 'Labirinto' })).not.toBeInTheDocument();
    expect(screen.getByRole('status', { name: 'Labirinto' })).toHaveTextContent('descobrindo');
    expect(screen.getByText(/descobrindo o tamanho do labirinto/)).toBeInTheDocument();

    // O Realtime simulado atualiza o estado do React: a inserção entra num `act`.
    await act(async () => {
      await clienteSimulado
        .from('telemetria')
        .insert(leituraSimulada(1, { corrida_id: 40, tempo_decorrido: 92.5, evento: 'objetivo_alcancado' }));
    });
    await waitFor(() => expect(screen.getByText('Concluído na Tentativa 3 · Nota 5')).toBeInTheDocument());
    expect(screen.getByRole('status', { name: 'Labirinto' })).toHaveTextContent('4x4 · descoberto pelo robô');
    expect(screen.queryByText(/descobrindo o tamanho do labirinto/)).not.toBeInTheDocument();
  });

  it('quando o robô informa o tamanho real, a corrida passa para o labirinto descoberto', async () => {
    clienteSimulado.definirTabela('corridas', [corridaSimulada({ id: 41, labirinto_id: 1, numero_tentativa: 1 })]);
    render(<App />);
    await waitFor(() => expect(screen.getByRole('status', { name: 'Labirinto' })).toHaveTextContent('descobrindo'));

    // O robô se localizou (8x4), o banco trocou o labirinto da corrida e a primeira leitura saiu.
    act(() =>
      clienteSimulado.emitirRealtime('corridas', 'UPDATE', corridaSimulada({ id: 41, labirinto_id: 2, numero_tentativa: 1 })),
    );
    await act(async () => {
      await clienteSimulado.from('telemetria').insert(leituraSimulada(1, { corrida_id: 41, posicao_coluna: 5 }));
    });
    await waitFor(() =>
      expect(screen.getByRole('status', { name: 'Labirinto' })).toHaveTextContent('8x4 · descoberto pelo robô'),
    );
  });
});
