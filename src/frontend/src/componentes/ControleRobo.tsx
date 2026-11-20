import { useEffect, useRef, useState } from 'react';
import { useCanalDoRobo } from '../hooks/useCanalDoRobo';
import {
  VALIDADE_COMANDO_MS,
  enviarComandoAoRobo,
  formatarLog,
  type ComandoRobo,
  type ResultadoComando,
} from '../lib/robo';
import './ControleRobo.css';

/** Onde a senha do painel fica: só nesta aba do navegador, nunca em disco nem no código. */
export const CHAVE_SENHA_DO_PAINEL = 'micromouse_senha_do_painel';

const NOMES: Record<ComandoRobo, string> = { iniciar: 'Iniciar', parar: 'Parar', zerar: 'Zerar' };

function lerSenhaSalva(): string {
  try {
    return window.sessionStorage.getItem(CHAVE_SENHA_DO_PAINEL) ?? '';
  } catch {
    return '';
  }
}

function guardarSenha(senha: string) {
  try {
    window.sessionStorage.setItem(CHAVE_SENHA_DO_PAINEL, senha);
  } catch {
    // Sem sessionStorage (modo privado, por exemplo), a senha vale só até recarregar a página.
  }
}

function formatarHora(iso: string): string {
  return new Date(iso).toLocaleTimeString('pt-BR');
}

interface ControleRoboProps {
  /** Manda o comando ao robô; trocável nos testes. */
  aoEnviar?: (comando: ComandoRobo, senha: string) => Promise<ResultadoComando>;
}

/**
 * Controle do robô pelo painel: os botões de iniciar, parar e zerar e os logs do robô ao vivo.
 *
 * O robô não tem botão de largada: a chave geral liga a energia e ele espera a largada. O comando vai ao
 * Supabase (Edge Function `robo`, com a senha do painel), o robô o busca a cada poucos segundos e
 * devolve os logs dos eventos, que chegam aqui pelo Realtime.
 */
export function ControleRobo({ aoEnviar = enviarComandoAoRobo }: ControleRoboProps) {
  const { disponivel, logs, ultimoComando, erro } = useCanalDoRobo();
  const [senha, setSenha] = useState(lerSenhaSalva);
  const [enviando, setEnviando] = useState<ComandoRobo | null>(null);
  const [aviso, setAviso] = useState<{ texto: string; erro: boolean } | null>(null);
  const [agora, setAgora] = useState(() => Date.now());
  const fim = useRef<HTMLLIElement | null>(null);

  // O relógio só gira enquanto há um comando esperando o robô buscá-lo (para mostrar quando expira).
  const esperando = ultimoComando !== null && ultimoComando.entregue_em === null;
  useEffect(() => {
    if (!esperando) return;
    const temporizador = window.setInterval(() => setAgora(Date.now()), 1000);
    return () => window.clearInterval(temporizador);
  }, [esperando]);

  // A lista rola sozinha para a linha mais nova.
  useEffect(() => {
    fim.current?.scrollIntoView?.({ block: 'nearest' });
  }, [logs.length]);

  const enviar = async (comando: ComandoRobo) => {
    setEnviando(comando);
    setAviso(null);
    const resultado = await aoEnviar(comando, senha);
    setEnviando(null);
    setAgora(Date.now());
    if (resultado.ok) {
      guardarSenha(senha);
      setAviso({ texto: `${NOMES[comando]} enviado. O robô busca o comando em poucos segundos.`, erro: false });
    } else {
      setAviso({ texto: resultado.mensagem, erro: true });
    }
  };

  let situacaoDoComando: string | null = null;
  if (ultimoComando) {
    const criado = new Date(ultimoComando.criado_em).getTime();
    if (ultimoComando.entregue_em) {
      situacaoDoComando = `Último comando: ${NOMES[ultimoComando.comando]}, entregue ao robô às ${formatarHora(ultimoComando.entregue_em)}.`;
    } else if (agora - criado > VALIDADE_COMANDO_MS) {
      situacaoDoComando = `Último comando: ${NOMES[ultimoComando.comando]}, expirou sem o robô buscá-lo (ele está ligado e no Wi-Fi?).`;
    } else {
      situacaoDoComando = `Último comando: ${NOMES[ultimoComando.comando]}, aguardando o robô buscá-lo…`;
    }
  }

  return (
    <section className="controle cartao" aria-label="Controle do robô">
      <div className="controle__cabecalho">
        <h2 className="controle__titulo">Controle e logs do robô</h2>
        <label className="controle__senha">
          <span>Senha do painel (opcional)</span>
          <input
            type="password"
            value={senha}
            autoComplete="off"
            onChange={(e) => setSenha(e.target.value)}
            placeholder="deixe vazio se o painel for aberto"
          />
        </label>
      </div>

      {!disponivel ? (
        <p className="controle__vazio">Configure o Supabase (.env.local) para controlar o robô e ver os logs.</p>
      ) : (
        <>
          <div className="controle__botoes">
            <button
              type="button"
              className="botao botao--primario"
              disabled={enviando !== null}
              onClick={() => void enviar('iniciar')}
            >
              INICIAR
            </button>
            <button
              type="button"
              className="botao botao--perigo"
              disabled={enviando !== null}
              onClick={() => void enviar('parar')}
            >
              PARAR
            </button>
            <button
              type="button"
              className="botao botao--contorno"
              disabled={enviando !== null}
              onClick={() => void enviar('zerar')}
              title="Para o robô recolocado na partida: zera a estimativa e volta a aguardar a largada"
            >
              ZERAR
            </button>
          </div>

          <div aria-live="polite" className="controle__estado">
            {aviso && <p className={aviso.erro ? 'controle__aviso controle__aviso--erro' : 'controle__aviso'}>{aviso.texto}</p>}
            {situacaoDoComando && <p className="controle__comando">{situacaoDoComando}</p>}
            {erro && <p className="controle__aviso controle__aviso--erro">{erro}</p>}
          </div>

          <ol className="controle__logs" aria-label="Logs do robô">
            {logs.length === 0 ? (
              <li className="controle__vazio">Nenhum log do robô ainda. Ligue o robô e espere o Wi-Fi conectar.</li>
            ) : (
              logs.map((log) => {
                const { texto, nivel } = formatarLog(log.mensagem);
                return (
                  <li key={log.id} className={`controle__log controle__log--${nivel}`}>
                    <time dateTime={log.criado_em}>{formatarHora(log.criado_em)}</time>
                    <span>{texto}</span>
                  </li>
                );
              })
            )}
            <li ref={fim} aria-hidden="true" />
          </ol>
        </>
      )}
    </section>
  );
}
