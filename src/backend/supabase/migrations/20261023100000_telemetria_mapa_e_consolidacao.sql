-- Telemetria em lote, mapa final e consolidação da corrida (Issue #229 / HU-09).
--
-- A chave `anon` só lê (RLS), então toda escrita passa por Edge Function com a chave de serviço.
-- A regra de negócio fica aqui, em funções SQL que só a `service_role` executa:
--   registrar_telemetria  grava um lote de amostras, de forma idempotente (reenvio não duplica);
--   registrar_mapa        grava o mapa final em mapa_celulas (reenvio atualiza, não duplica);
--   metricas_da_telemetria  calcula tempo, velocidades, consumo e conclusão a partir da telemetria;
--   consolidar_corrida    recalcula as métricas de uma corrida (reenvio atrasado, depois de encerrar);
--   encerrar_corrida      passa a calcular o que o robô não informar (status e métricas opcionais).
--
-- Erros (mensagem, SQLSTATE): corrida_inexistente (P0002), amostras_invalidas, lote_grande_demais,
-- celulas_invalidas, mapa_grande_demais e status_invalido (todos 22023), corrida_ja_encerrada (P0001).

set check_function_bodies = off;

-- Métricas de uma corrida calculadas só com a telemetria gravada. Sem amostras, tudo vem NULL
-- (e `m_objetivo` falso). `m_consumo` é a porcentagem consumida da LiPo 2S, estimada de forma
-- linear entre 8,4 V (carga plena) e 6,6 V (corte), pela primeira e pela última tensão medida
-- (tensão maior que zero). NULL quando a tensão nunca foi medida: a Rev 2.0 do robô ainda não lê a
-- bateria, e a telemetria vai com a tensão zerada. Regra provisória: a curva real não é linear.
CREATE OR REPLACE FUNCTION public.metricas_da_telemetria(
    p_corrida_id integer,
    OUT m_tempo_final double precision,
    OUT m_velocidade_media double precision,
    OUT m_velocidade_pico double precision,
    OUT m_consumo double precision,
    OUT m_objetivo boolean)
 RETURNS record
 LANGUAGE plpgsql
 STABLE
 SET search_path TO 'public'
AS $function$
DECLARE
    v_primeira double precision;
    v_ultima double precision;
BEGIN
    SELECT MAX(t.tempo_decorrido), AVG(t.velocidade), MAX(t.velocidade),
           COALESCE(bool_or(t.evento = 'objetivo_alcancado'), false)
    INTO m_tempo_final, m_velocidade_media, m_velocidade_pico, m_objetivo
    FROM telemetria t
    WHERE t.corrida_id = p_corrida_id;

    SELECT t.tensao_bateria INTO v_primeira
    FROM telemetria t
    WHERE t.corrida_id = p_corrida_id AND t.tensao_bateria > 0
    ORDER BY t.numero_sequencia ASC
    LIMIT 1;

    SELECT t.tensao_bateria INTO v_ultima
    FROM telemetria t
    WHERE t.corrida_id = p_corrida_id AND t.tensao_bateria > 0
    ORDER BY t.numero_sequencia DESC
    LIMIT 1;

    IF v_primeira IS NULL THEN
        m_consumo := NULL;
    ELSE
        m_consumo := LEAST(100.0, GREATEST(0.0, (v_primeira - v_ultima) / (8.4 - 6.6) * 100.0));
    END IF;
END;
$function$;

-- Grava um lote de até 100 amostras de telemetria (jsonb: lista de objetos com os nomes das colunas
-- de `telemetria`). Idempotente: uma amostra com (corrida_id, numero_sequencia) que já existe é
-- ignorada. Atômica: se uma amostra do lote for inválida, nenhuma é gravada.
-- Devolve {"gravadas": n, "existentes": m, "reconsolidada": bool}. Uma corrida já encerrada aceita
-- amostras (o robô pode reenviá-las depois de a rede voltar) e tem as métricas refeitas.
CREATE OR REPLACE FUNCTION public.registrar_telemetria(p_corrida_id integer, p_amostras jsonb)
 RETURNS jsonb
 LANGUAGE plpgsql
 SET search_path TO 'public'
AS $function$
DECLARE
    v_status text;
    v_linhas integer;
    v_colunas integer;
    v_total integer;
    v_gravadas integer;
BEGIN
    SELECT c.status, l.linhas, l.colunas INTO v_status, v_linhas, v_colunas
    FROM corridas c
    JOIN labirintos l ON l.id = c.labirinto_id
    WHERE c.id = p_corrida_id;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'corrida_inexistente' USING ERRCODE = 'P0002';
    END IF;

    IF p_amostras IS NULL OR jsonb_typeof(p_amostras) <> 'array' OR jsonb_array_length(p_amostras) = 0 THEN
        RAISE EXCEPTION 'amostras_invalidas' USING ERRCODE = '22023';
    END IF;
    v_total := jsonb_array_length(p_amostras);
    IF v_total > 100 THEN
        RAISE EXCEPTION 'lote_grande_demais' USING ERRCODE = '22023';
    END IF;

    BEGIN
        IF EXISTS (
            SELECT 1
            FROM jsonb_array_elements(p_amostras) AS a
            WHERE jsonb_typeof(a) <> 'object'
               OR (a->>'numero_sequencia')::int IS NULL
               OR (a->>'numero_sequencia')::int < 1
               OR (a->>'posicao_linha')::int IS NULL
               OR (a->>'posicao_linha')::int NOT BETWEEN 0 AND v_linhas - 1
               OR (a->>'posicao_coluna')::int IS NULL
               OR (a->>'posicao_coluna')::int NOT BETWEEN 0 AND v_colunas - 1
               OR (a->>'orientacao') IS NULL
               OR COALESCE((a->>'tempo_decorrido')::double precision, 0) < 0
               OR COALESCE((a->>'tensao_bateria')::double precision, 0) < 0
        ) THEN
            RAISE EXCEPTION 'amostras_invalidas' USING ERRCODE = '22023';
        END IF;

        INSERT INTO telemetria (
            corrida_id, numero_sequencia, posicao_linha, posicao_coluna, orientacao,
            velocidade, tensao_bateria, corrente_bateria, tempo_decorrido, estado_corrida, evento)
        SELECT p_corrida_id,
               (a->>'numero_sequencia')::int,
               (a->>'posicao_linha')::int,
               (a->>'posicao_coluna')::int,
               a->>'orientacao',
               COALESCE((a->>'velocidade')::double precision, 0.0),
               COALESCE((a->>'tensao_bateria')::double precision, 0.0),
               COALESCE((a->>'corrente_bateria')::double precision, 0.0),
               COALESCE((a->>'tempo_decorrido')::double precision, 0.0),
               a->>'estado_corrida',
               a->>'evento'
        FROM jsonb_array_elements(p_amostras) AS a
        ON CONFLICT (corrida_id, numero_sequencia) DO NOTHING;
        GET DIAGNOSTICS v_gravadas = ROW_COUNT;
    EXCEPTION
        WHEN invalid_text_representation OR check_violation OR not_null_violation
             OR numeric_value_out_of_range OR string_data_right_truncation THEN
            RAISE EXCEPTION 'amostras_invalidas' USING ERRCODE = '22023';
    END;

    IF v_status <> 'em_andamento' AND v_gravadas > 0 THEN
        PERFORM consolidar_corrida(p_corrida_id);
    END IF;

    RETURN jsonb_build_object(
        'gravadas', v_gravadas,
        'existentes', v_total - v_gravadas,
        'reconsolidada', (v_status <> 'em_andamento' AND v_gravadas > 0));
END;
$function$;

-- Grava o mapa final (jsonb: lista de células com `linha`, `coluna` e `parede_norte`, `parede_sul`,
-- `parede_leste` e `parede_oeste`, cada uma true, false ou null para desconhecida, o formato do
-- `MapaJson` do firmware). Reenviar atualiza as células já gravadas, sem duplicar.
-- Devolve {"novas": n, "atualizadas": m}.
CREATE OR REPLACE FUNCTION public.registrar_mapa(p_corrida_id integer, p_celulas jsonb)
 RETURNS jsonb
 LANGUAGE plpgsql
 SET search_path TO 'public'
AS $function$
DECLARE
    v_linhas integer;
    v_colunas integer;
    v_total integer;
    v_novas integer;
    v_atualizadas integer;
BEGIN
    SELECT l.linhas, l.colunas INTO v_linhas, v_colunas
    FROM corridas c
    JOIN labirintos l ON l.id = c.labirinto_id
    WHERE c.id = p_corrida_id;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'corrida_inexistente' USING ERRCODE = 'P0002';
    END IF;

    IF p_celulas IS NULL OR jsonb_typeof(p_celulas) <> 'array' OR jsonb_array_length(p_celulas) = 0 THEN
        RAISE EXCEPTION 'celulas_invalidas' USING ERRCODE = '22023';
    END IF;
    v_total := jsonb_array_length(p_celulas);
    IF v_total > v_linhas * v_colunas THEN
        RAISE EXCEPTION 'mapa_grande_demais' USING ERRCODE = '22023';
    END IF;

    BEGIN
        IF EXISTS (
            SELECT 1
            FROM jsonb_array_elements(p_celulas) AS a
            WHERE jsonb_typeof(a) <> 'object'
               OR (a->>'linha')::int IS NULL
               OR (a->>'linha')::int NOT BETWEEN 0 AND v_linhas - 1
               OR (a->>'coluna')::int IS NULL
               OR (a->>'coluna')::int NOT BETWEEN 0 AND v_colunas - 1
               OR jsonb_typeof(a->'parede_norte') NOT IN ('boolean', 'null')
               OR jsonb_typeof(a->'parede_sul') NOT IN ('boolean', 'null')
               OR jsonb_typeof(a->'parede_leste') NOT IN ('boolean', 'null')
               OR jsonb_typeof(a->'parede_oeste') NOT IN ('boolean', 'null')
        ) THEN
            RAISE EXCEPTION 'celulas_invalidas' USING ERRCODE = '22023';
        END IF;

        WITH gravadas AS (
            INSERT INTO mapa_celulas (corrida_id, linha, coluna, parede_norte, parede_sul, parede_leste, parede_oeste)
            SELECT p_corrida_id,
                   (a->>'linha')::int,
                   (a->>'coluna')::int,
                   (a->>'parede_norte')::boolean,
                   (a->>'parede_sul')::boolean,
                   (a->>'parede_leste')::boolean,
                   (a->>'parede_oeste')::boolean
            FROM jsonb_array_elements(p_celulas) AS a
            ON CONFLICT (corrida_id, linha, coluna) DO UPDATE
               SET parede_norte = EXCLUDED.parede_norte,
                   parede_sul = EXCLUDED.parede_sul,
                   parede_leste = EXCLUDED.parede_leste,
                   parede_oeste = EXCLUDED.parede_oeste
            RETURNING (xmax = 0) AS inserida
        )
        SELECT COUNT(*) FILTER (WHERE inserida), COUNT(*) FILTER (WHERE NOT inserida)
        INTO v_novas, v_atualizadas
        FROM gravadas;
    EXCEPTION
        WHEN invalid_text_representation OR cardinality_violation OR not_null_violation THEN
            -- cardinality_violation: a mesma célula duas vezes no mesmo lote.
            RAISE EXCEPTION 'celulas_invalidas' USING ERRCODE = '22023';
    END;

    RETURN jsonb_build_object('novas', v_novas, 'atualizadas', v_atualizadas);
END;
$function$;

-- Refaz as métricas de uma corrida a partir da telemetria. Só mexe no que a telemetria informa: sem
-- amostras, a corrida fica como está. Em corrida em andamento, não muda o status. Em corrida
-- encerrada, só promove `nao_concluida` a `concluida` se chegou o evento `objetivo_alcancado`;
-- nunca rebaixa uma corrida concluída.
CREATE OR REPLACE FUNCTION public.consolidar_corrida(p_id integer)
 RETURNS public.corridas
 LANGUAGE plpgsql
 SET search_path TO 'public'
AS $function$
DECLARE
    v_corrida corridas;
    m record;
BEGIN
    SELECT * INTO v_corrida FROM corridas WHERE id = p_id FOR UPDATE;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'corrida_inexistente' USING ERRCODE = 'P0002';
    END IF;

    SELECT * INTO m FROM metricas_da_telemetria(p_id);

    UPDATE corridas
    SET tempo_final = COALESCE(m.m_tempo_final, tempo_final),
        velocidade_media = COALESCE(m.m_velocidade_media, velocidade_media),
        velocidade_pico = COALESCE(m.m_velocidade_pico, velocidade_pico),
        consumo_bateria = COALESCE(m.m_consumo, consumo_bateria),
        status = CASE
                    WHEN status = 'nao_concluida' AND m.m_objetivo THEN 'concluida'
                    ELSE status
                 END
    WHERE id = p_id
    RETURNING * INTO v_corrida;

    RETURN v_corrida;
END;
$function$;

-- Encerra a corrida. O status e as métricas passam a ser opcionais: o que o robô não informar sai da
-- telemetria (status `concluida` se houve o evento `objetivo_alcancado`, senão `nao_concluida`). O que
-- o robô informar prevalece. Sem telemetria, o tempo final é o tempo desde `data_hora_inicio`.
-- Encerrar de novo sem status devolve a corrida como está (idempotente).
CREATE OR REPLACE FUNCTION public.encerrar_corrida(p_id integer, p_status text DEFAULT NULL::text, p_tempo_final double precision DEFAULT NULL::double precision, p_velocidade_media double precision DEFAULT NULL::double precision, p_velocidade_pico double precision DEFAULT NULL::double precision, p_consumo_bateria double precision DEFAULT NULL::double precision)
 RETURNS public.corridas
 LANGUAGE plpgsql
 SET search_path TO 'public'
AS $function$
DECLARE
    v_corrida corridas;
    m record;
    v_status text;
BEGIN
    IF p_status IS NOT NULL AND p_status NOT IN ('concluida', 'nao_concluida') THEN
        RAISE EXCEPTION 'status_invalido' USING ERRCODE = '22023';
    END IF;

    SELECT * INTO v_corrida FROM corridas WHERE id = p_id FOR UPDATE;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'corrida_inexistente' USING ERRCODE = 'P0002';
    END IF;

    IF v_corrida.status <> 'em_andamento' THEN
        IF p_status IS NULL OR v_corrida.status = p_status THEN
            RETURN v_corrida;
        END IF;
        RAISE EXCEPTION 'corrida_ja_encerrada' USING ERRCODE = 'P0001';
    END IF;

    SELECT * INTO m FROM metricas_da_telemetria(p_id);
    v_status := COALESCE(p_status, CASE WHEN m.m_objetivo THEN 'concluida' ELSE 'nao_concluida' END);

    UPDATE corridas
    SET status = v_status,
        data_hora_fim = now(),
        tempo_final = COALESCE(p_tempo_final, m.m_tempo_final, EXTRACT(EPOCH FROM (now() - data_hora_inicio))),
        velocidade_media = COALESCE(p_velocidade_media, m.m_velocidade_media),
        velocidade_pico = COALESCE(p_velocidade_pico, m.m_velocidade_pico),
        consumo_bateria = COALESCE(p_consumo_bateria, m.m_consumo)
    WHERE id = p_id
    RETURNING * INTO v_corrida;

    RETURN v_corrida;
END;
$function$;

-- Só a chave de serviço executa estas funções (anon e authenticated não escrevem no banco).
REVOKE ALL ON FUNCTION public.metricas_da_telemetria(INT) FROM PUBLIC, anon, authenticated;
REVOKE ALL ON FUNCTION public.registrar_telemetria(INT, JSONB) FROM PUBLIC, anon, authenticated;
REVOKE ALL ON FUNCTION public.registrar_mapa(INT, JSONB) FROM PUBLIC, anon, authenticated;
REVOKE ALL ON FUNCTION public.consolidar_corrida(INT) FROM PUBLIC, anon, authenticated;
GRANT EXECUTE ON FUNCTION public.metricas_da_telemetria(INT) TO service_role;
GRANT EXECUTE ON FUNCTION public.registrar_telemetria(INT, JSONB) TO service_role;
GRANT EXECUTE ON FUNCTION public.registrar_mapa(INT, JSONB) TO service_role;
GRANT EXECUTE ON FUNCTION public.consolidar_corrida(INT) TO service_role;
