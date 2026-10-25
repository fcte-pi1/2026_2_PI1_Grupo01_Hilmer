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

const seletor = () => screen.getByRole('combobox', { name: 'Labirinto' });

describe('App no modo ao vivo (integração com Supabase simulado)', () => {
  beforeEach(() => {
    clienteSimulado.limpar();
    clienteSimulado.definirTabela('labirintos', labirintos);
  });

  it('sem corrida, aguarda o robô e deixa escolher o labirinto', async () => {
    render(<App />);
    await waitFor(() => expect(screen.getByText(/labirintos lidos do banco/)).toBeInTheDocument());

    expect(screen.getByText('Telemetria ao vivo (Supabase Realtime)')).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'AGUARDANDO LARGADA DO ROBÔ' })).toBeDisabled();
    expect(seletor()).toHaveValue('8x4');
    expect(seletor()).toBeEnabled();
  });

  it('com corrida real, o cabeçalho mostra o labirinto dela e trava o seletor, mesmo depois de concluída', async () => {
    clienteSimulado.definirTabela('corridas', [corridaSimulada({ id: 40, labirinto_id: 1, numero_tentativa: 3 })]);
    render(<App />);
    await waitFor(() => expect(screen.getByRole('button', { name: 'CORRIDA EM ANDAMENTO' })).toBeInTheDocument());

    expect(seletor()).toHaveValue('4x4');
    expect(seletor()).toBeDisabled();

    await act(async () => {
      await clienteSimulado
        .from('telemetria')
        .insert(leituraSimulada(1, { corrida_id: 40, tempo_decorrido: 92.5, evento: 'objetivo_alcancado' }));
    });
    await waitFor(() => expect(screen.getByText('Concluído na Tentativa 3 · Nota 5')).toBeInTheDocument());
    expect(seletor()).toHaveValue('4x4');
    expect(seletor()).toBeDisabled();
    expect(seletor()).toHaveAttribute('title', 'Labirinto definido pela corrida do robô');
  });
});
