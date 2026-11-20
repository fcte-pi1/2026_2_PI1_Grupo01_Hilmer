import { TIPOS_LABIRINTO, nomeLabirinto } from '../lib/formatacao';
import type { Aba, EstadoPainel, Labirinto, TipoLabirinto } from '../tipos';
import { BadgeDesafio } from './BadgeDesafio';
import './Cabecalho.css';

interface CabecalhoProps {
  aba: Aba;
  aoMudarAba: (aba: Aba) => void;
  labirintos: Labirinto[];
  tipoSelecionado: TipoLabirinto;
  aoSelecionarLabirinto: (tipo: TipoLabirinto) => void;
  estado: EstadoPainel;
  /**
   * `true` quando o painel acompanha o robô pelo Supabase (#231). O labirinto real é surpresa: quem o
   * define é o robô, que o descobre explorando e o informa ao banco. Nesse modo não há seletor, só o
   * texto do que o robô descobriu (ou de que ainda está descobrindo).
   */
  modoAoVivo?: boolean;
  /** Só vale no modo ao vivo: `true` depois de o robô informar o tamanho do labirinto. */
  labirintoDescoberto?: boolean;
}

const ABAS: { id: Aba; rotulo: string }[] = [
  { id: 'ao_vivo', rotulo: 'Telemetria ao Vivo' },
  { id: 'historico', rotulo: 'Histórico de Consultas' },
];

/**
 * Cabeçalho global: marca, abas, labirinto e badge do desafio.
 * O labirinto e o badge só aparecem na aba ao vivo (no Frame 04 eles não existem). Com dados de
 * exemplo, há um seletor (travado durante a corrida); com o robô de verdade, só o texto do labirinto
 * que o robô descobriu, porque o labirinto real é surpresa e não se escolhe.
 */
export function Cabecalho({
  aba,
  aoMudarAba,
  labirintos,
  tipoSelecionado,
  aoSelecionarLabirinto,
  estado,
  modoAoVivo = false,
  labirintoDescoberto = false,
}: CabecalhoProps) {
  const ordenados = [...labirintos].sort(
    (a, b) => TIPOS_LABIRINTO.indexOf(a.tipo) - TIPOS_LABIRINTO.indexOf(b.tipo),
  );

  return (
    <header className="cabecalho cartao">
      <span className="cabecalho__marca">MICROMOUSE V1</span>

      <nav className="cabecalho__abas" role="tablist" aria-label="Telas do dashboard">
        {ABAS.map(({ id, rotulo }) => (
          <button
            key={id}
            type="button"
            role="tab"
            aria-selected={aba === id}
            className={`cabecalho__aba${aba === id ? ' cabecalho__aba--ativa' : ''}`}
            onClick={() => aoMudarAba(id)}
          >
            {rotulo}
          </button>
        ))}
      </nav>

      {aba === 'ao_vivo' && (
        <>
          <span className="cabecalho__separador" aria-hidden="true" />
          {modoAoVivo ? (
            <span className="cabecalho__labirinto" role="status" aria-label="Labirinto">
              {labirintoDescoberto
                ? `${nomeLabirinto(tipoSelecionado)} · descoberto pelo robô`
                : 'O robô ainda está descobrindo o labirinto'}
            </span>
          ) : (
            <label className="cabecalho__seletor">
              <span className="visualmente-oculto">Labirinto</span>
              <select
                value={tipoSelecionado}
                onChange={(evento) => aoSelecionarLabirinto(evento.target.value as TipoLabirinto)}
                disabled={estado === 'em_execucao'}
                title={estado === 'em_execucao' ? 'Aguarde o fim da corrida para trocar o labirinto' : undefined}
              >
                {ordenados.map((labirinto) => (
                  <option key={labirinto.tipo} value={labirinto.tipo}>
                    {nomeLabirinto(labirinto.tipo)}
                  </option>
                ))}
              </select>
            </label>
          )}
          <BadgeDesafio estado={estado} className="cabecalho__badge" />
        </>
      )}
    </header>
  );
}
