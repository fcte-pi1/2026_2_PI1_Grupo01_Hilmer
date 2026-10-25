import { useMemo, useState } from 'react';
import { Cabecalho } from './componentes/Cabecalho';
import { gerarCorridaExemplo } from './dados/corridaExemplo';
import { LABIRINTOS_EXEMPLO } from './dados/labirintosExemplo';
import { type EstadoConexao, useLabirintos } from './hooks/useLabirintos';
import { useReproducaoCorrida } from './hooks/useReproducaoCorrida';
import { useTelemetriaAoVivo } from './hooks/useTelemetriaAoVivo';
import { MAXIMO_TENTATIVAS } from './lib/formatacao';
import { TelaAoVivo } from './paginas/TelaAoVivo';
import { TelaHistorico } from './paginas/TelaHistorico';
import type { Aba, TipoLabirinto } from './tipos';
import './App.css';

const TEXTO_CONEXAO: Record<EstadoConexao, string> = {
  sem_configuracao: 'Supabase não configurado (crie o .env.local a partir do .env.example)',
  verificando: 'Supabase: conectando…',
  conectado: 'Supabase conectado: labirintos lidos do banco',
  erro: 'Supabase: falha ao ler o banco (veja o console)',
};

/**
 * Raiz do dashboard. Guarda a aba, o labirinto e a tentativa atuais; a corrida de exemplo
 * fica aqui para continuar rodando quando o usuário troca para o histórico.
 */
export function App() {
  const [aba, setAba] = useState<Aba>('ao_vivo');
  const [tipo, setTipo] = useState<TipoLabirinto>('8x4');
  const [tentativa, setTentativa] = useState(1);
  const { labirintos, conexao } = useLabirintos();

  // A corrida de exemplo usa o labirinto de exemplo do mesmo tipo, para não reiniciar
  // quando a lista real chegar do banco.
  const corridaExemplo = useMemo(() => {
    const labirinto = LABIRINTOS_EXEMPLO.find((l) => l.tipo === tipo) ?? LABIRINTOS_EXEMPLO[0];
    return gerarCorridaExemplo(labirinto, tentativa);
  }, [tipo, tentativa]);
  const reproducao = useReproducaoCorrida(corridaExemplo.telemetria);

  // Com o Supabase configurado, a tela acompanha a corrida real pelo Realtime (#231);
  // sem ele, segue com a corrida de exemplo.
  const aoVivo = useTelemetriaAoVivo();
  const labirintoAoVivo =
    labirintos.find((l) => l.id === aoVivo.corrida?.labirinto_id) ??
    labirintos.find((l) => l.tipo === tipo) ??
    corridaExemplo.labirinto;
  // Com uma corrida real, o cabeçalho mostra o labirinto dela, e não a escolha do modo exemplo.
  const corridaReal = aoVivo.disponivel && aoVivo.corrida !== null;
  const painel = aoVivo.disponivel
    ? {
        labirinto: labirintoAoVivo,
        mapa: aoVivo.mapa,
        estado: aoVivo.estado,
        tempo: aoVivo.tempo,
        leituras: aoVivo.leituras,
        tentativa: aoVivo.corrida?.numero_tentativa ?? tentativa,
      }
    : {
        labirinto: corridaExemplo.labirinto,
        mapa: corridaExemplo.mapa,
        estado: reproducao.estado,
        tempo: reproducao.tempo,
        leituras: reproducao.leituras,
        tentativa,
      };

  const selecionarLabirinto = (novo: TipoLabirinto) => {
    setTipo(novo);
    setTentativa(1);
    reproducao.reiniciar();
  };

  const resetar = () => {
    setTentativa((atual) => (atual >= MAXIMO_TENTATIVAS ? 1 : atual + 1));
    reproducao.reiniciar();
  };

  return (
    <div className="app">
      <Cabecalho
        aba={aba}
        aoMudarAba={setAba}
        labirintos={labirintos}
        tipoSelecionado={corridaReal ? labirintoAoVivo.tipo : tipo}
        aoSelecionarLabirinto={selecionarLabirinto}
        estado={painel.estado}
        seletorTravado={corridaReal}
      />

      <main className="app__conteudo">
        {aba === 'ao_vivo' ? (
          <TelaAoVivo
            {...painel}
            aoVivo={aoVivo.disponivel}
            aoIniciar={reproducao.iniciar}
            aoResetar={resetar}
          />
        ) : (
          <TelaHistorico labirintos={labirintos} />
        )}
      </main>

      <footer className="app__rodape">
        <span>{aoVivo.disponivel ? 'Telemetria ao vivo (Supabase Realtime)' : 'Dados de exemplo'}</span>
        <span aria-hidden="true">·</span>
        <span className={`app__conexao app__conexao--${conexao}`}>{TEXTO_CONEXAO[conexao]}</span>
      </footer>
    </div>
  );
}
