import { act, render, screen, waitFor, within } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { CHAVE_SENHA_DO_PAINEL, ControleRobo } from './ControleRobo';
import { criarClienteSupabaseSimulado, type ClienteSupabaseSimulado } from '../test/supabaseSimulado';
import type { ComandoRegistrado, LogRobo } from '../lib/robo';

const clienteSimulado = criarClienteSupabaseSimulado();
let clienteAtual: ClienteSupabaseSimulado | null = clienteSimulado;

vi.mock('../lib/supabase', () => ({
  get supabase() {
    return clienteAtual;
  },
}));

const log = (id: number, mensagem: string): LogRobo => ({ id, corrida_id: null, criado_em: '2026-11-17T12:00:00Z', mensagem });

describe('ControleRobo', () => {
  beforeEach(() => {
    clienteSimulado.limpar();
    clienteAtual = clienteSimulado;
    window.sessionStorage.clear();
  });
  afterEach(() => {
    vi.useRealTimers();
  });

  it('sem Supabase, avisa e não mostra os botões', () => {
    clienteAtual = null;
    render(<ControleRobo />);
    expect(screen.getByText(/Configure o Supabase/)).toBeInTheDocument();
    expect(screen.queryByRole('button', { name: 'INICIAR' })).not.toBeInTheDocument();
  });

  it('manda iniciar com a senha digitada e a guarda só nesta sessão', async () => {
    const aoEnviar = vi.fn().mockResolvedValue({ ok: true, id: 1 });
    render(<ControleRobo aoEnviar={aoEnviar} />);
    await userEvent.type(screen.getByLabelText('Senha do painel'), 'abc123');
    await userEvent.click(screen.getByRole('button', { name: 'INICIAR' }));
    expect(aoEnviar).toHaveBeenCalledWith('iniciar', 'abc123');
    expect(await screen.findByText(/Iniciar enviado/)).toBeInTheDocument();
    expect(window.sessionStorage.getItem(CHAVE_SENHA_DO_PAINEL)).toBe('abc123');
    expect(window.localStorage.getItem(CHAVE_SENHA_DO_PAINEL)).toBeNull();
  });

  it('lembra a senha da sessão ao abrir de novo', () => {
    window.sessionStorage.setItem(CHAVE_SENHA_DO_PAINEL, 'guardada');
    render(<ControleRobo aoEnviar={vi.fn()} />);
    expect(screen.getByLabelText('Senha do painel')).toHaveValue('guardada');
  });

  it('PARAR e ZERAR mandam os comandos certos', async () => {
    const aoEnviar = vi.fn().mockResolvedValue({ ok: true, id: 2 });
    render(<ControleRobo aoEnviar={aoEnviar} />);
    await userEvent.type(screen.getByLabelText('Senha do painel'), 's');
    await userEvent.click(screen.getByRole('button', { name: 'PARAR' }));
    await userEvent.click(screen.getByRole('button', { name: 'ZERAR' }));
    expect(aoEnviar.mock.calls.map((c) => c[0])).toEqual(['parar', 'zerar']);
  });

  it('mostra o erro do envio e não guarda a senha recusada', async () => {
    const aoEnviar = vi.fn().mockResolvedValue({ ok: false, mensagem: 'Senha do painel incorreta.' });
    render(<ControleRobo aoEnviar={aoEnviar} />);
    await userEvent.type(screen.getByLabelText('Senha do painel'), 'errada');
    await userEvent.click(screen.getByRole('button', { name: 'INICIAR' }));
    expect(await screen.findByText('Senha do painel incorreta.')).toBeInTheDocument();
    expect(window.sessionStorage.getItem(CHAVE_SENHA_DO_PAINEL)).toBeNull();
  });

  it('desabilita os botões enquanto o comando está sendo enviado', async () => {
    let concluir: (r: { ok: true; id: number }) => void = () => {};
    const aoEnviar = vi.fn().mockReturnValue(new Promise((resolve) => (concluir = resolve)));
    render(<ControleRobo aoEnviar={aoEnviar} />);
    await userEvent.click(screen.getByRole('button', { name: 'INICIAR' }));
    expect(screen.getByRole('button', { name: 'PARAR' })).toBeDisabled();
    await act(async () => concluir({ ok: true, id: 1 }));
    expect(screen.getByRole('button', { name: 'PARAR' })).toBeEnabled();
  });

  it('mostra os logs do robô, traduzidos, e os novos que chegam ao vivo', async () => {
    clienteSimulado.definirTabela('logs_robo', [log(1, '{"status":"aguardando_largada"}')]);
    render(<ControleRobo aoEnviar={vi.fn()} />);
    const lista = screen.getByRole('list', { name: 'Logs do robô' });
    expect(await within(lista).findByText('Calibrado: aguardando a largada')).toBeInTheDocument();
    await act(async () => {
      await clienteSimulado.from('logs_robo').insert(log(2, '{"status":"colisao","tipo":"impacto"}'));
    });
    const colisao = within(lista).getByText('Colisão detectada (tipo=impacto)').closest('li');
    expect(colisao).toHaveClass('controle__log--aviso');
  });

  it('sem logs, explica o que fazer', () => {
    render(<ControleRobo aoEnviar={vi.fn()} />);
    expect(screen.getByText(/Nenhum log do robô ainda/)).toBeInTheDocument();
  });

  it('acompanha o comando: aguardando, entregue e expirado', async () => {
    const pendente: ComandoRegistrado = {
      id: 3,
      comando: 'iniciar',
      criado_em: new Date().toISOString(),
      entregue_em: null,
    };
    clienteSimulado.definirTabela('comandos_robo', [pendente]);
    render(<ControleRobo aoEnviar={vi.fn()} />);
    expect(await screen.findByText(/Iniciar, aguardando o robô buscá-lo/)).toBeInTheDocument();

    act(() =>
      clienteSimulado.emitirRealtime('comandos_robo', 'UPDATE', { ...pendente, entregue_em: new Date().toISOString() }),
    );
    expect(await screen.findByText(/Iniciar, entregue ao robô às/)).toBeInTheDocument();

    const velho: ComandoRegistrado = {
      id: 4,
      comando: 'parar',
      criado_em: new Date(Date.now() - 60_000).toISOString(),
      entregue_em: null,
    };
    act(() => clienteSimulado.emitirRealtime('comandos_robo', 'INSERT', velho));
    await waitFor(() => expect(screen.getByText(/Parar, expirou sem o robô buscá-lo/)).toBeInTheDocument());
  });
});
