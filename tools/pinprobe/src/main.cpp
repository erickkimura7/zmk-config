// Diagnostico de pinos do clone nice!nano v2 (Corne).
//
// Comandos pela serial (115200):
//   i  modo INPUT: 115 (P1.15) fica HIGH como fonte; todos os outros pinos do
//      header ficam INPUT_PULLDOWN. Encoste um fio do 115 em um pino: a serial
//      mostra qual pino REAL do chip ficou HIGH.
//   o  modo OUTPUT normal: 029, 111 (suspeitos) e 002, 113 (referencia) em HIGH.
//      Meca com multimetro (preta no GND): saida boa = ~3.3 V.
//   h  igual ao 'o', mas com high drive (H0H1, ~10 mA): se o suspeito sobe muito,
//      o driver normal esta fraco ou ha carga puxando para baixo.
//   z  tudo em alta impedancia (para medir resistencia com a placa ligada).
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

struct Pin { uint8_t num; const char *label; };
// numeracao raw: P0.xx = xx, P1.xx = 32+xx
static const Pin PINS[] = {
  {6, "006"}, {8, "008"}, {17, "017"}, {20, "020"}, {22, "022"}, {24, "024"},
  {32, "100"}, {11, "011"}, {36, "104"}, {38, "106"},
  {31, "031"}, {29, "029"}, {2, "002"}, {45, "113"}, {43, "111"}, {10, "010"}, {9, "009"},
};
static const uint8_t SRC = 47;  // P1.15 = "115"
static const int N = sizeof(PINS) / sizeof(PINS[0]);
static const uint8_t OUTS[] = {29, 43, 2, 45};  // 029, 111, 002, 113
static int last[N];
static bool scanning = false;

static void allHighZ() {
  pinMode(SRC, INPUT);
  for (int i = 0; i < N; i++) pinMode(PINS[i].num, INPUT);
}

static void inputMode() {
  allHighZ();
  pinMode(SRC, OUTPUT); digitalWrite(SRC, HIGH);
  for (int i = 0; i < N; i++) { pinMode(PINS[i].num, INPUT_PULLDOWN); last[i] = -1; }
  scanning = true;
  Serial.println("MODO INPUT: encoste fio do 115 nos pinos");
}

static void outputMode(bool highDrive) {
  scanning = false;
  allHighZ();
  for (uint8_t p : OUTS) {
    NRF_GPIO_Type *port = (p >= 32) ? NRF_P1 : NRF_P0;
    uint8_t i = p & 31;
    port->PIN_CNF[i] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                       (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
                       ((highDrive ? GPIO_PIN_CNF_DRIVE_H0H1 : GPIO_PIN_CNF_DRIVE_S0S1)
                        << GPIO_PIN_CNF_DRIVE_Pos);
    port->OUTSET = 1UL << i;
  }
  Serial.println(highDrive ? "MODO OUTPUT HIGH-DRIVE: 029, 111, 002, 113 em HIGH"
                           : "MODO OUTPUT normal: 029, 111, 002, 113 em HIGH");
}

void setup() {
  Serial.begin(115200);
  uint32_t t = millis(); while (!Serial && millis() - t < 3000) delay(10);
  inputMode();
}

void loop() {
  while (Serial.available()) {
    switch (Serial.read()) {
      case 'i': inputMode(); break;
      case 'o': outputMode(false); break;
      case 'h': outputMode(true); break;
      case 'z': scanning = false; allHighZ(); Serial.println("MODO HIGH-Z"); break;
    }
  }
  if (scanning) {
    for (int i = 0; i < N; i++) {
      int v = digitalRead(PINS[i].num);
      if (v != last[i]) {
        if (last[i] != -1 || v)
          Serial.printf("%s pino %s (raw %u)\n", v ? "HIGH" : "low ", PINS[i].label, PINS[i].num);
        last[i] = v;
      }
    }
  }
  delay(5);
}
