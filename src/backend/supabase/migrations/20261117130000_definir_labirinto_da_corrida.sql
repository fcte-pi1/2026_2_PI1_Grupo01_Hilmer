-- O robô descobre o tamanho do labirinto (4x4, 8x4 ou 12x4) sozinho, explorando, e o informa depois.
--
-- Com a largada em qualquer célula, o labirinto é surpresa: o robô não sabe o tamanho quando a corrida
-- é criada (a corrida nasce como 4x4, o padrão do firmware, para o painel já mostrar o robô na espera).
-- Quando ele se localiza, `definir_labirinto_corrida` troca o labirinto da corrida pelo que ele
-- descobriu e renumera a tentativa dentro do labirinto novo. Só a chave de serviço a executa, como as
-- outras funções de corrida.
--
-- Erros (mensagem, SQLSTATE): labirinto_inexistente e corrida_inexistente (P0002), corrida_ja_encerrada
-- e telemetria_fora_do_labirinto (P0001).

CREATE OR REPLACE FUNCTION public.definir_labirinto_corrida(p_id INT, p_tipo TEXT)
RETURNS public.corridas
LANGUAGE plpgsql
SET search_path TO 'public'
AS $$
DECLARE
    v_labirinto labirintos;
    v_corrida corridas;
BEGIN
    SELECT * INTO v_labirinto FROM labirintos WHERE tipo = p_tipo;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'labirinto_inexistente' USING ERRCODE = 'P0002';
    END IF;

    SELECT * INTO v_corrida FROM corridas WHERE id = p_id FOR UPDATE;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'corrida_inexistente' USING ERRCODE = 'P0002';
    END IF;
    IF v_corrida.status <> 'em_andamento' THEN
        RAISE EXCEPTION 'corrida_ja_encerrada' USING ERRCODE = 'P0001';
    END IF;

    -- Repetir o mesmo tamanho (reenvio depois de uma queda de rede) não muda nada.
    IF v_corrida.labirinto_id = v_labirinto.id THEN
        RETURN v_corrida;
    END IF;

    -- Uma posição ou uma célula do mapa que já saiu fora da malha nova não pode ficar para trás.
    IF EXISTS (
        SELECT 1 FROM telemetria
         WHERE corrida_id = p_id
           AND (posicao_linha >= v_labirinto.linhas OR posicao_coluna >= v_labirinto.colunas)
    ) OR EXISTS (
        SELECT 1 FROM mapa_celulas
         WHERE corrida_id = p_id
           AND (linha >= v_labirinto.linhas OR coluna >= v_labirinto.colunas)
    ) THEN
        RAISE EXCEPTION 'telemetria_fora_do_labirinto' USING ERRCODE = 'P0001';
    END IF;

    -- O mesmo bloqueio de `criar_corrida`, para duas corridas no mesmo labirinto não pegarem a mesma tentativa.
    PERFORM pg_advisory_xact_lock(hashtext('criar_corrida'), v_labirinto.id);

    UPDATE corridas
       SET labirinto_id = v_labirinto.id,
           numero_tentativa = (
               SELECT COALESCE(MAX(numero_tentativa), 0) + 1 FROM corridas WHERE labirinto_id = v_labirinto.id
           )
     WHERE id = p_id
    RETURNING * INTO v_corrida;

    RETURN v_corrida;
END;
$$;

REVOKE ALL ON FUNCTION public.definir_labirinto_corrida(INT, TEXT) FROM PUBLIC, anon, authenticated;
GRANT EXECUTE ON FUNCTION public.definir_labirinto_corrida(INT, TEXT) TO service_role;
