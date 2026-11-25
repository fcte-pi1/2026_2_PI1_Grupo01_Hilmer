# Relato da troca da ESP32

Durante os testes do robô, os sensores deixaram de responder. O robô ainda andava quando controlado remotamente, mas não conseguia navegar sozinho: os sensores funcionavam, porém a ESP32 não reconhecia as leituras deles. A equipe testou cada sensor separadamente, e a falta de resposta persistiu.

Foi constatado que um pino da ESP32 queimou, o que impedia a placa de receber as leituras dos sensores. A equipe substituiu a placa ESP32, e com a placa nova os sensores voltaram a ser reconhecidos e o robô voltou a navegar de forma autônoma.

O que provocou a queima do pino não foi confirmado e permanece como hipótese.

## Decisão sobre o INA226

A equipe decidiu deixar o módulo de medição de corrente INA226 fora da montagem. A decisão foi motivada pela suspeita de que o módulo tenha contribuído para a queima do pino da ESP32 substituída. A relação entre o INA226 e a falha permanece sem confirmação.
