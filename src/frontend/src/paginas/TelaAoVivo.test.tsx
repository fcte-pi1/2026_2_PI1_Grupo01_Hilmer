import { act, render, screen, waitFor, within } from '@testing-library/react';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { TelaAoVivo } from './TelaAoVivo';
import { useTelemetriaAoVivo } from '../hooks/useTelemetriaAoVivo';
import { criarClienteSupabaseSimulado } from '../test/supabaseSimulado';
import { celulaSimulada, corridaSimulada, leituraSimulada } from '../test/dadosAoVivo';
import type { Corrida, Labirinto, MapaCelula, Telemetria } from '../tipos';

/*
 * Integração da tela ao vivo com o hook e o Supabase simulado (#231, #239). O teste faz o papel do
 * robô: grava linhas inventadas em `telemetria` e `mapa_celulas` e confere o que a tela mostra.
 */
const clienteSimulado = criarClienteSupabaseSimulado();

vi.mock('../lib/supabase', () => ({
  get supabase() {
    return clienteSimulado;
  },
}));

// O gráfico (Recharts) não é o foco aqui e precisa de medidas de layout que o jsdom não tem.
vi.mock('../componentes/GraficoVelocidade', () => ({
  GraficoVelocidade: ({ leituras }: { leituras: Telemetria[] }) => <p>gráfico com {leituras.length} leituras</p>,
}));

const labirinto: Labirinto = { id: 1, tipo: '4x4', linhas: 4, colunas: 4 };

function PainelReal() {
  const aoVivo = useTelemetriaAoVivo();
  return (
    <TelaAoVivo
      labirinto={labirinto}
      mapa={aoVivo.mapa}
      estado={aoVivo.estado}
      tempo={aoVivo.tempo}
      leituras={aoVivo.leituras}
      tentativa={aoVivo.corrida?.numero_tentativa ?? 1}
      aoVivo={aoVivo.disponivel}
      aoIniciar={() => {}}
      aoResetar={() => {}}
    />
  );
}

const cartao = (rotulo: string) => within(screen.getByRole('region', { name: rotulo }));
const limite = () => screen.getByRole('progressbar', { name: /limite de 10:00/ });

async function enviar(leitura: Telemetria) {
  await act(async () => {
    await clienteSimulado.from<Telemetria>('telemetria').insert(leitura);
  });
}

describe('TelaAoVivo (integração com Supabase simulado)', () => {
  beforeEach(() => {
    clienteSimulado.limpar();
  });

  it('antes da largada, aguarda o robô com cards nominais e o limite zerado', async () => {
    render(<PainelReal />);
    await act(async () => {});

    expect(screen.getByRole('button', { name: 'AGUARDANDO LARGADA DO ROBÔ' })).toBeDisabled();
    expect(screen.queryByRole('button', { name: 'INICIAR CORRIDA' })).not.toBeInTheDocument();
    expect(cartao('Tempo de conclusão').getByText('00:00.000')).toBeInTheDocument();
    expect(cartao('Tempo de conclusão').getByText(/Limite: 10:00/)).toBeInTheDocument();
    expect(limite()).toHaveAttribute('aria-valuenow', '0');
  });

  it('atualiza cards, cronômetro e marcador do limite a cada linha inserida', async () => {
    clienteSimulado.definirTabela<Corrida>('corridas', [corridaSimulada({ id: 20, numero_tentativa: 2 })]);
    render(<PainelReal />);
    await waitFor(() => screen.getByRole('button', { name: 'CORRIDA EM ANDAMENTO' }));
    expect(cartao('Tempo de conclusão').getByText(/Tentativa 2 de 3/)).toBeInTheDocument();

    await enviar(
      leituraSimulada(1, {
        corrida_id: 20,
        velocidade: 0.2,
        tensao_bateria: 8.0,
        corrente_bateria: 0.35,
        tempo_decorrido: 60,
      }),
    );
    expect(cartao('Consumo de bateria').getByText('8.0V · 350 mA')).toBeInTheDocument();
    expect(cartao('Velocidade média').getByText('0.20 m/s')).toBeInTheDocument();
    expect(cartao('Tempo de conclusão').getByText('01:00.000')).toBeInTheDocument();
    expect(limite()).toHaveAttribute('aria-valuenow', '60');
    expect(screen.getByText('gráfico com 1 leituras')).toBeInTheDocument();

    await enviar(
      leituraSimulada(2, {
        corrida_id: 20,
        velocidade: 0.4,
        tensao_bateria: 7.8,
        corrente_bateria: 0.4,
        tempo_decorrido: 150,
      }),
    );
    expect(cartao('Consumo de bateria').getByText('7.8V · 400 mA')).toBeInTheDocument();
    expect(cartao('Velocidade média').getByText('0.30 m/s')).toBeInTheDocument();
    expect(cartao('Velocidade média').getByText('Pico máx: 0.40 m/s')).toBeInTheDocument();
    expect(cartao('Tempo de conclusão').getByText('02:30.000')).toBeInTheDocument();
    expect(limite()).toHaveAttribute('aria-valuenow', '150');
    expect(screen.getByText('gráfico com 2 leituras')).toBeInTheDocument();
  });

  it('mostra a conclusão com a nota quando o robô chega ao objetivo', async () => {
    clienteSimulado.definirTabela<Corrida>('corridas', [corridaSimulada({ id: 21 })]);
    render(<PainelReal />);
    await waitFor(() => screen.getByRole('button', { name: 'CORRIDA EM ANDAMENTO' }));

    await enviar(leituraSimulada(1, { corrida_id: 21, tempo_decorrido: 45.25, evento: 'objetivo_alcancado' }));
    expect(cartao('Tempo de conclusão').getByText('00:45.250')).toBeInTheDocument();
    expect(cartao('Tempo de conclusão').getByText('Concluído na Tentativa 1 · Nota 10')).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'AGUARDANDO A PRÓXIMA TENTATIVA' })).toBeDisabled();
    expect(screen.queryByRole('button', { name: /RESETAR/ })).not.toBeInTheDocument();
  });

  it('esgota o tempo em 10:00 e marca o limite como esgotado', async () => {
    clienteSimulado.definirTabela<Corrida>('corridas', [corridaSimulada({ id: 22 })]);
    render(<PainelReal />);
    await waitFor(() => screen.getByRole('button', { name: 'CORRIDA EM ANDAMENTO' }));

    await enviar(leituraSimulada(1, { corrida_id: 22, tempo_decorrido: 612 }));
    expect(cartao('Tempo de conclusão').getByText('10:00.000')).toBeInTheDocument();
    expect(cartao('Tempo de conclusão').getByText(/Tempo esgotado/)).toBeInTheDocument();
    expect(limite()).toHaveAttribute('aria-valuenow', '600');
    expect(limite()).toHaveClass('limite--esgotado');
  });

  it('desenha a malha com largada verde e chegada vermelha, atualizando robô, percurso e paredes em tempo real (#239)', async () => {
    clienteSimulado.definirTabela<Corrida>('corridas', [corridaSimulada({ id: 23 })]);
    render(<PainelReal />);
    await waitFor(() => screen.getByRole('button', { name: 'CORRIDA EM ANDAMENTO' }));

    // Malha dimensional com largada e chegada
    const partida = screen.getByTestId('partida');
    const chegada = screen.getByTestId('chegada');
    expect(partida).toHaveClass('grade__largada');
    expect(chegada).toHaveClass('grade__objetivo');

    // Primeira telemetria: robô na largada (0, 0)
    await enviar(leituraSimulada(1, { corrida_id: 23, posicao_linha: 0, posicao_coluna: 0, orientacao: 'N' }));
    expect(screen.getByTestId('robot')).toHaveAttribute('transform', 'translate(30,174) rotate(0)');
    expect(screen.queryByTestId('robot-path')).not.toBeInTheDocument();

    // Segunda telemetria: robô avança para (1, 0)
    await enviar(leituraSimulada(2, { corrida_id: 23, posicao_linha: 1, posicao_coluna: 0, orientacao: 'N' }));
    expect(screen.getByTestId('robot')).toHaveAttribute('transform', 'translate(30,126) rotate(0)');
    expect(screen.getByTestId('robot-path')).toHaveAttribute('points', '30,174 30,126');

    // Evento Realtime de parede mapeada
    await act(async () => {
      await clienteSimulado
        .from<MapaCelula & { corrida_id: number }>('mapa_celulas')
        .insert(celulaSimulada(0, 0, { corrida_id: 23, parede_leste: true }));
    });
    const grupoParedes = screen.getByTestId('paredes');
    expect(grupoParedes.querySelectorAll('line').length).toBeGreaterThan(0);
  });

  it('mantém o traçado completo e o mapa ao final da corrida (#239)', async () => {
    clienteSimulado.definirTabela<Corrida>('corridas', [corridaSimulada({ id: 24 })]);
    render(<PainelReal />);
    await waitFor(() => screen.getByRole('button', { name: 'CORRIDA EM ANDAMENTO' }));

    await enviar(leituraSimulada(1, { corrida_id: 24, posicao_linha: 0, posicao_coluna: 0 }));
    await enviar(leituraSimulada(2, { corrida_id: 24, posicao_linha: 1, posicao_coluna: 0 }));
    await enviar(
      leituraSimulada(3, {
        corrida_id: 24,
        posicao_linha: 3,
        posicao_coluna: 3,
        evento: 'objetivo_alcancado',
      }),
    );

    // Corrida concluída
    expect(screen.getByRole('button', { name: 'AGUARDANDO A PRÓXIMA TENTATIVA' })).toBeDisabled();
    // Traçado completo preservado
    const path = screen.getByTestId('robot-path');
    expect(path).toBeInTheDocument();
    expect(path.getAttribute('points')?.split(' ')).toHaveLength(3);
    expect(screen.getByTestId('robot')).toHaveAttribute('transform', 'translate(174,30) rotate(0)');
  });
});

describe('TelaAoVivo fora do modo ao vivo (corrida de exemplo)', () => {
  it('mantém os botões de iniciar e resetar', () => {
    const props = { labirinto, mapa: [], tempo: 0, leituras: [], tentativa: 1, aoIniciar: vi.fn(), aoResetar: vi.fn() };
    const { rerender } = render(<TelaAoVivo {...props} estado="aguardando" />);
    screen.getByRole('button', { name: 'INICIAR CORRIDA' }).click();
    expect(props.aoIniciar).toHaveBeenCalledOnce();

    rerender(<TelaAoVivo {...props} estado="concluido" />);
    screen.getByRole('button', { name: 'RESETAR / NOVA TENTATIVA' }).click();
    expect(props.aoResetar).toHaveBeenCalledOnce();
  });
});
