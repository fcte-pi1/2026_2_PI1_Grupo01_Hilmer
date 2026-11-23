import { useCallback, useEffect, useRef, useState } from 'react';
import { supabase } from '../lib/supabase';
import type { LogRobo } from '../lib/robo';
import type { MapaCelula, Telemetria } from '../tipos';

export interface DetalheCorrida {
  /** Leituras da corrida, em ordem de `numero_sequencia`. */
  leituras: Telemetria[];
  /** Paredes descobertas pelo robô (`mapa_celulas`). */
  mapa: MapaCelula[];
  /** Logs do robô da corrida (`logs_robo`), em ordem cronológica. */
  logs: LogRobo[];
  carregando: boolean;
  /** Falha ao ler a telemetria ou o mapa: sem eles não há mapa, trajeto nem gráfico. */
  erro: string | null;
  /** Falha ao ler os logs: o resto do detalhe continua aparecendo. */
  erroLogs: string | null;
  /** `false` se o Supabase não está configurado (corridas de exemplo, sem telemetria). */
  disponivel: boolean;
  recarregar: () => Promise<void>;
}

/**
 * Dados de uma corrida já encerrada, para o detalhe do histórico (#557): telemetria, mapa e logs,
 * lidos do Supabase pela `corrida_id`, sem precisar do robô ligado. Não assina o Realtime: a corrida
 * já terminou.
 *
 * Os logs vêm numa consulta à parte e com erro próprio: o banco apaga logs com mais de 7 dias
 * (`registrar_logs_robo`) e uma falha neles não pode esconder o mapa e o gráfico.
 */
export function useDetalheCorrida(corridaId: number | null): DetalheCorrida {
  const [leituras, setLeituras] = useState<Telemetria[]>([]);
  const [mapa, setMapa] = useState<MapaCelula[]>([]);
  const [logs, setLogs] = useState<LogRobo[]>([]);
  const [carregando, setCarregando] = useState(Boolean(supabase) && corridaId !== null);
  const [erro, setErro] = useState<string | null>(null);
  const [erroLogs, setErroLogs] = useState<string | null>(null);
  // Só vale a resposta da corrida pedida por último (troca rápida de corrida no histórico).
  const ultimaConsulta = useRef(0);

  const recarregar = useCallback(async () => {
    const consulta = ++ultimaConsulta.current;
    setLeituras([]);
    setMapa([]);
    setLogs([]);
    setErro(null);
    setErroLogs(null);
    if (!supabase || corridaId === null) {
      setCarregando(false);
      return;
    }
    setCarregando(true);

    const [telemetria, celulas, registros] = await Promise.all([
      supabase.from('telemetria').select('*').eq('corrida_id', corridaId).order('numero_sequencia', { ascending: true }),
      supabase.from('mapa_celulas').select('*').eq('corrida_id', corridaId),
      supabase.from('logs_robo').select('*').eq('corrida_id', corridaId).order('id', { ascending: true }),
    ]);
    if (consulta !== ultimaConsulta.current) return;

    if (telemetria.error || celulas.error) {
      console.warn('Supabase: não foi possível ler a corrida.', telemetria.error ?? celulas.error);
      setErro('Não foi possível carregar a telemetria e o mapa desta corrida.');
    } else {
      setLeituras((telemetria.data as Telemetria[] | null) ?? []);
      setMapa((celulas.data as MapaCelula[] | null) ?? []);
    }
    if (registros.error) {
      console.warn('Supabase: não foi possível ler os logs da corrida.', registros.error);
      setErroLogs('Não foi possível carregar os logs do robô desta corrida.');
    } else {
      setLogs((registros.data as LogRobo[] | null) ?? []);
    }
    setCarregando(false);
  }, [corridaId]);

  useEffect(() => {
    void recarregar();
    return () => {
      ultimaConsulta.current++; // descarta a resposta que chegar depois de desmontar
    };
  }, [recarregar]);

  return { leituras, mapa, logs, carregando, erro, erroLogs, disponivel: Boolean(supabase), recarregar };
}
