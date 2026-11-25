# Relato da troca da ESP32

Durante os testes do robô, os sensores deixaram de responder. O robô ainda andava quando controlado remotamente, mas não conseguia navegar sozinho: os sensores funcionavam, porém a ESP32 não reconhecia as leituras deles. A equipe testou cada sensor separadamente, e a falta de resposta persistiu.

Foi constatado que um pino da ESP32 queimou, o que impedia a placa de receber as leituras dos sensores. A equipe substituiu a placa ESP32, e com a placa nova os sensores voltaram a ser reconhecidos e o robô voltou a navegar de forma autônoma.

O que provocou a queima do pino não foi confirmado e permanece como hipótese.
