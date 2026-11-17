-- Comandos do painel para o robô (iniciar, parar, zerar) e logs do robô para o painel.
--
-- O robô só tem a chave geral (sem botão de largada): o operador dá a largada pelo painel. O painel
-- escreve o comando pela Edge Function `robo`, o robô o busca por HTTPS (`proximo_comando_robo`) e o
-- executa; os eventos do robô (largada, colisão, falha do I²C, bateria fraca) voltam como logs, que o
-- painel recebe pelo Realtime. Como nas outras tabelas, anon e authenticated só leem: toda escrita
-- passa pelas funções abaixo, executáveis só pela chave de serviço.

-- ------------------------------------------------------------------------------
-- 1. COMANDOS
-- ------------------------------------------------------------------------------
CREATE TABLE comandos_robo (
    id BIGSERIAL PRIMARY KEY,
    comando TEXT NOT NULL CHECK (comando IN ('iniciar', 'parar', 'zerar')),
    criado_em TIMESTAMPTZ NOT NULL DEFAULT now(),
    entregue_em TIMESTAMPTZ  -- preenchido quando o robô busca o comando (ou quando um `parar` o cancela)
);

COMMENT ON TABLE comandos_robo IS 'Comandos que o painel manda ao robô; cada um é entregue uma vez e vale por pouco tempo';
COMMENT ON COLUMN comandos_robo.entregue_em IS 'Quando o robô buscou o comando, ou quando um parar cancelou um comando pendente';

CREATE INDEX idx_comandos_robo_pendentes ON comandos_robo (id) WHERE entregue_em IS NULL;

-- ------------------------------------------------------------------------------
-- 2. LOGS
-- ------------------------------------------------------------------------------
CREATE TABLE logs_robo (
    id BIGSERIAL PRIMARY KEY,
    corrida_id INT REFERENCES corridas(id) ON DELETE SET NULL,
    criado_em TIMESTAMPTZ NOT NULL DEFAULT now(),
    mensagem TEXT NOT NULL CHECK (char_length(mensagem) BETWEEN 1 AND 300)
);

COMMENT ON TABLE logs_robo IS 'Linhas de log dos eventos do robô (as linhas {"status":...} da serial), para o painel';
COMMENT ON COLUMN logs_robo.corrida_id IS 'Corrida em andamento quando o robô enviou o log; nula antes de a corrida existir';

CREATE INDEX idx_logs_robo_criado_em ON logs_robo (criado_em);

-- ------------------------------------------------------------------------------
-- 3. LEITURA PELO PAINEL (RLS) E REALTIME
-- ------------------------------------------------------------------------------
ALTER TABLE comandos_robo ENABLE ROW LEVEL SECURITY;
ALTER TABLE logs_robo ENABLE ROW LEVEL SECURITY;

CREATE POLICY "Leitura pública dos comandos do robô"
    ON comandos_robo FOR SELECT
    TO anon, authenticated
    USING (true);

CREATE POLICY "Leitura pública dos logs do robô"
    ON logs_robo FOR SELECT
    TO anon, authenticated
    USING (true);

ALTER PUBLICATION supabase_realtime ADD TABLE comandos_robo;
ALTER PUBLICATION supabase_realtime ADD TABLE logs_robo;

-- ------------------------------------------------------------------------------
-- 4. FUNÇÕES (só a chave de serviço executa)
-- ------------------------------------------------------------------------------

-- Grava um comando do painel. Um `parar` cancela os comandos ainda pendentes (um `iniciar` que o robô
-- não buscou não pode largar o robô depois de o operador mandar parar) e vale como o próximo a ser entregue.
CREATE OR REPLACE FUNCTION public.enviar_comando_robo(p_comando TEXT)
RETURNS JSONB
LANGUAGE plpgsql
AS $$
DECLARE
    v_id BIGINT;
BEGIN
    IF p_comando IS NULL OR p_comando NOT IN ('iniciar', 'parar', 'zerar') THEN
        RAISE EXCEPTION 'comando_invalido' USING ERRCODE = 'P0001';
    END IF;
    IF p_comando = 'parar' THEN
        UPDATE comandos_robo SET entregue_em = now() WHERE entregue_em IS NULL;
    END IF;
    INSERT INTO comandos_robo (comando) VALUES (p_comando) RETURNING id INTO v_id;
    RETURN jsonb_build_object('id', v_id, 'comando', p_comando);
END;
$$;

-- Entrega ao robô o próximo comando pendente e o marca como entregue, de uma vez só (duas buscas ao
-- mesmo tempo não recebem o mesmo comando). Só vale um comando criado há até `p_validade_segundos`: um
-- `iniciar` esquecido no painel não larga o robô minutos depois. `parar` passa na frente dos outros.
-- Sem comando, devolve `{"id": null, "comando": null}`.
CREATE OR REPLACE FUNCTION public.proximo_comando_robo(p_validade_segundos INT DEFAULT 30)
RETURNS JSONB
LANGUAGE plpgsql
AS $$
DECLARE
    v_id BIGINT;
    v_comando TEXT;
BEGIN
    UPDATE comandos_robo
       SET entregue_em = now()
     WHERE id = (
        SELECT id FROM comandos_robo
         WHERE entregue_em IS NULL
           AND criado_em > now() - make_interval(secs => p_validade_segundos)
         ORDER BY (comando = 'parar') DESC, id
         LIMIT 1
         FOR UPDATE SKIP LOCKED
     )
    RETURNING id, comando INTO v_id, v_comando;
    RETURN jsonb_build_object('id', v_id, 'comando', v_comando);
END;
$$;

-- Grava as linhas de log do robô. Aceita até 50 linhas de até 300 caracteres e recusa o lote inteiro se
-- alguma for inválida. Uma corrida que não existe vira `NULL`. Apaga os logs com mais de 7 dias.
CREATE OR REPLACE FUNCTION public.registrar_logs_robo(p_corrida_id INT, p_linhas TEXT[])
RETURNS INT
LANGUAGE plpgsql
AS $$
DECLARE
    v_corrida INT := p_corrida_id;
    v_quantidade INT := COALESCE(array_length(p_linhas, 1), 0);
BEGIN
    IF v_quantidade = 0 THEN
        RAISE EXCEPTION 'logs_invalidos' USING ERRCODE = 'P0001';
    END IF;
    IF v_quantidade > 50 THEN
        RAISE EXCEPTION 'logs_grande_demais' USING ERRCODE = 'P0001';
    END IF;
    IF EXISTS (SELECT 1 FROM unnest(p_linhas) AS l WHERE l IS NULL OR char_length(l) NOT BETWEEN 1 AND 300) THEN
        RAISE EXCEPTION 'logs_invalidos' USING ERRCODE = 'P0001';
    END IF;
    IF v_corrida IS NOT NULL AND NOT EXISTS (SELECT 1 FROM corridas WHERE id = v_corrida) THEN
        v_corrida := NULL;
    END IF;
    INSERT INTO logs_robo (corrida_id, mensagem)
    SELECT v_corrida, l FROM unnest(p_linhas) WITH ORDINALITY AS t(l, ordem) ORDER BY ordem;
    DELETE FROM logs_robo WHERE criado_em < now() - interval '7 days';
    RETURN v_quantidade;
END;
$$;

REVOKE ALL ON FUNCTION public.enviar_comando_robo(TEXT) FROM PUBLIC, anon, authenticated;
REVOKE ALL ON FUNCTION public.proximo_comando_robo(INT) FROM PUBLIC, anon, authenticated;
REVOKE ALL ON FUNCTION public.registrar_logs_robo(INT, TEXT[]) FROM PUBLIC, anon, authenticated;
GRANT EXECUTE ON FUNCTION public.enviar_comando_robo(TEXT) TO service_role;
GRANT EXECUTE ON FUNCTION public.proximo_comando_robo(INT) TO service_role;
GRANT EXECUTE ON FUNCTION public.registrar_logs_robo(INT, TEXT[]) TO service_role;
