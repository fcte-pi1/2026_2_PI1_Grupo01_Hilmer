-- Funções de corrida: criação com tentativa numerada por labirinto e encerramento (Issue #233 / HU-15)
--
-- A escrita em `corridas` não passa mais pelo PostgREST com a chave `anon` (veja
-- 20261014180500_seguranca_rls.sql). Estas funções são chamadas pela Edge Function `corridas`
-- com a chave de serviço (`service_role`), e só por ela.

-- ------------------------------------------------------------------------------
-- 1. criar_corrida
-- Cria uma corrida em andamento para o labirinto do tipo informado ('4x4', '8x4' ou '12x4').
-- A tentativa é numerada por labirinto: a primeira corrida do 4x4 é a tentativa 1, a segunda
-- é a 2, e a primeira do 8x4 volta a ser a 1.
-- O advisory lock serializa as criações do mesmo labirinto, para que duas corridas simultâneas
-- não recebam o mesmo número; a restrição uq_corridas_labirinto_tentativa é a segunda barreira.
-- ------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION criar_corrida(p_tipo TEXT)
RETURNS corridas
LANGUAGE plpgsql
SET search_path = public
AS $$
DECLARE
    v_labirinto_id INT;
    v_corrida corridas;
BEGIN
    SELECT id INTO v_labirinto_id FROM labirintos WHERE tipo = p_tipo;
    IF v_labirinto_id IS NULL THEN
        RAISE EXCEPTION 'labirinto_inexistente' USING ERRCODE = 'P0002';
    END IF;

    PERFORM pg_advisory_xact_lock(hashtext('criar_corrida'), v_labirinto_id);

    INSERT INTO corridas (labirinto_id, numero_tentativa)
    SELECT v_labirinto_id, COALESCE(MAX(numero_tentativa), 0) + 1
    FROM corridas
    WHERE labirinto_id = v_labirinto_id
    RETURNING * INTO v_corrida;

    RETURN v_corrida;
END;
$$;

COMMENT ON FUNCTION criar_corrida(TEXT) IS 'Cria uma corrida em andamento com a tentativa numerada por labirinto (4x4, 8x4 ou 12x4)';

-- ------------------------------------------------------------------------------
-- 2. encerrar_corrida
-- Encerra uma corrida em andamento: define o status final ('concluida' ou 'nao_concluida'),
-- preenche data_hora_fim e as métricas informadas. Sem `p_tempo_final`, usa o tempo decorrido
-- desde data_hora_inicio.
-- Idempotente: encerrar de novo com o mesmo status devolve a corrida como está (o robô pode
-- reenviar o pedido depois de uma queda de rede). Com outro status, recusa.
-- ------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION encerrar_corrida(
    p_id INT,
    p_status TEXT,
    p_tempo_final FLOAT DEFAULT NULL,
    p_velocidade_media FLOAT DEFAULT NULL,
    p_velocidade_pico FLOAT DEFAULT NULL,
    p_consumo_bateria FLOAT DEFAULT NULL
)
RETURNS corridas
LANGUAGE plpgsql
SET search_path = public
AS $$
DECLARE
    v_corrida corridas;
BEGIN
    IF p_status NOT IN ('concluida', 'nao_concluida') THEN
        RAISE EXCEPTION 'status_invalido' USING ERRCODE = '22023';
    END IF;

    SELECT * INTO v_corrida FROM corridas WHERE id = p_id FOR UPDATE;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'corrida_inexistente' USING ERRCODE = 'P0002';
    END IF;

    IF v_corrida.status <> 'em_andamento' THEN
        IF v_corrida.status = p_status THEN
            RETURN v_corrida;
        END IF;
        RAISE EXCEPTION 'corrida_ja_encerrada' USING ERRCODE = 'P0001';
    END IF;

    UPDATE corridas
    SET status = p_status,
        data_hora_fim = now(),
        tempo_final = COALESCE(p_tempo_final, EXTRACT(EPOCH FROM (now() - data_hora_inicio))),
        velocidade_media = p_velocidade_media,
        velocidade_pico = p_velocidade_pico,
        consumo_bateria = p_consumo_bateria
    WHERE id = p_id
    RETURNING * INTO v_corrida;

    RETURN v_corrida;
END;
$$;

COMMENT ON FUNCTION encerrar_corrida(INT, TEXT, FLOAT, FLOAT, FLOAT, FLOAT) IS 'Encerra uma corrida em andamento como concluida ou nao_concluida, preenchendo data_hora_fim; idempotente para o mesmo status';

-- ------------------------------------------------------------------------------
-- 3. PERMISSÕES
-- O PostgREST expõe as funções do schema public a quem tiver EXECUTE, e o Supabase concede
-- EXECUTE a anon e authenticated por padrão. Aqui só a chave de serviço (Edge Function) executa.
-- ------------------------------------------------------------------------------
REVOKE ALL ON FUNCTION criar_corrida(TEXT) FROM PUBLIC, anon, authenticated;
REVOKE ALL ON FUNCTION encerrar_corrida(INT, TEXT, FLOAT, FLOAT, FLOAT, FLOAT) FROM PUBLIC, anon, authenticated;
GRANT EXECUTE ON FUNCTION criar_corrida(TEXT) TO service_role;
GRANT EXECUTE ON FUNCTION encerrar_corrida(INT, TEXT, FLOAT, FLOAT, FLOAT, FLOAT) TO service_role;
