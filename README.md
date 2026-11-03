# Earline — personal turtle-neck alerts (laptop · tablet · phone · vibration cushion)

앱 화면은 전부 영어. 이름 "Earline"은 임시 이름이라 `index.html`에서 찾아 바꾸면 돼요.

## 실행 방법

카메라는 보안 때문에 **https 주소나 localhost**에서만 켜져요.

**노트북**
```bash
cd earline
python3 -m http.server 8000
```
크롬에서 `http://localhost:8000` 열기.

**아이패드 · 휴대폰 (https 필요)**
- GitHub Pages: 레포에 이 폴더 올리고 Settings → Pages 켜기 → `https://아이디.github.io/레포이름/`
- 또는 app.netlify.com/drop 에 폴더째 끌어다 놓기
- 아이폰·아이패드 푸시 알림: Safari 공유 → **Add to Home Screen** 후 그 아이콘으로 열기 (iOS 16.4+)

**카메라 없이 흐름 테스트**: 주소 뒤에 `?sim` → 키보드 `t` 바른자세/거북목 전환, `a` 자리비움.

## 동작 (Face-id와 같은 3단계)

| Face-id | Earline | 내용 |
|---|---|---|
| Part1 얼굴 100장 수집 | Step 1 | 바른 자세 60개, 거북목 60개 샘플 기록 (줄자 모양 진행바) |
| Part2 LBPH 학습 | Step 2 | 특징 10개 표준화 → 로지스틱 회귀 학습, 뒤 20%로 검증 정확도 |
| Part3 confidence > 75 | Step 3 | 거북목 확률 **90% 이상**이 3초 유지되면 알림 (화면 + 소리 + 푸시 + 방석 진동) |

MediaPipe(얼굴 478점, 몸 33점)를 브라우저 안에서 돌려서 영상은 기기 밖으로 안 나가요.

## 정면 그림

카메라는 정면에서 찍으니까 그림도 정면. 정면에서 보면 거북목은 **목이 사라지는 것**처럼 보여요: 머리가 어깨 쪽으로 내려오고(목 길이 ↓), 카메라에 가까워져 커지고, 어깨가 말려 올라감. 그림은 거북목 확률에 따라 이렇게 변하고, 귀·어깨에 붙은 점은 물리치료사가 자세 평가할 때 붙이는 마커를 본뜬 것.

보드 왼쪽 위 "Neck height 78% of your upright baseline"은 **실제 측정값**: 지금 귀-어깨 높이를 내가 학습 때 기록한 바른 자세 평균과 비교한 비율 (어깨가 보일 때만 표시).

## 특징 10개
- 얼굴: Head tilt down, Face size (distance), Face height in frame, Face shape ratio, Chin to nose
- 어깨: Shoulder width, Ear-to-shoulder height, Nose-to-shoulder height, Head-to-shoulder ratio, **3D neck angle (CVA)** ← 물리치료에서 거북목 진단에 쓰는 두개척추각을 3D 좌표로 근사

## 아이패드 각도·방향 문제

기기·방향마다 따로 학습: `laptop`, `tablet-landscape`, `tablet-portrait`, `phone-portrait`, `phone-landscape`. 아이패드를 돌리면 "isn't calibrated yet" 배너가 뜨고 그 방향으로 한 번 더 학습. 절대 각도가 아니라 "그 카메라 위치에서 본 내 바른 자세 vs 내 거북목"을 비교하니까 카메라가 옆이나 아래에 있어도 됨. 어깨가 안 보이면 얼굴 특징만 쓰는 모델로 자동 전환.

## 백그라운드
- 노트북(크롬·엣지): 다른 탭/앱 써도 계속 감지. **Mini widget** 버튼으로 화면 구석에 항상 떠 있는 작은 창
- 휴대폰·태블릿: 브라우저 정책상 페이지를 나가면 카메라가 꺼짐 → 화면 켜둔 동안 감지 (화면 꺼짐 방지 켬). 진짜 백그라운드는 네이티브 앱 단계

## 진동 방석 (신민기)

방석에는 센서 없이 **진동모터만** 있음. 웹앱이 거북목 알림을 보낼 때 블루투스로 `[1, 세기]`를 보내면 두 번 진동. `cushion_esp32.ino`에 배선과 펌웨어 있음.
- 앱: Devices 탭 또는 Settings → Vibration cushion → Connect (노트북·안드로이드 크롬. 아이폰 Safari는 웹 블루투스 미지원)
- 세기: gentle / medium / strong, Test buzz 버튼으로 확인

## 클라우드 DB (통합 플랫폼)
1. supabase.com 무료 프로젝트
2. SQL Editor에서 `supabase_setup.sql` 실행
3. 앱 Settings → Cloud sync에 URL, anon key, 코드(예: `suah-01`) 입력
4. 노트북·아이패드·폰에서 같은 코드 → 학습 결과와 하루 기록이 모여서 Devices 탭에 전체 점수로 보임

DB 없이도 기기 안에 저장되고, Export/Import로 JSON 파일로 옮길 수 있음.

## 파일
- `index.html` 앱 전체
- `sw.js` 알림용 서비스 워커
- `manifest.webmanifest`, `icon*` 홈 화면 설치용
- `supabase_setup.sql` 클라우드 DB
- `cushion_esp32.ino` 진동 방석 펌웨어
