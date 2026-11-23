import { useEffect, useState } from 'react';
import { supabase } from '../lib/supabase';
import {
  MAXIMO_LOGS_NA_TELA,
  aplicarComando,
  aplicarLog,
  type ComandoRegistrado,
  type LogRobo,
} from '../lib/robo';

/** Prefixo do nome dos canais Realtime do controle do robô. */
export const CANAL_DO_ROBO = 'painel-robo';

/** Cada montagem do hook usa canais com nome próprio (veja `useTelemetriaAoVivo`). */
let montagens = 0;

export interface CanalDoRobo {
  /** `false` se o Supabase não está configurado. */
  disponivel: boolean;
  /** Logs do robô, do mais antigo ao mais novo. */
  logs: LogRobo[];
  /** O último comando que o painel mandou, com o instante em que o robô o buscou. */
  ultimoComando: ComandoRegistrado | null;
  /** Mensagem de erro da última consulta que falhou, se houver. */
  erro: string | null;
}

/**
 * Acompanha o robô pelo Supabase: os logs de evento (`logs_robo`) e o último comando enviado
 * (`comandos_robo`), em tempo real.
 *
 * Assina os canais antes de consultar, para não perder linhas que cheguem no meio, e depois lê os logs
 * mais recentes. Cada `INSERT` em `logs_robo` entra na lista (sem repetir). `INSERT` e `UPDATE` em
 * `comandos_robo` atualizam o último comando: o `UPDATE` é o robô buscando o comando.
 */
export function useCanalDoRobo(): CanalDoRobo {
  const [logs, setLogs] = useState<LogRobo[]>([]);
  const [ultimoComando, setUltimoComando] = useState<ComandoRegistrado | null>(null);
  const [erro, setErro] = useState<string | null>(null);

  useEffect(() => {
    if (!supabase) return;
    const cliente = supabase;
    let ativo = true;

    const montagem = ++montagens;
    const canalLogs = cliente
      .channel(`${CANAL_DO_ROBO}-logs-${montagem}`)
      .on('postgres_changes', { event: 'INSERT', schema: 'public', table: 'logs_robo' }, (payload) => {
        if (ativo) setLogs((atual) => aplicarLog(atual, payload.new as LogRobo));
      })
      .subscribe();
    const canalComandos = cliente
      .channel(`${CANAL_DO_ROBO}-comandos-${montagem}`)
      .on('postgres_changes', { event: '*', schema: 'public', table: 'comandos_robo' }, (payload) => {
        if (ativo) setUltimoComando((atual) => aplicarComando(atual, payload.new as ComandoRegistrado));
      })
      .subscribe();

    void (async () => {
      const [recentes, comandos] = await Promise.all([
        cliente.from('logs_robo').select('*').order('id', { ascending: false }).limit(MAXIMO_LOGS_NA_TELA),
        cliente.from('comandos_robo').select('*').order('id', { ascending: false }).limit(1),
      ]);
      if (!ativo) return;
      if (recentes.error) {
        setErro('Não foi possível ler os logs do robô no Supabase (a migração dos logs foi aplicada?).');
      } else {
        setLogs((atual) => ((recentes.data as LogRobo[] | null) ?? []).reduce(aplicarLog, atual));
      }
      if (comandos.error) {
        setErro('Não foi possível ler os comandos do robô no Supabase.');
      } else {
        const ultimo = (comandos.data as ComandoRegistrado[] | null)?.[0];
        if (ultimo) setUltimoComando((atual) => aplicarComando(atual, ultimo));
      }
    })();

    return () => {
      ativo = false;
      void cliente.removeChannel(canalLogs);
      void cliente.removeChannel(canalComandos);
    };
  }, []);

  return { disponivel: supabase !== null, logs, ultimoComando, erro };
}
