import {
  TRACO,
  descreverTentativa,
  desafioCumprido,
  formatarDataHora,
  formatarTempo,
  formatarVelocidadeOuTraco,
  nomeLabirintoHistorico,
} from '../lib/formatacao';
import type { Corrida, Labirinto } from '../tipos';
import { BadgeDesafio } from './BadgeDesafio';
import './TabelaHistorico.css';

interface TabelaHistoricoProps {
  corridas: Corrida[];
  labirintos: Labirinto[];
  filtroAtivo?: boolean;
  /** Abre o detalhe da corrida (#557). Sem ele, a tabela não tem linhas clicáveis. */
  aoSelecionar?: (corrida: Corrida) => void;
}

/**
 * Tabela do histórico de corridas (Frame 04; HU-10, critério 2). Clicar numa linha (ou no botão
 * "Ver detalhes", pelo teclado) abre o detalhe da corrida (#557).
 */
export function TabelaHistorico({ corridas, labirintos, filtroAtivo = false, aoSelecionar }: TabelaHistoricoProps) {
  const tipoDoLabirinto = (id: number) => labirintos.find((l) => l.id === id)?.tipo;

  return (
    <div className="tabela-historico">
      <table>
        <caption className="visualmente-oculto">Histórico de corridas</caption>
        <thead>
          <tr>
            <th scope="col">Data / Hora</th>
            <th scope="col">Labirinto</th>
            <th scope="col">Tentativa</th>
            <th scope="col">Tempo final</th>
            <th scope="col" title="Sem velocidade registrada, aparece “—”; o detalhe calcula pela distância">
              Vel. média
            </th>
            <th scope="col" title="Carga restante ao final da corrida">Bateria</th>
            <th scope="col">Desafio cumprido (S/N)</th>
            <th scope="col">Status</th>
            {aoSelecionar && (
              <th scope="col">
                <span className="visualmente-oculto">Detalhe</span>
              </th>
            )}
          </tr>
        </thead>
        <tbody>
          {corridas.map((corrida) => {
            const tipo = tipoDoLabirinto(corrida.labirinto_id);
            return (
              <tr
                key={corrida.id}
                className={aoSelecionar ? 'tabela-historico__linha--clicavel' : undefined}
                onClick={aoSelecionar ? () => aoSelecionar(corrida) : undefined}
              >
                <td>{formatarDataHora(corrida.data_hora_inicio)}</td>
                <td>{tipo ? nomeLabirintoHistorico(tipo) : TRACO}</td>
                <td>{descreverTentativa(corrida.numero_tentativa, corrida.status)}</td>
                <td className="numero">{corrida.tempo_final != null ? formatarTempo(corrida.tempo_final) : TRACO}</td>
                <td className="numero">{formatarVelocidadeOuTraco(corrida.velocidade_media)}</td>
                <td className="numero">
                  {corrida.consumo_bateria != null ? `${Math.round(100 - corrida.consumo_bateria)}%` : TRACO}
                </td>
                <td>{desafioCumprido(corrida.status)}</td>
                <td>
                  <BadgeDesafio status={corrida.status} />
                </td>
                {aoSelecionar && (
                  <td>
                    <button
                      type="button"
                      className="tabela-historico__detalhe"
                      onClick={(evento) => {
                        evento.stopPropagation(); // a linha também abre o detalhe: evita abrir duas vezes
                        aoSelecionar(corrida);
                      }}
                    >
                      Ver detalhes
                    </button>
                  </td>
                )}
              </tr>
            );
          })}
        </tbody>
      </table>

      {corridas.length === 0 && (
        <p className="tabela-historico__vazio">
          {filtroAtivo ? 'Nenhuma corrida registrada para este filtro.' : 'Nenhuma corrida registrada.'}
        </p>
      )}
    </div>
  );
}
