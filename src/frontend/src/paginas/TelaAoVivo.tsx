import { lazy, Suspense, useMemo } from 'react';
import { CartaoMetrica } from '../componentes/CartaoMetrica';
import { GradeLabirinto } from '../componentes/GradeLabirinto';
import {
  LIMITE_TEMPO_S,
  MAXIMO_TENTATIVAS,
  TENSAO_CHEIA_V,
  formatarMinutos,
  formatarTempo,
  formatarVelocidadeOuTraco,
  notaDaTentativa,
  percentualBateria,
} from '../lib/formatacao';
import { mapaConhecido } from '../lib/labirinto';
import { calcularMetricas, fracaoDoLimite, leiturasDaCorrida, serieDeVelocidade } from '../lib/painelAoVivo';
import type { EstadoPainel, Labirinto, MapaCelula, Orientacao, Posicao, Telemetria } from '../tipos';
import './TelaAoVivo.css';

/**
 * O gráfico usa o Recharts, a maior dependência do projeto. Ele é importado sob demanda:
 * o navegador só baixa esse arquivo quando a corrida começa e o gráfico aparece.
 */
const GraficoVelocidade = lazy(() =>
  import('../componentes/GraficoVelocidade').then((modulo) => ({ default: modulo.GraficoVelocidade })),
);

interface TelaAoVivoProps {
  labirinto: Labirinto;
  /** Paredes conhecidas do labirinto (descobertas pelo robô em mapa_celulas ou simuladas, #239). */
  mapa: MapaCelula[];
  estado: EstadoPainel;
  /** Tempo da corrida, em segundos. */
  tempo: number;
  /** Leituras recebidas, em ordem de `numero_sequencia`. */
  leituras: Telemetria[];
  tentativa: number;
  /**
   * `true` quando a tela acompanha o robô pelo Supabase (#231): a corrida começa e termina pelo
   * robô, então os botões de iniciar e resetar dão lugar a avisos.
   */
  aoVivo?: boolean;
  /**
   * `false` enquanto o robô ainda não descobriu o tamanho do labirinto (largada em qualquer célula): sem
   * tamanho, não há grade para desenhar. Padrão `true`, que é o dos dados de exemplo.
   */
  labirintoDescoberto?: boolean;
  aoIniciar: () => void;
  aoResetar: () => void;
}

/**
 * Tela "Telemetria ao Vivo" (Frames 01, 02 e 03; HU-14): mapa com o trajeto à esquerda e,
 * à direita, os cards de bateria, velocidade e tempo e o botão de controle da corrida.
 */
export function TelaAoVivo({
  labirinto,
  mapa,
  estado,
  tempo,
  leituras,
  tentativa,
  aoVivo = false,
  labirintoDescoberto = true,
  aoIniciar,
  aoResetar,
}: TelaAoVivoProps) {
  const ultima = leituras.at(-1);

  // Mapa e trajeto
  const percurso: Posicao[] = leituras.map((l) => ({ linha: l.posicao_linha, coluna: l.posicao_coluna }));
  const robo: Posicao & { orientacao: Orientacao } = ultima
    ? { linha: ultima.posicao_linha, coluna: ultima.posicao_coluna, orientacao: ultima.orientacao }
    : { linha: 0, coluna: 0, orientacao: 'N' };

  // Cards: recalculados a cada leitura nova (sem leituras, valores nominais do Frame 01)
  const metricas = calcularMetricas(leituras, tempo);
  const tensao = metricas.tensao ?? TENSAO_CHEIA_V;
  const usoDoLimite = fracaoDoLimite(tempo);

  // Antes da largada o robô já manda leituras (parado na célula de partida), mas o gráfico e os
  // cards de velocidade e tempo só passam a contar depois dela (#242).
  const leiturasNaCorrida = leiturasDaCorrida(leituras);
  // Com o robô mandando velocidade 0, o gráfico mostra a média acumulada pela distância (#557).
  const serie = useMemo(() => serieDeVelocidade(leituras), [leituras]);

  const detalheTempo =
    estado === 'concluido'
      ? `Concluído na Tentativa ${tentativa} · Nota ${notaDaTentativa(tentativa)}`
      : estado === 'nao_concluido'
        ? `Tentativa ${tentativa} de ${MAXIMO_TENTATIVAS} · Tempo esgotado`
        : estado === 'aguardando' && leituras.length > 0
          ? `Pronto na largada · Tentativa ${tentativa} de ${MAXIMO_TENTATIVAS} · Limite: ${formatarMinutos(LIMITE_TEMPO_S)}`
          : `Tentativa ${tentativa} de ${MAXIMO_TENTATIVAS} · Limite: ${formatarMinutos(LIMITE_TEMPO_S)}`;

  return (
    <div className="ao-vivo">
      <section className="ao-vivo__mapa cartao" aria-label="Mapa do labirinto">
        {labirintoDescoberto ? (
          <GradeLabirinto
            linhas={labirinto.linhas}
            colunas={labirinto.colunas}
            mapa={aoVivo ? mapa : mapaConhecido(mapa, percurso)}
            percurso={percurso}
            robo={robo}
          />
        ) : (
          <p className="grafico-vazio">
            O robô está se localizando e descobrindo o tamanho do labirinto. O mapa aparece quando ele souber.
          </p>
        )}

        <div className="ao-vivo__grafico">
          <h2 className="ao-vivo__titulo">
            Velocidade (m/s) × tempo (s){serie.estimada ? ' · estimada pela distância' : ''}
          </h2>
          {leiturasNaCorrida.length === 0 ? (
            <p className="grafico-vazio">O gráfico aparece quando a corrida começar.</p>
          ) : (
            <Suspense fallback={<p className="grafico-vazio">Carregando o gráfico…</p>}>
              <GraficoVelocidade leituras={serie.leituras} />
            </Suspense>
          )}
        </div>
      </section>

      <aside className="ao-vivo__lateral" aria-label="Telemetria">
        <CartaoMetrica
          rotulo="Consumo de bateria"
          valor={`${percentualBateria(tensao)}%`}
          detalhe={`${tensao.toFixed(1)}V · ${metricas.correnteMa} mA`}
        />
        <CartaoMetrica
          rotulo="Velocidade média"
          valor={formatarVelocidadeOuTraco(metricas.velocidadeMedia)}
          detalhe={
            metricas.mediaPelaDistancia
              ? 'Pela distância (células × 0,18 m ÷ tempo)'
              : `Pico máx: ${formatarVelocidadeOuTraco(metricas.velocidadePico)}`
          }
        />
        <CartaoMetrica rotulo="Tempo de conclusão" valor={formatarTempo(tempo)} detalhe={detalheTempo} />
        <div
          className={`limite${usoDoLimite >= 1 ? ' limite--esgotado' : ''}`}
          role="progressbar"
          aria-label={`Tempo usado do limite de ${formatarMinutos(LIMITE_TEMPO_S)}`}
          aria-valuemin={0}
          aria-valuemax={LIMITE_TEMPO_S}
          aria-valuenow={Math.round(Math.min(tempo, LIMITE_TEMPO_S))}
        >
          <div className="limite__barra" style={{ width: `${usoDoLimite * 100}%` }} />
          <span className="limite__marcador">{formatarMinutos(LIMITE_TEMPO_S)}</span>
        </div>

        {estado === 'aguardando' &&
          (aoVivo ? (
            <button type="button" className="botao botao--contorno" disabled>
              AGUARDANDO LARGADA DO ROBÔ
            </button>
          ) : (
            <button type="button" className="botao botao--primario" onClick={aoIniciar}>
              INICIAR CORRIDA
            </button>
          ))}
        {estado === 'em_execucao' && (
          <button type="button" className="botao botao--contorno" disabled>
            CORRIDA EM ANDAMENTO
          </button>
        )}
        {(estado === 'concluido' || estado === 'nao_concluido') &&
          (aoVivo ? (
            <button type="button" className="botao botao--contorno" disabled>
              AGUARDANDO A PRÓXIMA TENTATIVA
            </button>
          ) : (
            <button type="button" className="botao botao--primario" onClick={aoResetar}>
              RESETAR / NOVA TENTATIVA
            </button>
          ))}
      </aside>
    </div>
  );
}
