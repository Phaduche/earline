-- Earline 클라우드 DB (Supabase) 설정
-- Supabase 대시보드 > SQL Editor 에 붙여넣고 Run.
-- 그 다음 Project Settings > API 에서 Project URL 과 anon public 키를 앱 설정에 넣으면 됨.

-- 기기·방향별 개인 학습 모델 (노트북 / 태블릿 가로·세로 / 휴대폰 세로·가로)
create table if not exists posture_profiles (
  user_code   text        not null,
  profile_key text        not null,          -- 'laptop', 'tablet-landscape', 'phone-portrait' ...
  device      text,
  data        jsonb       not null,          -- 학습된 가중치, 정확도, 신호 비중
  updated_at  timestamptz not null default now(),
  primary key (user_code, profile_key)
);

-- 하루 단위, 기기별 자세 기록 (통합 리포트용)
create table if not exists posture_daily (
  user_code  text        not null,
  day        date        not null,
  device     text        not null,           -- 'laptop' | 'tablet' | 'phone'
  good_sec   real        not null default 0,
  bad_sec    real        not null default 0,
  alerts     int         not null default 0,
  updated_at timestamptz not null default now(),
  primary key (user_code, day, device)
);

alter table posture_profiles enable row level security;
alter table posture_daily    enable row level security;

-- ⚠ 시제품용 정책: anon 키로 누구나 읽고 쓸 수 있음. 실제 출시 전엔 Supabase Auth로 바꿀 것.
drop policy if exists prototype_profiles on posture_profiles;
drop policy if exists prototype_daily    on posture_daily;
create policy prototype_profiles on posture_profiles for all to anon using (true) with check (true);
create policy prototype_daily    on posture_daily    for all to anon using (true) with check (true);
