// Earline 진동 방석 (ESP32 + 코인 진동모터 3개)
// 웹앱에서 거북목 알림이 나가면 블루투스(BLE)로 신호를 받아 모터 3개를 울린다. 센서는 없음.
//
// 배선: 모터 하나당 아래 회로를 3번 (GPIO 25, 26, 27)
//   GPIO ── 1kΩ 저항 ── NPN 트랜지스터(2N2222A, S8050) B(베이스)
//   트랜지스터 E(이미터) ── − 레일(GND)
//   트랜지스터 C(컬렉터) ── 모터 −(파랑/검정)
//   모터 +(빨강) ── + 레일 = AA 2개 배터리(3V) +
//   다이오드(1N4148, 1N4001): 띠 쪽 ── + 레일, 반대쪽 ── 모터 −
//   배터리 − ── − 레일 ── ESP32 GND   (GND는 꼭 서로 연결)
//   ESP32 전원은 USB 케이블. ESP32의 3V3, 5V 핀은 배터리 레일에 연결하지 말 것.
//
// 웹앱과 맞춘 BLE 형식 (바꾸면 index.html의 CUSHION_* 도 같이 바꿀 것)
//   Service UUID        4fafc201-1fb5-459e-8fcc-c5c9c331914b
//   Characteristic UUID beb5483e-36e1-4688-b7f5-ea07361b26a8  (WRITE)
//   받는 값 [pattern, strength]
//     pattern 1 = 거북목 알림, 2 = 연결 테스트, 0 = 멈춤
//     strength 0~255 (웹앱은 205를 보냄)
//
// 웹앱 없이 테스트: 시리얼 모니터(115200)에서 1 = 알림, 2 = 테스트, 0 = 멈춤
// 전원을 켜면 모터 1 → 2 → 3 → 셋 다 순서로 한 번씩 울려서 배선을 확인할 수 있다.

#include <BLEDevice.h>
#include <BLEServer.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"

const uint8_t MOTORS[3] = {25, 26, 27};  // 모터1(왼쪽), 모터2(가운데), 모터3(오른쪽)
const uint8_t ALL = 0b111;

// 진동 패턴: {켤 모터(비트), 시간 ms}. 시간이 0이면 끝.
struct Step { uint8_t mask; uint16_t ms; };
// 거북목 알림: 왼쪽 → 가운데 → 오른쪽 물결, 그다음 셋이 같이 두 번
const Step ALERT[] = { {0b001, 140}, {0b010, 140}, {0b100, 140}, {0, 120},
                       {ALL, 280}, {0, 160}, {ALL, 280}, {0, 0} };
// 테스트: 모터를 하나씩 울려서 어느 선이 어느 모터인지 확인
const Step TEST[]  = { {0b001, 350}, {0, 150}, {0b010, 350}, {0, 150},
                       {0b100, 350}, {0, 150}, {ALL, 400}, {0, 0} };

// 코인 모터는 약한 PWM으로는 잘 안 돌기 시작해서, 켤 때 잠깐 최대로 밀어 준다
const uint16_t KICK_MS = 40;
const uint8_t  MIN_STRENGTH = 90;

volatile bool    pending = false;
volatile uint8_t pendingPattern = 0, pendingStrength = 205;

const Step   *pat = nullptr;
uint8_t       idx = 0, strength = 205;
unsigned long stepStart = 0;
bool          kicking = false;

void setMotors(uint8_t mask, uint8_t duty) {
  for (uint8_t i = 0; i < 3; i++) analogWrite(MOTORS[i], ((mask >> i) & 1) ? duty : 0);
}

void startStep() {
  stepStart = millis();
  if (pat[idx].ms == 0) {          // 패턴 끝
    setMotors(0, 0);
    pat = nullptr;
    return;
  }
  kicking = pat[idx].mask != 0;
  setMotors(pat[idx].mask, kicking ? 255 : 0);
}

void play(uint8_t pattern, uint8_t s) {
  if (pattern != 1 && pattern != 2) {  // 0이나 모르는 값이면 멈춤
    pat = nullptr;
    setMotors(0, 0);
    return;
  }
  strength = s < MIN_STRENGTH ? MIN_STRENGTH : s;
  pat = pattern == 1 ? ALERT : TEST;
  idx = 0;
  startStep();
  Serial.println(pattern == 1 ? "거북목 알림 진동" : "테스트 진동");
}

class ServerCB : public BLEServerCallbacks {
  void onConnect(BLEServer *) override { Serial.println("웹앱 연결됨"); }
  void onDisconnect(BLEServer *s) override {
    Serial.println("연결 끊김, 다시 기다리는 중");
    s->getAdvertising()->start();
  }
};

class WriteCB : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    auto v = c->getValue();            // core 3.x는 String, 2.x는 std::string
    if (v.length() < 1) return;
    pendingStrength = v.length() >= 2 ? (uint8_t)v[1] : 205;
    pendingPattern = (uint8_t)v[0];
    pending = true;                    // 실제 진동은 loop()에서
  }
};

void setup() {
  for (uint8_t pin : MOTORS) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  Serial.begin(115200);

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

  Serial.println("Earline Cushion 준비 완료. 모터 1 → 2 → 3 → 전체 순서로 울립니다.");
  play(2, 205);
}

void loop() {
  if (pending) {
    pending = false;
    play(pendingPattern, pendingStrength);
  }
  while (Serial.available()) {
    char c = Serial.read();
    if (c >= '0' && c <= '2') play(c - '0', 205);
  }
  if (pat) {
    unsigned long t = millis() - stepStart;
    if (kicking && t >= KICK_MS) {     // 시작 밀어주기 끝 → 원래 세기로
      kicking = false;
      setMotors(pat[idx].mask, strength);
    }
    if (t >= pat[idx].ms) {
      idx++;
      startStep();
    }
  }
  delay(2);
}
