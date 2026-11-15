// Earline 진동 방석 (ESP32 + 진동 모터 모듈 10개)
// 웹앱에서 거북목 알림이 나가면 블루투스(BLE)로 신호를 받아 방석 속 모듈을 울린다. 센서는 없음.
//
// 사용 부품: "진동 모터 모듈 PWM DC 모터 드라이버 3.3V 5V" (핀 IN · VCC · GND) × 10
//   모듈 안에 드라이버(트랜지스터)가 있어서 ESP32 핀에 바로 연결한다.
//
// 구역: 방석을 왼쪽→오른쪽 5구역으로 나누고, 구역마다 모듈 2개(앞·뒤)를 같은 핀에 연결
//   구역1 GPIO 27 · 구역2 GPIO 26 · 구역3 GPIO 25 · 구역4 GPIO 33 · 구역5 GPIO 32
//   구역 2·4(모듈 4개)는 브레드보드 위쪽 절반에, 구역 1·3·5(모듈 6개)는 아래쪽 절반에 꽂는다.
//   (브레드보드에 꽂은 ESP32에서 한쪽 줄만 쓸 수 있어서, 그 줄의 출력 핀 5개를 쓴다)
//
// 배선 (모듈마다)
//   모듈 IN  ── 자기 구역의 GPIO (앞·뒤 모듈 IN 둘 다 같은 핀)
//   모듈 VCC ── + 레일 = 배터리 + (AA 2개 = 3V, 또는 AA 3개 = 4.5V)
//   모듈 GND ── − 레일 = 배터리 − = ESP32 GND   (GND는 꼭 서로 연결)
//   위·아래 + 레일, 위·아래 − 레일은 점퍼로 서로 이어 둔다 (위쪽 모듈 4개가 위 레일을 쓰므로)
//   ESP32 전원은 USB 케이블. ESP32의 3V3, 5V 핀은 배터리 레일에 연결하지 말 것.
//
// 전원 때문에 한 번에 최대 3구역(모듈 6개)까지만 같이 켠다. 10개를 동시에 켜면 건전지가 버거움.
//
// 웹앱과 맞춘 BLE 형식 (바꾸면 index.html의 CUSHION_* 도 같이 바꿀 것)
//   Service UUID        4fafc201-1fb5-459e-8fcc-c5c9c331914b
//   Characteristic UUID beb5483e-36e1-4688-b7f5-ea07361b26a8  (WRITE)
//   받는 값 [pattern, strength]
//     pattern 1 = 거북목 알림, 2 = 연결 테스트, 0 = 멈춤
//     strength 0~255 (웹앱은 205를 보냄)
//
// 웹앱 없이 테스트: 시리얼 모니터(115200)에서 1 = 알림, 2 = 테스트, 0 = 멈춤
// 전원을 켜면 구역 1 → 2 → 3 → 4 → 5 순서로 한 번씩 울려서 배선을 확인할 수 있다.

#include <BLEDevice.h>
#include <BLEServer.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"

const uint8_t ZONES = 5;
const uint8_t PINS[ZONES] = {27, 26, 25, 33, 32};  // 구역1(왼쪽 끝) ~ 구역5(오른쪽 끝)
// 보통 모듈은 IN이 HIGH일 때 울린다. 꺼져 있어야 할 때 계속 울리면 true로 바꿀 것.
const bool ACTIVE_LOW = false;
// 모듈 VCC에 연결한 AA 건전지 개수: 2개(3V)면 2, 3개(4.5V)면 3
//   2개일 때는 전압이 낮아서 앱이 보낸 세기와 상관없이 항상 최대 세기로 울린다.
const uint8_t AA_CELLS = 2;

// 진동 패턴: {켤 구역(비트, 1번 비트 = 구역1), 시간 ms}. 시간이 0이면 끝.
struct Step { uint8_t mask; uint16_t ms; };
// 거북목 알림: 왼쪽 → 오른쪽 물결, 오른쪽 → 왼쪽 물결, 그다음 1·3·5 / 2·4 구역이 번갈아 진동
const Step ALERT[] = { {0b00001, 120}, {0b00010, 120}, {0b00100, 120}, {0b01000, 120}, {0b10000, 120}, {0, 80},
                       {0b10000, 120}, {0b01000, 120}, {0b00100, 120}, {0b00010, 120}, {0b00001, 120}, {0, 140},
                       {0b10101, 260}, {0, 140}, {0b01010, 260}, {0, 0} };
// 테스트: 구역을 하나씩 울려서 어느 선이 어느 구역인지 확인
const Step TEST[]  = { {0b00001, 350}, {0, 150}, {0b00010, 350}, {0, 150}, {0b00100, 350}, {0, 150},
                       {0b01000, 350}, {0, 150}, {0b10000, 350}, {0, 0} };

// 작은 진동 모터는 약한 PWM으로는 잘 안 돌기 시작해서, 켤 때 잠깐 최대로 밀어 준다
const uint16_t KICK_MS = 40;
const uint8_t  MIN_STRENGTH = 90;

volatile bool    pending = false;
volatile uint8_t pendingPattern = 0, pendingStrength = 205;

const Step   *pat = nullptr;
uint8_t       idx = 0, strength = 205;
unsigned long stepStart = 0;
bool          kicking = false;

void setMotors(uint8_t mask, uint8_t duty) {
  for (uint8_t i = 0; i < ZONES; i++) {
    uint8_t d = ((mask >> i) & 1) ? duty : 0;
    analogWrite(PINS[i], ACTIVE_LOW ? 255 - d : d);
  }
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
  strength = AA_CELLS <= 2 ? 255 : (s < MIN_STRENGTH ? MIN_STRENGTH : s);
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
  for (uint8_t pin : PINS) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, ACTIVE_LOW ? HIGH : LOW);
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

  Serial.println("Earline Cushion 준비 완료. 구역 1 → 2 → 3 → 4 → 5 순서로 울립니다.");
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
