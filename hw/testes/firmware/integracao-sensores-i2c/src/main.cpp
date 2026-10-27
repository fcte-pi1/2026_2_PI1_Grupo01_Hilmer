// Envia a orientacao do MPU6050 (calculada pelo DMP), as leituras brutas, as distancias dos 3 VL53L0X,
// a potencia dos motores e o encoder do motor esquerdo. Recebe comandos dos motores pelo serial.
// Uma linha por amostra: w,x,y,z,ax,ay,az,gx,gy,gz,frente,esquerda,direita,motorEsq,motorDir,encEsq,velEsq
//   (quaternion, g, graus/s, mm, -255..255, pulsos, pulsos/s)
// distancia = -1 sem parede ao alcance, -2 sensor nao respondeu
// Comando: "M,esq,dir" com valores de -255 a 255 (positivo = frente). Sem comando por 500 ms, para.
// Para ver em 3D e dirigir: rode visualizador/servidor.py e abra http://localhost:8000
//
// Todos no mesmo I2C: SDA no GPIO6, SCL no GPIO7, 3.3V. Pinagem em lib/robo/pinos.h e PINAGEM.md.
// Testes de cada sensor na placa: pio test
#include <Arduino.h>

#include <VL53L0X.h>
#include "i2c.h"
#include "pinos.h"

#include <MPU6050_6Axis_MotionApps20.h>

static MPU6050 mpu;
static uint8_t fifo[64];
static float acelLsb, giroLsb;  // leitura bruta por g e por grau/s
bool mpuOk = false;

VL53L0X tof[3];
bool tofOk[3];
int dist[3] = {-2, -2, -2};
static unsigned long ultimaConsulta = 0;

#define MPU_ENDERECO 0x68

struct LeituraImu {
  float w = 1, x = 0, y = 0, z = 0;  // quaternion
  float ax = 0, ay = 0, az = 0;      // g
  float gx = 0, gy = 0, gz = 0;      // graus/s
};

// 0 frente, 1 esquerda, 2 direita
#define TOF_FRENTE 0x30
#define TOF_ESQ 0x31
#define TOF_DIR 0x29

bool iniciarImu() {
  mpu.initialize();
  mpuOk = mpu.dmpInitialize() == 0;
  if (!mpuOk) return false;

  // so o giro: o CalibrateAccel da biblioteca 1.4.5 erra o alvo e satura o Z em 2 g
  Serial.println("calibrando: deixe o sensor parado...");
  mpu.CalibrateGyro(6);
  Serial.println();
  mpu.setDMPEnabled(true);

  acelLsb = 16384 >> mpu.getFullScaleAccelRange();
  giroLsb = 131.0 / (1 << mpu.getFullScaleGyroRange());
  return true;
}

bool lerImu(LeituraImu &l) {
  if (!mpu.dmpGetCurrentFIFOPacket(fifo)) return false;
  Quaternion q;
  mpu.dmpGetQuaternion(&q, fifo);
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  l.w = q.w; l.x = q.x; l.y = q.y; l.z = q.z;
  l.ax = ax / acelLsb; l.ay = ay / acelLsb; l.az = az / acelLsb;
  l.gx = gx / giroLsb; l.gy = gy / giroLsb; l.gz = gz / giroLsb;
  return true;
}

static bool iniciarTof(int i, uint8_t endereco) {
  tof[i] = VL53L0X();  // o XSHUT voltou o sensor para 0x29: esquece o endereco de uma chamada anterior
  tof[i].setTimeout(100);
  if (i == 0 && respondeEm(TOF_FRENTE)) tof[0].setAddress(TOF_FRENTE);
  tofOk[i] = tof[i].init();
  if (!tofOk[i]) return false;
  if (endereco != 0x29) tof[i].setAddress(endereco);
  tof[i].startContinuous();
  return true;
}

void iniciarTofs() {
  // esquerda e direita desligadas: so a frente responde
  pinMode(XSHUT_ESQ, OUTPUT);
  pinMode(XSHUT_DIR, OUTPUT);
  digitalWrite(XSHUT_ESQ, LOW);
  digitalWrite(XSHUT_DIR, LOW);
  delay(10);

  // a frente nao tem XSHUT: se a ESP reiniciou sem cortar a energia, ela continua em 0x30
  iniciarTof(0, TOF_FRENTE);

  pinMode(XSHUT_ESQ, INPUT_PULLUP);  // solta o XSHUT em vez de forcar 3.3V no pino do sensor
  delay(10);
  iniciarTof(1, TOF_ESQ);

  pinMode(XSHUT_DIR, INPUT_PULLUP);
  delay(10);
  iniciarTof(2, TOF_DIR);
}

void lerDistancias() {
  // a cada 5 ms basta: cada sensor gera uma medida a cada ~33 ms
  if (millis() - ultimaConsulta < 5) return;
  ultimaConsulta = millis();
  for (int i = 0; i < 3; i++) {
    if (!tofOk[i]) continue;
    // so le quando ha medida nova, para nao travar o loop esperando o sensor
    if ((tof[i].readReg(VL53L0X::RESULT_INTERRUPT_STATUS) & 0x07) == 0) continue;
    int mm = tof[i].readRangeContinuousMillimeters();
    dist[i] = mm > 2000 ? -1 : mm;
  }
}

void lerComandos() {
  // while (Serial.available()) {
  //   char c = Serial.read();
  //   if (c != '\n') {
  //     if (tamComando < (int)sizeof(comando) - 1) comando[tamComando++] = c;
  //     continue;
  //   }
  //   comando[tamComando] = 0;
  //   tamComando = 0;
  //   int esq, dir;
  //   if (sscanf(comando, "M,%d,%d", &esq, &dir) == 2) {
  //     mover(esq, dir);
  //     ultimoComando = millis();
  //   }
  // }
  // // pagina fechada ou servidor parado: o robo nao sai andando sozinho
  // if ((motorEsq || motorDir) && millis() - ultimoComando > 500) mover(0, 0);
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // tempo para o monitor serial conectar
  iniciarI2C();

  iniciarTofs();
  Serial.printf("VL53L0X frente %s, esquerda %s, direita %s\n",
                tofOk[0] ? "ok" : "FALHOU", tofOk[1] ? "ok" : "FALHOU", tofOk[2] ? "ok" : "FALHOU");

  if (!iniciarImu()){
    Serial.println("MPU6050 nao iniciou: confira os fios (seguindo sem ele)");
    while(1);
  }
}

unsigned long ultimoEnvio = 0;

void loop() {
  // sem o DMP para marcar o ritmo, envia a cada 20 ms
  if (millis() - ultimoEnvio < 20) return;
  ultimoEnvio = millis();

  lerComandos();
  lerDistancias();

  LeituraImu l;  // sem MPU: orientacao parada e leituras zeradas
  lerImu(l);

  lerDistancias();

  // tudo numa linha: o visualizador descarta linhas com menos de 10 colunas
  Serial.printf("%.4f,%.4f,%.4f,%.4f,%.2f,%.2f,%.2f,%.1f,%.1f,%.1f,%d,%d,%d\n",
                l.w, l.x, l.y, l.z, l.ax, l.ay, l.az, l.gx, l.gy, l.gz,
                dist[0], dist[1], dist[2]);
}
