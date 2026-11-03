// Earline vibration cushion (ESP32)
// 웹앱이 거북목 알림을 보낼 때 BLE로 신호를 받아 진동모터를 울리는 펌웨어.
// The cushion has no sensors: it only buzzes when the web app tells it to.
//
// 배선 (코인/원통형 진동모터 3V)
//   GPIO 25 ── 1kΩ ── NPN(2N2222) 베이스
//   모터 + ── 3.3V,  모터 − ── NPN 컬렉터,  NPN 이미터 ── GND
//   모터 양단에 1N4148 다이오드 역방향(플라이백) 연결
//
// 웹앱과 맞춘 BLE 형식 (바꾸면 index.html의 CUSHION_* 도 같이 바꿀 것)
//   Service UUID        4fafc201-1fb5-459e-8fcc-c5c9c331914b
//   Characteristic UUID beb5483e-36e1-4688-b7f5-ea07361b26a8  (WRITE)
//   받는 값 2바이트: [pattern, strength]
//     pattern 1 = 거북목 알림 (두 번 진동), 2 = 테스트 (한 번 진동)
//     strength 0~255 (gentle 150, medium 205, strong 255)

#include <BLEDevice.h>
#include <BLEServer.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define MOTOR_PIN    25

// 진동 패턴: 켜기 ms, 끄기 ms, 켜기 ms ... 0이 나오면 끝
const uint16_t PATTERN_ALERT[] = {260, 160, 260, 0};
const uint16_t PATTERN_TEST[]  = {400, 0};

volatile uint8_t  pendingPattern = 0;
volatile uint8_t  pendingStrength = 205;
const uint16_t   *pat = nullptr;
uint8_t           step = 0, strength = 205;
unsigned long     stepStart = 0;

class ServerCB : public BLEServerCallbacks {
  void onDisconnect(BLEServer *s) override { s->getAdvertising()->start(); }
};

class WriteCB : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    auto v = c->getValue();                 // String (core 3.x) or std::string (core 2.x)
    if (v.length() >= 1) pendingPattern = (uint8_t)v[0];
    if (v.length() >= 2) pendingStrength = (uint8_t)v[1];
  }
};

void setup() {
  pinMode(MOTOR_PIN, OUTPUT);
  analogWrite(MOTOR_PIN, 0);
  BLEDevice::init("Earline Cushion");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCB());
  BLEService *svc = server->createService(SERVICE_UUID);
  BLECharacteristic *ch = svc->createCharacteristic(
      CHAR_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  ch->setCallbacks(new WriteCB());
  svc->start();
  BLEAdvertising *ad = BLEDevice::getAdvertising();
  ad->addServiceUUID(SERVICE_UUID);
  ad->setScanResponse(true);
  BLEDevice::startAdvertising();
}

void loop() {
  // 새 명령이 오면 패턴 시작
  if (pendingPattern) {
    pat = pendingPattern == 1 ? PATTERN_ALERT : PATTERN_TEST;
    strength = pendingStrength;
    pendingPattern = 0;
    step = 0;
    stepStart = millis();
    analogWrite(MOTOR_PIN, strength);
  }
  // 패턴 진행 (delay 없이)
  if (pat) {
    uint16_t dur = pat[step];
    if (dur == 0) {                         // 끝
      analogWrite(MOTOR_PIN, 0);
      pat = nullptr;
    } else if (millis() - stepStart >= dur) {
      step++;
      stepStart = millis();
      analogWrite(MOTOR_PIN, step % 2 == 0 ? strength : 0);
    }
  }
  delay(5);
}
