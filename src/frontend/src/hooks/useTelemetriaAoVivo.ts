import { useEffect, useRef, useState } from 'react';
import { supabase } from '../lib/supabase';
import {
  PAINEL_INICIAL,
  aplicarCorrida,
  aplicarMapa,
  aplicarMapaCelula,
  aplicarTelemetria,
  type PainelAoVivo,
} from '../lib/painelAoVivo';
import type { Corrida, MapaCelula, Telemetria } from '../tipos';

/** Prefixo do nome dos canais Realtime da tela ao vivo. */
export const CANAL_AO_VIVO = 'painel-ao-vivo';

/**
 * Cada montagem do hook usa canais com nome próprio. O supabase-js devolve o canal já
 * existente quando o nome se repete; no StrictMode (montar, desmontar, montar), a segunda
 * montagem pegaria o canal que a primeira está removendo e ficaria sem eventos.
 */
let montagens = 0;

export interface TelemetriaAoVivo extends PainelAoVivo {
  /** `false` se o Supabase não está configurado (a tela usa os dados de exemplo). */
  disponivel: boolean;
  /** Mensagem de erro da última consulta que falhou, se houver. */
  erro: string | null;
}

/**
 * Acompanha a corrida do robô em tempo real pelo Supabase (HU-14, #231; HU-06, #239).
 *
 * 1. Assina os canais Realtime antes de consultar, para não perder linhas que cheguem no meio.
 * 2. Busca a corrida `em_andamento` mais recente, as leituras dela e as células do mapa, se o robô já largou.
 * 3. A cada `INSERT` em `telemetria`, atualiza o painel com as funções puras de
 *    `painelAoVivo.ts`, que ignoram linhas repetidas e de corridas encerradas.
 * 4. A cada evento em `mapa_celulas`, atualiza as paredes das células mapeadas pelo robô (#239).
 * 5. Leitura ou célula de uma corrida que a tela ainda não conhece: busca essa corrida e seus dados.
 * 6. Eventos de `corridas` e `mapa_celulas` ficam em canais separados, garantindo isolamento
 *    caso alguma tabela ainda não esteja na publicação do Realtime.
 *
 * Sem Supabase configurado, devolve `disponivel: false` e o painel inicial.
 */
export function useTelemetriaAoVivo(): TelemetriaAoVivo {
  const [painel, setPainel] = useState<PainelAoVivo>(PAINEL_INICIAL);
  const [erro, setErro] = useState<string | null>(null);
  const acompanhada = useRef<number | null>(null);

  useEffect(() => {
    if (!supabase) return;
    const cliente = supabase;
    let ativo = true;
    const buscadas = new Set<number>();

    const aplicar = (corrida: Corrida) => {
      if (corrida.status === 'em_andamento') acompanhada.current = corrida.id;
      setPainel((atual) => aplicarCorrida(atual, corrida));
    };

    /** Carrega a corrida, as leituras e as células do mapa que ela já tem (chegada atrasada ou largada nova). */
    const carregar = async (corrida: Corrida) => {
      aplicar(corrida);
      if (corrida.status !== 'em_andamento') return;
      const [leituras, celulas] = await Promise.all([
        cliente
          .from('telemetria')
          .select('*')
          .eq('corrida_id', corrida.id)
          .order('numero_sequencia', { ascending: true }),
        cliente
          .from('mapa_celulas')
          .select('*')
          .eq('corrida_id', corrida.id),
      ]);
      if (!ativo) return;
      if (leituras.error) {
        setErro('Não foi possível ler a telemetria da corrida no Supabase.');
        return;
      }
      if (celulas.error) {
        setErro('Não foi possível ler o mapa da corrida no Supabase.');
        return;
      }
      setPainel((atual) => {
        const comLeituras = ((leituras.data as Telemetria[] | null) ?? []).reduce(aplicarTelemetria, atual);
        return aplicarMapa(comLeituras, (celulas.data as (MapaCelula & { corrida_id?: number })[] | null) ?? []);
      });
    };

    const buscarCorrida = async (id: number) => {
      if (buscadas.has(id)) return;
      buscadas.add(id);
      const resposta = await cliente.from('corridas').select('*').eq('id', id).limit(1);
      if (!ativo) return;
      if (resposta.error) {
        buscadas.delete(id);
        setErro('Não foi possível ler a corrida em andamento no Supabase.');
        return;
      }
      const corrida = (resposta.data as Corrida[] | null)?.[0];
      if (corrida) await carregar(corrida);
    };

    const montagem = ++montagens;
    const canalTelemetria = cliente
      .channel(`${CANAL_AO_VIVO}-telemetria-${montagem}`)
      .on('postgres_changes', { event: 'INSERT', schema: 'public', table: 'telemetria' }, (payload) => {
        if (!ativo) return;
        const leitura = payload.new as Telemetria;
        if (leitura.corrida_id === acompanhada.current) {
          setPainel((atual) => aplicarTelemetria(atual, leitura));
        } else {
          void buscarCorrida(leitura.corrida_id);
        }
      })
      .subscribe();

    const canalCorridas = cliente
      .channel(`${CANAL_AO_VIVO}-corridas-${montagem}`)
      .on('postgres_changes', { event: '*', schema: 'public', table: 'corridas' }, (payload) => {
        if (ativo && payload.new) aplicar(payload.new as Corrida);
      })
      .subscribe();

    const canalMapa = cliente
      .channel(`${CANAL_AO_VIVO}-mapa-${montagem}`)
      .on('postgres_changes', { event: '*', schema: 'public', table: 'mapa_celulas' }, (payload) => {
        if (!ativo) return;
        const celula = payload.new as MapaCelula & { corrida_id: number };
        if (celula.corrida_id === acompanhada.current) {
          setPainel((atual) => aplicarMapaCelula(atual, celula));
        } else if (acompanhada.current === null) {
          void buscarCorrida(celula.corrida_id);
        }
      })
      .subscribe();

    (async () => {
      const corridas = await cliente
        .from('corridas')
        .select('*')
        .eq('status', 'em_andamento')
        .order('data_hora_inicio', { ascending: false })
        .limit(1);
      if (!ativo) return;
      if (corridas.error) {
        setErro('Não foi possível ler a corrida em andamento no Supabase.');
        return;
      }
      const corrida = (corridas.data as Corrida[] | null)?.[0];
      if (corrida && !buscadas.has(corrida.id)) {
        buscadas.add(corrida.id);
        await carregar(corrida);
      }
    })();

    return () => {
      ativo = false;
      void cliente.removeChannel(canalTelemetria);
      void cliente.removeChannel(canalCorridas);
      void cliente.removeChannel(canalMapa);
    };
  }, []);

  return { ...painel, disponivel: Boolean(supabase), erro };
}
