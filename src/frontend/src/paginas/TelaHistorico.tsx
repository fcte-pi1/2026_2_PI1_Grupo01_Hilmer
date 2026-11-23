import { useState } from 'react';
import { DetalheCorrida } from '../componentes/DetalheCorrida';
import { FiltroLabirinto, type Filtro } from '../componentes/FiltroLabirinto';
import { TabelaHistorico } from '../componentes/TabelaHistorico';
import { useCorridas } from '../hooks/useCorridas';
import type { Corrida, Labirinto } from '../tipos';
import './TelaHistorico.css';

interface TelaHistoricoProps {
  corridas?: Corrida[];
  labirintos: Labirinto[];
}

/**
 * Tela "Histórico de Consultas" (Frame 04 e filtros M1–M3; HUs 10 e 15): todas as
 * corridas, das mais recentes para as mais antigas, com filtro por labirinto.
 *
 * Consulta dados reais da tabela `corridas` no Supabase via `useCorridas` (HU-10 / Issue #235).
 * O filtro escolhido vai para a consulta ao banco (#234); o filtro abaixo, no navegador, só
 * garante o resultado quando as corridas vêm por props ou o tipo não tem labirinto no banco.
 *
 * Clicar numa corrida abre o detalhe dela (#557) no lugar da tabela. Trocar o filtro fecha o detalhe.
 * Só o `id` da corrida aberta fica no estado: o objeto vem da lista atual, então uma atualização do
 * banco aparece no detalhe, e uma corrida que sumiu da lista fecha o detalhe.
 */
export function TelaHistorico({ corridas: corridasProp, labirintos }: TelaHistoricoProps) {
  const [filtro, setFiltro] = useState<Filtro>('todos');
  const [selecionadaId, setSelecionadaId] = useState<number | null>(null);

  const mudarFiltro = (novo: Filtro) => {
    setFiltro(novo);
    setSelecionadaId(null); // o detalhe não reabre sozinho ao voltar ao filtro anterior
  };
  const labirintoId = filtro === 'todos' ? null : (labirintos.find((l) => l.tipo === filtro)?.id ?? null);
  const hookCorridas = useCorridas(labirintoId);

  const corridas = corridasProp ?? hookCorridas.corridas;
  const { carregando, erro, recarregar } = hookCorridas;

  const idsFiltrados =
    filtro === 'todos' ? null : new Set(labirintos.filter((l) => l.tipo === filtro).map((l) => l.id));
  const visiveis = corridas
    .filter((c) => idsFiltrados === null || idsFiltrados.has(c.labirinto_id))
    .sort((a, b) => Date.parse(b.data_hora_inicio) - Date.parse(a.data_hora_inicio));
  const aberta = selecionadaId === null ? null : (visiveis.find((c) => c.id === selecionadaId) ?? null);

  return (
    <section className="historico cartao" aria-label="Histórico de corridas">
      <div className="historico__topo">
        <FiltroLabirinto valor={filtro} aoMudar={mudarFiltro} />
        <div className="historico__acoes">
          {!corridasProp && (
            <button
              type="button"
              className="historico__recarregar"
              onClick={() => void recarregar()}
              disabled={carregando}
              title="Atualizar histórico de corridas do banco de dados"
            >
              {carregando ? 'Atualizando…' : 'Atualizar'}
            </button>
          )}
          <span className="historico__contagem">
            {visiveis.length} {visiveis.length === 1 ? 'corrida' : 'corridas'}
          </span>
        </div>
      </div>

      {erro && (
        <div className="historico__erro" role="alert">
          <span>{erro}</span>
          <button type="button" onClick={() => void recarregar()} className="historico__erro-botao">
            Tentar novamente
          </button>
        </div>
      )}

      {aberta ? (
        <DetalheCorrida corrida={aberta} labirintos={labirintos} aoVoltar={() => setSelecionadaId(null)} />
      ) : carregando && corridas.length === 0 ? (
        <div className="historico__carregando">
          <p>Carregando histórico do banco de dados…</p>
        </div>
      ) : erro && corridas.length === 0 ? null : (
        // Com erro e sem corridas, só o alerta aparece: "Nenhuma corrida registrada" seria enganoso.
        <TabelaHistorico
          corridas={visiveis}
          labirintos={labirintos}
          filtroAtivo={filtro !== 'todos'}
          aoSelecionar={(corrida) => setSelecionadaId(corrida.id)}
        />
      )}
    </section>
  );
}
