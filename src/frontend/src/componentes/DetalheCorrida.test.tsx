import { render, screen, waitFor, within } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { beforeEach, describe, expect, it, vi } from 'vitest';
import { TelaHistorico } from '../paginas/TelaHistorico';
import { criarClienteSupabaseSimulado } from '../test/supabaseSimulado';
import { celulaSimulada, corridaSimulada, leituraSimulada } from '../test/dadosAoVivo';
import type { Labirinto, Telemetria } from '../tipos';

/*
 * Detalhe de uma corrida do histórico (#557), com o Supabase simulado (mock): nenhuma linha vem do
 * robô, os valores são inventados no formato das tabelas `corridas`, `telemetria`, `mapa_celulas` e `logs_robo`.
 */
const clienteSimulado = criarClienteSupabaseSimulado();

vi.mock('../lib/supabase', () => ({
  get supabase() {
    return clienteSimulado;
  },
}));

// O Recharts precisa de medidas de layout que o jsdom não tem.
vi.mock('./GraficoVelocidade', () => ({
  GraficoVelocidade: ({ leituras }: { leituras: Telemetria[] }) => (
    <p>
      gráfico com {leituras.length} leituras, primeira a {leituras[0]?.velocidade.toFixed(2)} m/s
    </p>
  ),
}));

const labirintos: Labirinto[] = [
  { id: 1, tipo: '4x4', linhas: 4, colunas: 4 },
  { id: 2, tipo: '8x4', linhas: 8, colunas: 4 },
];

// Corrida 20: concluída, com velocidade_media = 0 gravada (o robô manda velocidade 0).
const concluida = corridaSimulada({
  id: 20,
  labirinto_id: 1,
  status: 'concluida',
  data_hora_inicio: '2026-11-20T14:00:00Z',
  data_hora_fim: '2026-11-20T14:00:04Z',
  tempo_final: 4,
  velocidade_media: 0,
  velocidade_pico: 0,
  consumo_bateria: 10,
});
// Corrida 21: antiga, sem telemetria nem logs.
const antiga = corridaSimulada({
  id: 21,
  labirinto_id: 2,
  numero_tentativa: 2,
  status: 'nao_concluida',
  data_hora_inicio: '2026-11-19T10:00:00Z',
  tempo_final: 600,
  velocidade_media: null,
  consumo_bateria: null,
});

// 4 células trocadas (0,72 m) em 4 s → 0,18 m/s
const leituras: Telemetria[] = [
  leituraSimulada(1, { corrida_id: 20, estado_corrida: 'aguardando', velocidade: 0, tempo_decorrido: 0 }),
  leituraSimulada(2, { corrida_id: 20, posicao_linha: 1, velocidade: 0, tempo_decorrido: 1 }),
  leituraSimulada(3, { corrida_id: 20, posicao_linha: 2, velocidade: 0, tempo_decorrido: 2 }),
  leituraSimulada(4, { corrida_id: 20, posicao_linha: 2, posicao_coluna: 1, velocidade: 0, tempo_decorrido: 3 }),
  leituraSimulada(5, { corrida_id: 20, posicao_linha: 3, posicao_coluna: 1, velocidade: 0, tempo_decorrido: 4, evento: 'objetivo_alcancado', tensao_bateria: 7.9 }),
];

const logs = [
  { id: 2, corrida_id: 20, criado_em: '2026-11-20T14:00:02Z', mensagem: '{"status":"colisao","tipo":"impacto"}' },
  { id: 1, corrida_id: 20, criado_em: '2026-11-20T14:00:00Z', mensagem: '{"status":"largada_liberada"}' },
  { id: 3, corrida_id: 99, criado_em: '2026-11-20T15:00:00Z', mensagem: 'log de outra corrida' },
];

async function abrirDetalhe(user: ReturnType<typeof userEvent.setup>, rotuloLabirinto: string) {
  render(<TelaHistorico labirintos={labirintos} />);
  await waitFor(() => expect(screen.getByText('2 corridas')).toBeInTheDocument());
  const linha = screen.getByRole('cell', { name: rotuloLabirinto }).closest('tr')!;
  await user.click(within(linha).getByRole('button', { name: 'Ver detalhes' }));
  return screen.getByRole('region', { name: 'Detalhe da corrida' });
}

describe('Detalhe da corrida no histórico (#557)', () => {
  beforeEach(() => {
    clienteSimulado.limpar();
    clienteSimulado.definirTabela('corridas', [concluida, antiga]);
    clienteSimulado.definirTabela('telemetria', leituras);
    clienteSimulado.definirTabela('mapa_celulas', [celulaSimulada(0, 0, { corrida_id: 20 }), celulaSimulada(1, 0, { corrida_id: 20 })]);
    clienteSimulado.definirTabela('logs_robo', logs);
  });

  it('a tabela mostra "Desafio cumprido (S/N)" e "—" no lugar de 0,0 m/s', async () => {
    render(<TelaHistorico labirintos={labirintos} />);
    await waitFor(() => expect(screen.getByText('2 corridas')).toBeInTheDocument());
    expect(screen.getByRole('columnheader', { name: 'Desafio cumprido (S/N)' })).toBeInTheDocument();
    const [primeira, segunda] = screen.getAllByRole('row').slice(1);
    expect(within(primeira).getByRole('cell', { name: 'Sim' })).toBeInTheDocument();
    expect(within(segunda).getByRole('cell', { name: 'Não' })).toBeInTheDocument();
    expect(screen.queryByText(/0\.00 m\/s/)).not.toBeInTheDocument();
  });

  it('clicar numa corrida concluída mostra mapa, trajeto, gráfico, cartões e logs dela', async () => {
    const user = userEvent.setup();
    const detalhe = await abrirDetalhe(user, 'Labirinto 1 (4×4)');
    await waitFor(() => expect(within(detalhe).getByLabelText('Logs da corrida')).toBeInTheDocument());

    // Mapa e trajeto (GradeLabirinto) e gráfico com a velocidade estimada pela distância
    expect(
      within(detalhe).getByRole('img', { name: /Labirinto de 4 linhas.*Robô na linha 3, coluna 1.*4 passos percorridos/ }),
    ).toBeInTheDocument();
    expect(within(detalhe).getByText(/estimada pela distância/)).toBeInTheDocument();
    expect(within(detalhe).getByText('gráfico com 4 leituras, primeira a 0.18 m/s')).toBeInTheDocument();

    // Cartões: velocidade pela distância (0,72 m ÷ 4 s), nunca 0,00
    const cartao = (rotulo: string) => within(within(detalhe).getByRole('region', { name: rotulo }));
    expect(cartao('Tempo final').getByText('00:04.000')).toBeInTheDocument();
    expect(cartao('Velocidade média').getByText('0.18 m/s')).toBeInTheDocument();
    expect(cartao('Velocidade média').getByText(/Pela distância/)).toBeInTheDocument();
    expect(cartao('Bateria restante').getByText('90%')).toBeInTheDocument();
    expect(within(detalhe).getByText('DESAFIO CUMPRIDO: SIM')).toBeInTheDocument();

    // Logs só desta corrida, em ordem cronológica
    const itens = within(within(detalhe).getByLabelText('Logs da corrida')).getAllByRole('listitem');
    expect(itens.map((i) => i.textContent)).toEqual([
      expect.stringContaining('Largada liberada'),
      expect.stringContaining('Colisão detectada'),
    ]);
    expect(within(detalhe).queryByText('log de outra corrida')).not.toBeInTheDocument();

    await user.click(within(detalhe).getByRole('button', { name: '← Voltar ao histórico' }));
    expect(screen.getByRole('table')).toBeInTheDocument();
  });

  it('corrida que andou pouco e esgotou o tempo não mostra 0.00 m/s', async () => {
    const user = userEvent.setup();
    clienteSimulado.definirTabela('corridas', [{ ...concluida, velocidade_media: 0.004, velocidade_pico: 0.15 }, antiga]);
    const detalhe = await abrirDetalhe(user, 'Labirinto 1 (4×4)');
    const cartao = within(within(detalhe).getByRole('region', { name: 'Velocidade média' }));
    expect(cartao.getByText('< 0.01 m/s')).toBeInTheDocument();
    expect(cartao.getByText('Pico máx: 0.15 m/s')).toBeInTheDocument();
    expect(within(detalhe).queryByText(/0\.00 m\/s/)).not.toBeInTheDocument();
  });

  it('corrida sem telemetria e sem logs mostra mensagens claras, sem quebrar a tela', async () => {
    const user = userEvent.setup();
    const detalhe = await abrirDetalhe(user, 'Labirinto 2 (8×4)');
    await waitFor(() => expect(within(detalhe).getByText(/não tem telemetria gravada/)).toBeInTheDocument());
    expect(within(detalhe).getByText(/Nenhum log gravado para esta corrida/)).toBeInTheDocument();
    const cartao = within(within(detalhe).getByRole('region', { name: 'Velocidade média' }));
    expect(cartao.getByText('—')).toBeInTheDocument();
    expect(within(detalhe).getByText('DESAFIO CUMPRIDO: NÃO')).toBeInTheDocument();
  });

  it('falha nos logs não esconde o mapa; falha na telemetria mostra erro com "Tentar novamente"', async () => {
    const user = userEvent.setup();
    clienteSimulado.definirErro('logs_robo', 'tabela não existe');
    let detalhe = await abrirDetalhe(user, 'Labirinto 1 (4×4)');
    await waitFor(() => expect(within(detalhe).getByText(/logs do robô desta corrida/)).toBeInTheDocument());
    expect(within(detalhe).getByRole('img', { name: /Labirinto de/ })).toBeInTheDocument();
    await user.click(within(detalhe).getByRole('button', { name: '← Voltar ao histórico' }));

    clienteSimulado.definirErro('logs_robo', null);
    clienteSimulado.definirErro('telemetria', 'falha de rede');
    detalhe = (await (async () => {
      const linha = screen.getByRole('cell', { name: 'Labirinto 1 (4×4)' }).closest('tr')!;
      await user.click(linha); // a linha inteira também abre o detalhe
      return screen.getByRole('region', { name: 'Detalhe da corrida' });
    })());
    await waitFor(() => expect(within(detalhe).getByRole('alert')).toHaveTextContent(/telemetria e o mapa/));
    expect(within(detalhe).queryByRole('img', { name: /Labirinto de/ })).not.toBeInTheDocument();

    clienteSimulado.definirErro('telemetria', null);
    await user.click(within(detalhe).getByRole('button', { name: 'Tentar novamente' }));
    await waitFor(() => expect(within(detalhe).getByRole('img', { name: /Labirinto de/ })).toBeInTheDocument());
  });

  it('o filtro vale também no detalhe: trocar para outro labirinto fecha a corrida aberta', async () => {
    const user = userEvent.setup();
    await abrirDetalhe(user, 'Labirinto 1 (4×4)');
    await user.click(screen.getByRole('button', { name: 'Labirinto 2 (8×4)' }));
    await waitFor(() => expect(screen.getByText('1 corrida')).toBeInTheDocument());
    expect(screen.queryByRole('region', { name: 'Detalhe da corrida' })).not.toBeInTheDocument();
    expect(screen.getByRole('cell', { name: 'Labirinto 2 (8×4)' })).toBeInTheDocument();
  });

  it('voltar ao filtro anterior não reabre o detalhe sozinho', async () => {
    const user = userEvent.setup();
    await abrirDetalhe(user, 'Labirinto 1 (4×4)');
    await user.click(screen.getByRole('button', { name: 'Labirinto 2 (8×4)' }));
    await waitFor(() => expect(screen.getByText('1 corrida')).toBeInTheDocument());
    await user.click(screen.getByRole('button', { name: 'Todos os Labirintos' }));
    await waitFor(() => expect(screen.getByText('2 corridas')).toBeInTheDocument());
    expect(screen.queryByRole('region', { name: 'Detalhe da corrida' })).not.toBeInTheDocument();
    expect(screen.getByRole('table')).toBeInTheDocument();
  });

  it('o detalhe aberto mostra a versão atual da corrida depois de "Atualizar"', async () => {
    const user = userEvent.setup();
    const detalhe = await abrirDetalhe(user, 'Labirinto 1 (4×4)');
    const tempoFinal = () => within(within(detalhe).getByRole('region', { name: 'Tempo final' }));
    await waitFor(() => expect(tempoFinal().getByText('00:04.000')).toBeInTheDocument());

    clienteSimulado.definirTabela('corridas', [{ ...concluida, tempo_final: 5.5 }, antiga]);
    await user.click(screen.getByRole('button', { name: 'Atualizar' }));
    await waitFor(() => expect(tempoFinal().getByText('00:05.500')).toBeInTheDocument());
  });
});
