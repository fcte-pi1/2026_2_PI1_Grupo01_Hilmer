import { lazy, Suspense } from 'react';
import { CartaoMetrica } from '../componentes/CartaoMetrica';
import { GradeLabirinto } from '../componentes/GradeLabirinto';
import {
  LIMITE_TEMPO_S,
  MAXIMO_TENTATIVAS,
  TENSAO_CHEIA_V,
  formatarMinutos,
  formatarTempo,
  formatarVelocidade,
  notaDaTentativa,
  percentualBateria,
} from '../lib/formatacao';
import { mapaConhecido } from '../lib/labirinto';
import { calcularMetricas, fracaoDoLimite } from '../lib/painelAoVivo';
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
  const metricas = calcularMetricas(leituras);
  const tensao = metricas.tensao ?? TENSAO_CHEIA_V;
  const usoDoLimite = fracaoDoLimite(tempo);

  const detalheTempo =
    estado === 'concluido'
      ? `Concluído na Tentativa ${tentativa} · Nota ${notaDaTentativa(tentativa)}`
      : estado === 'nao_concluido'
        ? `Tentativa ${tentativa} de ${MAXIMO_TENTATIVAS} · Tempo esgotado`
        : `Tentativa ${tentativa} de ${MAXIMO_TENTATIVAS} · Limite: ${formatarMinutos(LIMITE_TEMPO_S)}`;

  return (
    <div className="ao-vivo">
      <section className="ao-vivo__mapa cartao" aria-label="Mapa do labirinto">
        <GradeLabirinto
          linhas={labirinto.linhas}
          colunas={labirinto.colunas}
          mapa={aoVivo ? mapa : mapaConhecido(mapa, percurso)}
          percurso={percurso}
          robo={robo}
        />

        <div className="ao-vivo__grafico">
          <h2 className="ao-vivo__titulo">Velocidade (m/s) × tempo (s)</h2>
          {leituras.length === 0 ? (
            <p className="grafico-vazio">O gráfico aparece quando a corrida começar.</p>
          ) : (
            <Suspense fallback={<p className="grafico-vazio">Carregando o gráfico…</p>}>
              <GraficoVelocidade leituras={leituras} />
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
          valor={formatarVelocidade(metricas.velocidadeMedia)}
          detalhe={`Pico máx: ${formatarVelocidade(metricas.velocidadePico)}`}
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
