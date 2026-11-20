/**
 * Canal painel ↔ robô: o comando que o painel manda (iniciar, parar, zerar) e os logs que o robô devolve.
 *
 * O painel só lê as tabelas `comandos_robo` e `logs_robo` (RLS, só `SELECT`). Quem escreve o comando é a
 * Edge Function `robo` (`POST /functions/v1/robo/comandos`), que exige o `x-painel-token`: a senha do
 * operador, definida no Supabase (`PAINEL_TOKEN`) e digitada no painel. Ela fica só na sessão do
 * navegador e nunca vai no código nem no `.env.local`.
 */

export type ComandoRobo = 'iniciar' | 'parar' | 'zerar';

/** Tabela `logs_robo`. */
export interface LogRobo {
  id: number;
  corrida_id: number | null;
  /** ISO 8601, como o supabase-js devolve `timestamptz`. */
  criado_em: string;
  mensagem: string;
}

/** Tabela `comandos_robo`. */
export interface ComandoRegistrado {
  id: number;
  comando: ComandoRobo;
  criado_em: string;
  /** Preenchido quando o robô buscou o comando (ou quando um `parar` o cancelou). */
  entregue_em: string | null;
}

/** Quanto tempo um comando vale no servidor, em ms (`proximo_comando_robo`): depois disso ele expira. */
export const VALIDADE_COMANDO_MS = 30_000;

/** Quantos logs o painel guarda na tela. */
export const MAXIMO_LOGS_NA_TELA = 200;

export type ResultadoComando = { ok: true; id: number } | { ok: false; mensagem: string };

export interface AmbienteRobo {
  /** URL base do projeto Supabase. */
  url: string | undefined;
  /** Chave pública `anon`, que o gateway do Supabase exige. */
  chaveAnon: string | undefined;
}

const AMBIENTE_PADRAO: AmbienteRobo = {
  url: import.meta.env.VITE_SUPABASE_URL,
  chaveAnon: import.meta.env.VITE_SUPABASE_ANON_KEY,
};

/**
 * Manda um comando ao robô pela Edge Function `robo`. O robô o busca em até alguns segundos.
 *
 * @param tokenPainel Senha do operador (`PAINEL_TOKEN`); vazia, não é enviada (o painel é aberto se o segredo não existir).
 * @param buscar `fetch`, trocável nos testes.
 */
export async function enviarComandoAoRobo(
  comando: ComandoRobo,
  tokenPainel: string,
  ambiente: AmbienteRobo = AMBIENTE_PADRAO,
  buscar: typeof fetch = fetch,
): Promise<ResultadoComando> {
  if (!ambiente.url || !ambiente.chaveAnon) {
    return { ok: false, mensagem: 'Supabase não configurado: preencha o .env.local.' };
  }
  const endereco = `${ambiente.url.replace(/\/+$/, '')}/functions/v1/robo/comandos`;
  let resposta: Response;
  try {
    resposta = await buscar(endereco, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        apikey: ambiente.chaveAnon,
        Authorization: `Bearer ${ambiente.chaveAnon}`,
        // A senha é opcional: só vai se o operador digitou uma (a função só a exige com o segredo configurado).
        ...(tokenPainel.trim() !== '' ? { 'x-painel-token': tokenPainel } : {}),
      },
      body: JSON.stringify({ comando }),
    });
  } catch {
    return { ok: false, mensagem: 'Sem conexão com o Supabase. Confira a internet e tente de novo.' };
  }
  if (resposta.status === 401) {
    return { ok: false, mensagem: 'O painel exige senha: digite a senha do painel (o segredo PAINEL_TOKEN está definido no Supabase).' };
  }
  if (resposta.status === 404) {
    return { ok: false, mensagem: 'A função "robo" não está publicada no Supabase (supabase functions deploy robo).' };
  }
  if (!resposta.ok) {
    return { ok: false, mensagem: `O Supabase recusou o comando (código ${resposta.status}).` };
  }
  const corpo: unknown = await resposta.json().catch(() => null);
  const id = typeof corpo === 'object' && corpo !== null ? (corpo as { id?: unknown }).id : undefined;
  return { ok: true, id: typeof id === 'number' ? id : 0 };
}

/** Insere um log na lista: ignora repetido, mantém a ordem por `id` e guarda só os mais recentes. */
export function aplicarLog(logs: LogRobo[], novo: LogRobo): LogRobo[] {
  if (logs.some((l) => l.id === novo.id)) return logs;
  const lista = [...logs, novo].sort((a, b) => a.id - b.id);
  return lista.length > MAXIMO_LOGS_NA_TELA ? lista.slice(lista.length - MAXIMO_LOGS_NA_TELA) : lista;
}

/** Guarda o comando de maior `id` (o mais recente); uma atualização do mesmo `id` o substitui. */
export function aplicarComando(atual: ComandoRegistrado | null, novo: ComandoRegistrado): ComandoRegistrado {
  return atual === null || novo.id >= atual.id ? novo : atual;
}

export type NivelLog = 'info' | 'aviso' | 'erro';

export interface LogFormatado {
  texto: string;
  nivel: NivelLog;
}

const TEXTOS: Record<string, { texto: string; nivel: NivelLog }> = {
  largada_pedida: { texto: 'Largada pedida', nivel: 'info' },
  largada_liberada: { texto: 'Largada liberada: o robô começou', nivel: 'info' },
  largada_expirada: { texto: 'A largada expirou: o robô não ficou pronto a tempo', nivel: 'aviso' },
  parada_remota: { texto: 'Parada remota: o robô freou e espera nova largada', nivel: 'aviso' },
  comando_do_painel: { texto: 'Comando do painel recebido', nivel: 'info' },
  encoders_zerados: { texto: 'Estimativa zerada: o robô voltou a aguardar a largada', nivel: 'info' },
  aguardando_largada: { texto: 'Calibrado: aguardando a largada', nivel: 'info' },
  giroscopio_calibrado: { texto: 'Giroscópio calibrado', nivel: 'info' },
  calibracao: { texto: 'Calibração do boot', nivel: 'info' },
  imu_iniciada: { texto: 'IMU iniciada', nivel: 'info' },
  falha_imu: { texto: 'Falha ao iniciar a IMU', nivel: 'erro' },
  falha_barramento_i2c: { texto: 'Falha ao iniciar o barramento I²C', nivel: 'erro' },
  navegacao: { texto: 'Navegação', nivel: 'info' },
  colisao: { texto: 'Colisão detectada', nivel: 'aviso' },
  motor_travado: { texto: 'Motor travado: a proteção freou o robô', nivel: 'erro' },
  motores_travados_i2c: { texto: 'Motores parados por falha no I²C', nivel: 'erro' },
  motores_liberados_i2c: { texto: 'Motores liberados: o I²C voltou', nivel: 'info' },
  i2c: { texto: 'I²C', nivel: 'aviso' },
  reinicio_i2c: { texto: 'Reinício de dispositivo do I²C', nivel: 'aviso' },
  bateria: { texto: 'Bateria', nivel: 'aviso' },
  corrida_criada: { texto: 'Corrida criada no banco', nivel: 'info' },
  corrida_encerrada: { texto: 'Corrida encerrada no banco', nivel: 'info' },
  falha_criar_corrida: { texto: 'Não foi possível criar a corrida no banco', nivel: 'aviso' },
  envio_pausado: { texto: 'Envio da telemetria pausado: credenciais recusadas', nivel: 'erro' },
  envio_desligado: { texto: 'Envio da telemetria desligado', nivel: 'aviso' },
  wifi_conectado: { texto: 'Wi-Fi conectado', nivel: 'info' },
  wifi_caiu: { texto: 'Wi-Fi caiu', nivel: 'aviso' },
  aviso: { texto: 'Aviso do robô', nivel: 'aviso' },
};

/** Campos que não entram no detalhe: o `status` já é o título da linha. */
function detalhes(objeto: Record<string, unknown>): string {
  return Object.entries(objeto)
    .filter(([chave, valor]) => chave !== 'status' && valor !== null && valor !== undefined)
    .map(([chave, valor]) => `${chave}=${String(valor)}`)
    .join(' · ');
}

/**
 * Transforma a linha do robô (`{"status":"colisao","tipo":"impacto"}`) num texto para o operador, com um
 * nível para a cor. Linha que não é um JSON de evento aparece como veio. Eventos de `situacao` ruim
 * (bateria fraca, dispositivo em falha) sobem de nível.
 */
export function formatarLog(mensagem: string): LogFormatado {
  let objeto: unknown;
  try {
    objeto = JSON.parse(mensagem);
  } catch {
    return { texto: mensagem, nivel: 'info' };
  }
  if (typeof objeto !== 'object' || objeto === null || typeof (objeto as { status?: unknown }).status !== 'string') {
    return { texto: mensagem, nivel: 'info' };
  }
  const evento = objeto as Record<string, unknown>;
  const status = evento.status as string;
  const conhecido = TEXTOS[status];
  let nivel: NivelLog = conhecido?.nivel ?? 'info';
  if (status === 'bateria' && evento.situacao === 'ok') nivel = 'info';
  if (status === 'bateria' && evento.situacao === 'fraca') nivel = 'erro';
  if (status === 'i2c' && evento.situacao === 'em_falha') nivel = 'erro';
  if (status === 'navegacao' && evento.estado === 'falha') nivel = 'erro';
  if (status === 'reinicio_i2c' && evento.sucesso === false) nivel = 'erro';
  const titulo = conhecido?.texto ?? status.replace(/_/g, ' ');
  const resto = detalhes(evento);
  return { texto: resto === '' ? titulo : `${titulo} (${resto})`, nivel };
}
