/* Circuito SenseGuard
Entradas: 4 PIRs (sensores de movimento)
Saídas: 4 LEDs 
--------------------- // ---------------------
Autor: Antônio de Souza Cerdeira, Anna Júlia Xavier Martins, Gabriel Ferreira de Souza, Giovanna Calderon Lachi,
Lucca Sandri Tonel, Tchinndjia Rufina Ferreira de Gonzaga, Vinícius Estevem de Paula
--------------------- // ---------------------
Data: 24/11/2025
--------------------- // ---------------------
Versão 6.0: Versão funcional com 1 ESP32, que é responsável por realizar o acionamento de cada LED de acordo com cada PIR,
além de possuir um monitoramento por Bluetooth BLE pelo aplicativo LightBlue.
--------------------- // ---------------------
Version 6.0: Functional version with 1 ESP32, responsible for controlling each LED according to each PIR, 
and also providing monitoring via BLE Bluetooth through the LightBlue app.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// === PIR -> LED ===
// PIR 33 -> LED 18 (ESCADA)
// PIR 32 -> LED 23 (CORREDOR)
// PIR 36 -> LED 2  (QUARTO)
// PIR 39 -> LED 4  (BANHEIRO)

int pirPins[4] = {33, 32, 36, 39};
int ledPins[4] = {18, 23, 2, 4};

String nomes[4] = {
  "ESCADA",
  "CORREDOR",
  "QUARTO",
  "BANHEIRO"
};

// Estados
int lastPirState[4] = {0, 0, 0, 0};
unsigned long lastMotionTime[4] = {0, 0, 0, 0};

unsigned long ledTimeout = 5000; // 5 segundos

// BLE
BLECharacteristic *estadoChar;

void enviaBLE(String msg) {
  Serial.println("[BLE] " + msg);
  estadoChar->setValue(msg.c_str());
  estadoChar->notify();
}

void setup() {
  Serial.begin(9600);

  // Configura pinos
  for (int i = 0; i < 4; i++) {
    pinMode(pirPins[i], INPUT);
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  // ===== BLE =====
  BLEDevice::init("SenseGuard-ESP32");
  BLEServer *server = BLEDevice::createServer();
  BLEService *service = server->createService("1234");

  estadoChar = service->createCharacteristic(
    "ABCD",
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  estadoChar->addDescriptor(new BLE2902());
  estadoChar->setValue("READY");

  service->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID("1234");
  adv->setScanResponse(true);
  adv->start();

  Serial.println("BLE pronto! Abra o LightBlue e conecte.");
}

void loop() {
  unsigned long agora = millis();

  for (int i = 0; i < 4; i++) {

    int leitura = digitalRead(pirPins[i]);

    // Mudança no PIR → liga LED, mas NOTIFICAÇÃO SÓ PELO LED
    if (leitura != lastPirState[i]) {
      lastPirState[i] = leitura;

      if (leitura == HIGH) {
        digitalWrite(ledPins[i], HIGH);
        lastMotionTime[i] = agora;

        // Notificação baseada no estado do LED, não do PIR
        enviaBLE(nomes[i] + "_ON");
      }
    }

    // Apaga LED por timeout → manda OFF
    if (digitalRead(ledPins[i]) == HIGH &&
        (agora - lastMotionTime[i] > ledTimeout)) {

      digitalWrite(ledPins[i], LOW);
      enviaBLE(nomes[i] + "_OFF");
    }
  }
}
