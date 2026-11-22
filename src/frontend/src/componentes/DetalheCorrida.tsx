import { lazy, Suspense } from 'react';
import { useDetalheCorrida } from '../hooks/useDetalheCorrida';
import { useReproducaoCorrida } from '../hooks/useReproducaoCorrida';
import {
  LIMITE_TEMPO_S,
  TRACO,
  descreverTentativa,
  formatarDataHora,
  formatarMinutos,
  formatarTempo,
  formatarVelocidadeOuTraco,
  nomeLabirintoHistorico,
  percentualBateria,
} from '../lib/formatacao';
import { calcularMetricas, leiturasDaCorrida, serieDeVelocidade } from '../lib/painelAoVivo';
import { formatarLog } from '../lib/robo';
import type { Corrida, Labirinto, Posicao } from '../tipos';
import { BadgeDesafio } from './BadgeDesafio';
import { CartaoMetrica } from './CartaoMetrica';
import { GradeLabirinto } from './GradeLabirinto';
import './DetalheCorrida.css';

/** Recharts sob demanda, como na tela ao vivo. */
const GraficoVelocidade = lazy(() =>
  import('./GraficoVelocidade').then((modulo) => ({ default: modulo.GraficoVelocidade })),
);

interface DetalheCorridaProps {
  corrida: Corrida;
  labirintos: Labirinto[];
  aoVoltar: () => void;
}

function formatarHora(iso: string): string {
  return new Date(iso).toLocaleTimeString('pt-BR');
}

/**
 * Detalhe de uma corrida concluída ou não concluída (#557): mapa descoberto, trajeto, gráfico de
 * velocidade × tempo, cartões e logs do robô, lidos do Supabase sem o robô ligado.
 *
 * Velocidade média: vale a gravada na corrida; se ela veio zerada (o robô ainda manda `velocidade = 0`),
 * é calculada pela distância (células trocadas × 0,18 m ÷ tempo). Sem dado nenhum, aparece "—".
 */
export function DetalheCorrida({ corrida, labirintos, aoVoltar }: DetalheCorridaProps) {
  const { leituras, mapa, logs, carregando, erro, erroLogs, disponivel, recarregar } = useDetalheCorrida(corrida.id);
  const reproducao = useReproducaoCorrida(leituras);
  const reproduzindo = reproducao.estado !== 'aguardando';

  const labirinto = labirintos.find((l) => l.id === corrida.labirinto_id);
  // Sem o labirinto na lista, o tamanho sai das células e posições conhecidas.
  const linhas =
    labirinto?.linhas ?? Math.max(1, ...mapa.map((c) => c.linha + 1), ...leituras.map((l) => l.posicao_linha + 1));
  const colunas =
    labirinto?.colunas ?? Math.max(1, ...mapa.map((c) => c.coluna + 1), ...leituras.map((l) => l.posicao_coluna + 1));

  const visiveis = reproduzindo ? reproducao.leituras : leituras;
  const percurso: Posicao[] = visiveis.map((l) => ({ linha: l.posicao_linha, coluna: l.posicao_coluna }));
  const ultima = visiveis.at(-1);

  const tempo = corrida.tempo_final ?? leiturasDaCorrida(leituras).at(-1)?.tempo_decorrido ?? null;
  const metricas = calcularMetricas(leituras, tempo);
  const mediaGravada = corrida.velocidade_media != null && corrida.velocidade_media > 0;
  const velocidadeMedia = mediaGravada ? corrida.velocidade_media : metricas.velocidadeMedia;
  const velocidadePico =
    corrida.velocidade_pico != null && corrida.velocidade_pico > 0 ? corrida.velocidade_pico : metricas.velocidadePico;
  const bateria =
    corrida.consumo_bateria != null
      ? `${Math.round(100 - corrida.consumo_bateria)}%`
      : metricas.tensao !== null
        ? `${percentualBateria(metricas.tensao)}%`
        : TRACO;
  const serie = serieDeVelocidade(leituras);
  const semTelemetria = !carregando && !erro && leituras.length === 0;

  return (
    <section className="detalhe" aria-label="Detalhe da corrida">
      <div className="detalhe__topo">
        <button type="button" className="detalhe__voltar" onClick={aoVoltar}>
          ← Voltar ao histórico
        </button>
        <h2 className="detalhe__titulo">
          {labirinto ? nomeLabirintoHistorico(labirinto.tipo) : 'Labirinto'} ·{' '}
          {descreverTentativa(corrida.numero_tentativa, corrida.status)}
        </h2>
        <span className="detalhe__data">{formatarDataHora(corrida.data_hora_inicio)}</span>
        <BadgeDesafio status={corrida.status} />
      </div>

      {erro && (
        <div className="historico__erro" role="alert">
          <span>{erro}</span>
          <button type="button" onClick={() => void recarregar()} className="historico__erro-botao">
            Tentar novamente
          </button>
        </div>
      )}

      <div className="detalhe__corpo">
        <div className="detalhe__mapa cartao">
          {carregando ? (
            <p className="grafico-vazio">Carregando a corrida…</p>
          ) : semTelemetria ? (
            <p className="grafico-vazio">
              {disponivel
                ? 'Esta corrida não tem telemetria gravada: não há trajeto nem gráfico para mostrar.'
                : 'Dados de exemplo: configure o Supabase para ver a telemetria das corridas.'}
            </p>
          ) : (
            !erro && (
              <>
                <GradeLabirinto
                  linhas={linhas}
                  colunas={colunas}
                  mapa={mapa}
                  percurso={percurso}
                  robo={
                    ultima
                      ? { linha: ultima.posicao_linha, coluna: ultima.posicao_coluna, orientacao: ultima.orientacao }
                      : { linha: 0, coluna: 0, orientacao: 'N' }
                  }
                />
                {mapa.length === 0 && (
                  <p className="detalhe__nota">O robô não gravou o mapa desta corrida: aparece só o trajeto.</p>
                )}
                <button
                  type="button"
                  className="detalhe__reproduzir"
                  onClick={reproduzindo ? reproducao.reiniciar : reproducao.iniciar}
                >
                  {reproduzindo ? 'Mostrar o trajeto completo' : 'Reproduzir o trajeto'}
                </button>

                <div className="detalhe__grafico">
                  <h3 className="ao-vivo__titulo">
                    Velocidade (m/s) × tempo (s){serie.estimada ? ' · estimada pela distância' : ''}
                  </h3>
                  {serie.leituras.length === 0 ? (
                    <p className="grafico-vazio">O robô não chegou a largar nesta corrida.</p>
                  ) : (
                    <Suspense fallback={<p className="grafico-vazio">Carregando o gráfico…</p>}>
                      <GraficoVelocidade leituras={serie.leituras} />
                    </Suspense>
                  )}
                </div>
              </>
            )
          )}
        </div>

        <aside className="detalhe__lateral" aria-label="Resultado da corrida">
          <CartaoMetrica
            rotulo="Tempo final"
            valor={tempo != null ? formatarTempo(tempo) : TRACO}
            detalhe={`Limite: ${formatarMinutos(LIMITE_TEMPO_S)}`}
          />
          <CartaoMetrica
            rotulo="Velocidade média"
            valor={formatarVelocidadeOuTraco(velocidadeMedia)}
            detalhe={
              !mediaGravada && metricas.mediaPelaDistancia
                ? `Pela distância · Pico máx: ${formatarVelocidadeOuTraco(velocidadePico)}`
                : `Pico máx: ${formatarVelocidadeOuTraco(velocidadePico)}`
            }
          />
          <CartaoMetrica
            rotulo="Bateria restante"
            valor={bateria}
            detalhe={metricas.tensao !== null ? `${metricas.tensao.toFixed(1)}V na última leitura` : 'Sem leitura de tensão'}
          />

          <div className="detalhe__logs cartao">
            <h3 className="ao-vivo__titulo">Logs do robô</h3>
            {erroLogs ? (
              <p className="detalhe__nota detalhe__nota--erro" role="alert">
                {erroLogs}
              </p>
            ) : carregando ? (
              <p className="detalhe__nota">Carregando os logs…</p>
            ) : logs.length === 0 ? (
              <p className="detalhe__nota">
                Nenhum log gravado para esta corrida (o banco guarda os logs por 7 dias).
              </p>
            ) : (
              <ol className="detalhe__lista-logs" aria-label="Logs da corrida">
                {logs.map((log) => {
                  const { texto, nivel } = formatarLog(log.mensagem);
                  return (
                    <li key={log.id} className={`detalhe__log detalhe__log--${nivel}`}>
                      <time dateTime={log.criado_em}>{formatarHora(log.criado_em)}</time>
                      <span>{texto}</span>
                    </li>
                  );
                })}
              </ol>
            )}
          </div>
        </aside>
      </div>
    </section>
  );
}
