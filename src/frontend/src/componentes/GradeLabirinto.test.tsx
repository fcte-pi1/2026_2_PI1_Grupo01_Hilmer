import { render, screen } from '@testing-library/react';
import { describe, expect, it } from 'vitest';
import { GradeLabirinto } from './GradeLabirinto';
import type { MapaCelula, Orientacao, Posicao } from '../tipos';

describe('GradeLabirinto (HU-06, Issue #239)', () => {
  const baseRobo: Posicao & { orientacao: Orientacao } = { linha: 0, coluna: 0, orientacao: 'N' };

  describe('Malha com as dimensões do labirinto da corrida', () => {
    it('renderiza o SVG com as dimensões e o viewBox correto para labirinto 4x4', () => {
      // 4 colunas * 48 + 2 * 6 = 204; 4 linhas * 48 + 2 * 6 = 204
      const { container } = render(
        <GradeLabirinto linhas={4} colunas={4} mapa={[]} percurso={[]} robo={baseRobo} />,
      );
      const svg = container.querySelector('svg');
      expect(svg).toBeInTheDocument();
      expect(svg).toHaveAttribute('viewBox', '0 0 204 204');
    });

    it('renderiza o SVG com as dimensões e o viewBox correto para labirinto 8x4', () => {
      // 8 colunas * 48 + 12 = 396; 4 linhas * 48 + 12 = 204
      const { container } = render(
        <GradeLabirinto linhas={4} colunas={8} mapa={[]} percurso={[]} robo={baseRobo} />,
      );
      const svg = container.querySelector('svg');
      expect(svg).toBeInTheDocument();
      expect(svg).toHaveAttribute('viewBox', '0 0 396 204');
    });

    it('renderiza o SVG com as dimensões e o viewBox correto para labirinto 12x4', () => {
      // 12 colunas * 48 + 12 = 588; 4 linhas * 48 + 12 = 204
      const { container } = render(
        <GradeLabirinto linhas={4} colunas={12} mapa={[]} percurso={[]} robo={baseRobo} />,
      );
      const svg = container.querySelector('svg');
      expect(svg).toBeInTheDocument();
      expect(svg).toHaveAttribute('viewBox', '0 0 588 204');
    });

    it('posiciona a partida em (0, 0) no canto inferior esquerdo e a chegada em (linhas-1, colunas-1) no canto oposto', () => {
      render(<GradeLabirinto linhas={4} colunas={4} mapa={[]} percurso={[]} robo={baseRobo} />);

      const partida = screen.getByTestId('partida');
      const chegada = screen.getByTestId('chegada');

      expect(partida).toHaveClass('grade__largada');
      expect(chegada).toHaveClass('grade__objetivo');

      // x(0) = 6, y(0) = 6 + (4 - 1 - 0) * 48 = 150 -> x = 9, y = 153 (com offset de 3)
      expect(partida).toHaveAttribute('x', '9');
      expect(partida).toHaveAttribute('y', '153');

      // x(3) = 6 + 3 * 48 = 150, y(3) = 6 + (4 - 1 - 3) * 48 = 6 -> x = 153, y = 9
      expect(chegada).toHaveAttribute('x', '153');
      expect(chegada).toHaveAttribute('y', '9');
    });
  });

  describe('Marcador Robot', () => {
    it('renderiza o marcador do robô na posição e orientação iniciais (0, 0, N)', () => {
      render(
        <GradeLabirinto
          linhas={4}
          colunas={4}
          mapa={[]}
          percurso={[{ linha: 0, coluna: 0 }]}
          robo={{ linha: 0, coluna: 0, orientacao: 'N' }}
        />,
      );

      const roboMarker = screen.getByTestId('robot');
      expect(roboMarker).toBeInTheDocument();
      expect(roboMarker).toHaveAttribute('aria-label', 'Robot');
      // centro(0, 0): x = 6 + 24 = 30, y = 6 + 3 * 48 + 24 = 174 -> translate(30,174) rotate(0)
      expect(roboMarker).toHaveAttribute('transform', 'translate(30,174) rotate(0)');
    });

    it('atualiza a translação e a rotação conforme o robô se desloca e gira', () => {
      const { rerender } = render(
        <GradeLabirinto
          linhas={4}
          colunas={4}
          mapa={[]}
          percurso={[{ linha: 0, coluna: 0 }]}
          robo={{ linha: 0, coluna: 0, orientacao: 'L' }}
        />,
      );

      let roboMarker = screen.getByTestId('robot');
      expect(roboMarker).toHaveAttribute('transform', 'translate(30,174) rotate(90)');

      // Move para linha 1, coluna 2 com orientação Sul
      // centro(coluna 2, linha 1): x = 6 + 2 * 48 + 24 = 126, y = 6 + 2 * 48 + 24 = 126
      rerender(
        <GradeLabirinto
          linhas={4}
          colunas={4}
          mapa={[]}
          percurso={[{ linha: 0, coluna: 0 }, { linha: 1, coluna: 2 }]}
          robo={{ linha: 1, coluna: 2, orientacao: 'S' }}
        />,
      );

      roboMarker = screen.getByTestId('robot');
      expect(roboMarker).toHaveAttribute('transform', 'translate(126,126) rotate(180)');

      // Gira para Oeste
      rerender(
        <GradeLabirinto
          linhas={4}
          colunas={4}
          mapa={[]}
          percurso={[{ linha: 0, coluna: 0 }, { linha: 1, coluna: 2 }]}
          robo={{ linha: 1, coluna: 2, orientacao: 'O' }}
        />,
      );

      roboMarker = screen.getByTestId('robot');
      expect(roboMarker).toHaveAttribute('transform', 'translate(126,126) rotate(270)');
    });
  });

  describe('Traçado Robot Path', () => {
    it('não desenha traçado quando o percurso tem apenas a largada (1 ponto)', () => {
      render(
        <GradeLabirinto
          linhas={4}
          colunas={4}
          mapa={[]}
          percurso={[{ linha: 0, coluna: 0 }]}
          robo={baseRobo}
        />,
      );

      expect(screen.queryByTestId('robot-path')).not.toBeInTheDocument();
    });

    it('desenha e atualiza o traçado em tempo real conforme as posições avançam', () => {
      const percurso: Posicao[] = [
        { linha: 0, coluna: 0 },
        { linha: 1, coluna: 0 },
        { linha: 1, coluna: 1 },
      ];

      const { rerender } = render(
        <GradeLabirinto
          linhas={4}
          colunas={4}
          mapa={[]}
          percurso={percurso.slice(0, 2)}
          robo={{ linha: 1, coluna: 0, orientacao: 'N' }}
        />,
      );

      let path = screen.getByTestId('robot-path');
      expect(path).toBeInTheDocument();
      expect(path).toHaveAttribute('aria-label', 'Robot Path');
      // centro(0,0)=30,174; centro(0,1)=30,126
      expect(path).toHaveAttribute('points', '30,174 30,126');

      // Avança mais uma célula
      rerender(
        <GradeLabirinto
          linhas={4}
          colunas={4}
          mapa={[]}
          percurso={percurso}
          robo={{ linha: 1, coluna: 1, orientacao: 'L' }}
        />,
      );

      path = screen.getByTestId('robot-path');
      expect(path).toHaveAttribute('points', '30,174 30,126 78,126');
    });

    it('mantém o traçado completo ao final da corrida', () => {
      const percursoCompleto: Posicao[] = [
        { linha: 0, coluna: 0 },
        { linha: 1, coluna: 0 },
        { linha: 2, coluna: 0 },
        { linha: 3, coluna: 0 },
        { linha: 3, coluna: 1 },
        { linha: 3, coluna: 2 },
        { linha: 3, coluna: 3 }, // objetivo
      ];

      render(
        <GradeLabirinto
          linhas={4}
          colunas={4}
          mapa={[]}
          percurso={percursoCompleto}
          robo={{ linha: 3, coluna: 3, orientacao: 'L' }}
        />,
      );

      const path = screen.getByTestId('robot-path');
      expect(path).toBeInTheDocument();
      const points = path.getAttribute('points')?.split(' ');
      expect(points).toHaveLength(7);
      expect(points?.[0]).toBe('30,174'); // início (0,0)
      expect(points?.[6]).toBe('174,30'); // chegada (3,3)
    });
  });

  describe('Paredes mapeadas', () => {
    it('desenha linhas para paredes com valor true e ignora paredes false ou null', () => {
      const mapa: MapaCelula[] = [
        {
          linha: 0,
          coluna: 0,
          parede_norte: true,
          parede_sul: true,
          parede_leste: false,
          parede_oeste: null,
        },
        {
          linha: 1,
          coluna: 1,
          parede_norte: false,
          parede_sul: null,
          parede_leste: true,
          parede_oeste: true,
        },
      ];

      const { container } = render(
        <GradeLabirinto linhas={4} colunas={4} mapa={mapa} percurso={[]} robo={baseRobo} />,
      );

      const grupoParedes = screen.getByTestId('paredes');
      const linhasDeParede = grupoParedes.querySelectorAll('line');
      // 4 paredes com true no total (2 na celula 0,0 e 2 na celula 1,1)
      expect(linhasDeParede).toHaveLength(4);

      // Perímetro externo também é desenhado
      const perimetro = container.querySelector('.grade__perimetro');
      expect(perimetro).toBeInTheDocument();
    });
  });
});
