import type { MapaCelula, Orientacao, Posicao } from '../tipos';
import './GradeLabirinto.css';

/** Lado de uma célula e margem, em unidades do viewBox do SVG. */
const LADO = 48;
const MARGEM = 6;

/** Rotação do marcador do robô, com Norte para cima. */
const ANGULO: Record<Orientacao, number> = { N: 0, L: 90, S: 180, O: 270 };

interface GradeLabirintoProps {
  linhas: number;
  colunas: number;
  /** Paredes conhecidas até o momento (`null` = não explorada, não é desenhada). */
  mapa: MapaCelula[];
  /** Células percorridas, em ordem (traçado `Robot Path`). */
  percurso: Posicao[];
  /** Posição e orientação atuais do robô (marcador `Robot`). */
  robo: Posicao & { orientacao: Orientacao };
}

/**
 * Malha do labirinto (`Maze Grid` do protótipo, HU-06): fundo escuro, largada verde no
 * canto inferior esquerdo, objetivo vermelho no canto oposto, paredes conhecidas,
 * traçado laranja e marcador do robô. A linha 0 fica embaixo e a coluna 0 à esquerda.
 */
export function GradeLabirinto({ linhas, colunas, mapa, percurso, robo }: GradeLabirintoProps) {
  const largura = colunas * LADO + 2 * MARGEM;
  const altura = linhas * LADO + 2 * MARGEM;
  const x = (coluna: number) => MARGEM + coluna * LADO;
  const y = (linha: number) => MARGEM + (linhas - 1 - linha) * LADO;
  const centro = (p: Posicao) => `${x(p.coluna) + LADO / 2},${y(p.linha) + LADO / 2}`;

  const paredes = mapa.flatMap((c) => {
    const x0 = x(c.coluna);
    const y0 = y(c.linha);
    const segmentos: [number, number, number, number][] = [];
    if (c.parede_norte) segmentos.push([x0, y0, x0 + LADO, y0]);
    if (c.parede_sul) segmentos.push([x0, y0 + LADO, x0 + LADO, y0 + LADO]);
    if (c.parede_oeste) segmentos.push([x0, y0, x0, y0 + LADO]);
    if (c.parede_leste) segmentos.push([x0 + LADO, y0, x0 + LADO, y0 + LADO]);
    return segmentos;
  });

  const descricao =
    `Labirinto de ${linhas} linhas por ${colunas} colunas. ` +
    `Robô na linha ${robo.linha}, coluna ${robo.coluna}, virado para ${robo.orientacao}. ` +
    `${Math.max(0, percurso.length - 1)} passos percorridos.`;

  return (
    <figure className="grade">
      <svg
        className="grade__svg"
        viewBox={`0 0 ${largura} ${altura}`}
        role="img"
        aria-label={descricao}
      >
        <rect className="grade__fundo" x="0" y="0" width={largura} height={altura} rx="6" />

        {Array.from({ length: colunas + 1 }, (_, c) => (
          <line key={`v${c}`} className="grade__linha" x1={x(c)} y1={y(linhas - 1)} x2={x(c)} y2={y(-1)} />
        ))}
        {Array.from({ length: linhas + 1 }, (_, l) => (
          <line key={`h${l}`} className="grade__linha" x1={x(0)} y1={y(l - 1)} x2={x(colunas)} y2={y(l - 1)} />
        ))}

        <rect
          className="grade__largada"
          data-testid="partida"
          aria-label="Partida"
          x={x(0) + 3}
          y={y(0) + 3}
          width={LADO - 6}
          height={LADO - 6}
          rx="3"
        />
        <rect
          className="grade__objetivo"
          data-testid="chegada"
          aria-label="Chegada"
          x={x(colunas - 1) + 3}
          y={y(linhas - 1) + 3}
          width={LADO - 6}
          height={LADO - 6}
          rx="3"
        />

        <g data-testid="paredes">
          {paredes.map(([x1, y1, x2, y2], i) => (
            <line key={i} className="grade__parede" x1={x1} y1={y1} x2={x2} y2={y2} />
          ))}
        </g>
        <rect
          className="grade__parede grade__perimetro"
          x={x(0)}
          y={y(linhas - 1)}
          width={colunas * LADO}
          height={linhas * LADO}
        />

        {percurso.length > 1 && (
          <polyline
            className="grade__trajeto"
            data-testid="robot-path"
            aria-label="Robot Path"
            points={percurso.map(centro).join(' ')}
          />
        )}

        <g
          className="grade__robo"
          data-testid="robot"
          aria-label="Robot"
          role="img"
          transform={`translate(${centro(robo)}) rotate(${ANGULO[robo.orientacao]})`}
        >
          <circle r={LADO * 0.22} />
          <path d={`M 0 ${-LADO * 0.13} L ${LADO * 0.08} ${LADO * 0.03} L ${-LADO * 0.08} ${LADO * 0.03} Z`} />
        </g>
      </svg>

      <figcaption className="grade__legenda">
        <span><i className="grade__amostra grade__amostra--largada" /> Largada</span>
        <span><i className="grade__amostra grade__amostra--objetivo" /> Objetivo</span>
        <span><i className="grade__amostra grade__amostra--trajeto" /> Trajeto</span>
        <span><i className="grade__amostra grade__amostra--parede" /> Parede mapeada</span>
      </figcaption>
    </figure>
  );
}
