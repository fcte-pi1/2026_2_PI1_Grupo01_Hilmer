-- Migração remota sincronizada via `supabase db pull`
-- Espelha as stored procedures de gerenciamento do ciclo de vida das corridas
-- (criar_corrida e encerrar_corrida), introduzidas no escopo da Issue #233.

set check_function_bodies = off;

CREATE OR REPLACE FUNCTION public.criar_corrida(p_tipo text)
 RETURNS public.corridas
 LANGUAGE plpgsql
 SET search_path TO 'public'
AS $function$
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
$function$;

CREATE OR REPLACE FUNCTION public.encerrar_corrida(p_id integer, p_status text, p_tempo_final double precision DEFAULT NULL::double precision, p_velocidade_media double precision DEFAULT NULL::double precision, p_velocidade_pico double precision DEFAULT NULL::double precision, p_consumo_bateria double precision DEFAULT NULL::double precision)
 RETURNS public.corridas
 LANGUAGE plpgsql
 SET search_path TO 'public'
AS $function$
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
$function$;
