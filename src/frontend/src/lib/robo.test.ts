import { describe, expect, it, vi } from 'vitest';
import {
  MAXIMO_LOGS_NA_TELA,
  aplicarComando,
  aplicarLog,
  enviarComandoAoRobo,
  formatarLog,
  type ComandoRegistrado,
  type LogRobo,
} from './robo';

const AMBIENTE = { url: 'https://projeto.supabase.co/', chaveAnon: 'chave-anon' };

function resposta(status: number, corpo: unknown = {}): Response {
  return new Response(JSON.stringify(corpo), { status, headers: { 'Content-Type': 'application/json' } });
}

const log = (id: number, mensagem = 'x'): LogRobo => ({ id, corrida_id: null, criado_em: '2026-11-17T12:00:00Z', mensagem });
const comando = (id: number, entregue: string | null = null): ComandoRegistrado => ({
  id,
  comando: 'iniciar',
  criado_em: '2026-11-17T12:00:00Z',
  entregue_em: entregue,
});

describe('enviarComandoAoRobo', () => {
  it('manda o comando para a função robo, com as chaves e a senha do painel', async () => {
    const buscar = vi.fn().mockResolvedValue(resposta(202, { id: 12, comando: 'iniciar' }));
    const r = await enviarComandoAoRobo('iniciar', 'senha', AMBIENTE, buscar);
    expect(r).toEqual({ ok: true, id: 12 });
    const [endereco, opcoes] = buscar.mock.calls[0] as [string, RequestInit];
    expect(endereco).toBe('https://projeto.supabase.co/functions/v1/robo/comandos');
    expect(opcoes.method).toBe('POST');
    expect(opcoes.body).toBe('{"comando":"iniciar"}');
    const cabecalhos = opcoes.headers as Record<string, string>;
    expect(cabecalhos['x-painel-token']).toBe('senha');
    expect(cabecalhos.apikey).toBe('chave-anon');
    expect(cabecalhos.Authorization).toBe('Bearer chave-anon');
  });

  it('não chama a rede sem o Supabase configurado ou sem a senha', async () => {
    const buscar = vi.fn();
    expect(await enviarComandoAoRobo('parar', 'senha', { url: undefined, chaveAnon: undefined }, buscar)).toMatchObject({ ok: false });
    expect(await enviarComandoAoRobo('parar', '   ', AMBIENTE, buscar)).toMatchObject({
      ok: false,
      mensagem: 'Digite a senha do painel.',
    });
    expect(buscar).not.toHaveBeenCalled();
  });

  it('explica cada falha: senha errada, função não publicada, recusa e rede fora', async () => {
    const mensagem = async (buscar: typeof fetch) => {
      const r = await enviarComandoAoRobo('zerar', 'senha', AMBIENTE, buscar);
      return r.ok ? '' : r.mensagem;
    };
    expect(await mensagem(vi.fn().mockResolvedValue(resposta(401)))).toMatch(/senha do painel incorreta/i);
    expect(await mensagem(vi.fn().mockResolvedValue(resposta(404)))).toMatch(/não está publicada/);
    expect(await mensagem(vi.fn().mockResolvedValue(resposta(500)))).toMatch(/código 500/);
    expect(await mensagem(vi.fn().mockRejectedValue(new TypeError('Failed to fetch')))).toMatch(/Sem conexão/);
  });

  it('aceita uma resposta de sucesso sem corpo legível', async () => {
    const buscar = vi.fn().mockResolvedValue(new Response('', { status: 202 }));
    expect(await enviarComandoAoRobo('iniciar', 'senha', AMBIENTE, buscar)).toEqual({ ok: true, id: 0 });
  });
});

describe('aplicarLog e aplicarComando', () => {
  it('põe os logs em ordem de id e ignora o repetido', () => {
    const lista = [log(3), log(1)].reduce(aplicarLog, [] as LogRobo[]);
    expect(lista.map((l) => l.id)).toEqual([1, 3]);
    expect(aplicarLog(lista, log(3))).toBe(lista);
    expect(aplicarLog(lista, log(2)).map((l) => l.id)).toEqual([1, 2, 3]);
  });

  it('guarda só os logs mais recentes', () => {
    let lista: LogRobo[] = [];
    for (let id = 1; id <= MAXIMO_LOGS_NA_TELA + 5; id++) lista = aplicarLog(lista, log(id));
    expect(lista).toHaveLength(MAXIMO_LOGS_NA_TELA);
    expect(lista[0].id).toBe(6);
  });

  it('o comando mais novo substitui o antigo, e a atualização do mesmo id também', () => {
    expect(aplicarComando(null, comando(1)).id).toBe(1);
    expect(aplicarComando(comando(2), comando(1)).id).toBe(2);
    expect(aplicarComando(comando(2), comando(3)).id).toBe(3);
    expect(aplicarComando(comando(2), comando(2, '2026-11-17T12:00:05Z')).entregue_em).toBe('2026-11-17T12:00:05Z');
  });
});

describe('formatarLog', () => {
  it('traduz os eventos do robô e mostra os detalhes', () => {
    expect(formatarLog('{"status":"largada_liberada"}')).toEqual({
      texto: 'Largada liberada: o robô começou',
      nivel: 'info',
    });
    expect(formatarLog('{"status":"colisao","tipo":"impacto","celulas":3}')).toEqual({
      texto: 'Colisão detectada (tipo=impacto · celulas=3)',
      nivel: 'aviso',
    });
  });

  it('sobe o nível de bateria fraca, falha do I²C e falha de navegação, e desce na recuperação', () => {
    expect(formatarLog('{"status":"bateria","situacao":"fraca","mv":6400}').nivel).toBe('erro');
    expect(formatarLog('{"status":"bateria","situacao":"ok","mv":7800}').nivel).toBe('info');
    expect(formatarLog('{"status":"i2c","dispositivo":"mpu6050","situacao":"em_falha"}').nivel).toBe('erro');
    expect(formatarLog('{"status":"navegacao","estado":"falha","falha":"tempo_esgotado"}').nivel).toBe('erro');
    expect(formatarLog('{"status":"reinicio_i2c","dispositivo":"tof_esquerdo","sucesso":false}').nivel).toBe('erro');
  });

  it('um status desconhecido vira texto legível, e o que não é evento aparece como veio', () => {
    expect(formatarLog('{"status":"teste_bancada","n":1}')).toEqual({ texto: 'teste bancada (n=1)', nivel: 'info' });
    expect(formatarLog('micromouse: iniciando')).toEqual({ texto: 'micromouse: iniciando', nivel: 'info' });
    expect(formatarLog('{"seq":1}')).toEqual({ texto: '{"seq":1}', nivel: 'info' });
    expect(formatarLog('{"status":5}')).toEqual({ texto: '{"status":5}', nivel: 'info' });
  });
});
