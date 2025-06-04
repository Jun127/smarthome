/*******************************************************************
 *  ESP8266 + Firebase + IRremote 2.8.6  (Fan + Hitachi_AC344 + Light)
 *******************************************************************/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Hitachi.h>            // Hitachi_AC344

/* ─────────── Wi-Fi ─────────── */
const char* WIFI_SSID     = "M2091";
const char* WIFI_PASSWORD = "m2091wpa";

/* ─────────── Firebase SA ───── */
#define FIREBASE_HOST "mysmarthome-a8989-default-rtdb.firebaseio.com"
// ↓↓↓ 以下三個常數沿用你原本的 ↓↓↓
static const char PRIVATE_KEY[] PROGMEM = R"KEY(
-----BEGIN PRIVATE KEY-----
MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQDRdLdlFZxLbrMq
5OLr9/+QO3HXkYOx3FLIXjmSTxJQGbxg+1CeXwibVD2wu5sw/QKeal9Ev7PYPkvE
24ctombWSvqpwhu4FICdgbpe0cbkZ8jqCK/Eb0clt3kKfoevasipJRPGcq95PT2h
ohAk9DLJsuGnYDOOZYlC7Q2NiVPRZMFyjgm6Opg0+hlDG5PcyfRD1lxlKIzTVJSO
j3KRdG8TA6Ci5ijUvNopns7f90XUgCiaeqbYs+PufjuQsgmgKICSxexZOTyB+w9c
UywbEUjN+MbZHozZqKXYtsgMe1KJ8LCellGjfC+uldVacGk04ftyyMN+FBjdBPU0
x9mnKpkhAgMBAAECggEAMG/Yz1gBhlIpvcQjk+UMZRrSF0NP3KwyOqLLloGRnxNs
hFLp0E3yzBTdlUa1O01vMxovQNNCMEIrvHD14EA7ntfFAsfgKCAZpBLxeebyIzS6
+iAbIYaIQfkdVSdwkkjT8YWg/pXb7WITpBK5wAn2yr0CwPWWxpnFdcQjC7WTvAqw
7ETU1CwAIgFoiwbq8iT6iBLi3KtyQv/Y0+RP9wcmIt8TkIcyi1Dq7xqajINi5Y+c
lcHUNLDtoj7ld4FekeE5l0SzNZFkD5dAFku8WQ/K/ZR/kS353l0xPGSH23rCan01
Qic/ufoTcNCTQxGxYR9kN+mvzEhPJQOVF2mbHW61LwKBgQD5NALukQwAwIqQwI16
wCOCvCwIToeOt50BaUTXrsDTQrs0Iu/4Giz7YFJ6P9Gt3RsSrOHh/XxAFwChZ0+d
zGtku/BuWZpcJ24vyh8/eSi8kz096ixBBDtB6kaACP5HvDTdoi0A3fZ5JeoIPCZG
z4bVR6R8jgGS7iLuMNFbhP7/bwKBgQDXKy5tzYGGJjohOB1Y74RnbYs/xFP34E7B
1HRa3F2GyISEQY0oTLVZMMzwt+5m598S8u+Zw/9k4kz8FWvhE109v8Wl9ZxU/6j2
AsEX1gZgm+9qMjmNbSNG5t3el97P8wLh/O9VzStJFBOssUoEWVVILteyFwOXnyAn
HAnXbheobwKBgC3IKGG7tADPXqWJOnS6p8t01oIQK0dE5EtGE6esKzCkqc/CO2PR
K0JjR8O9xb+zRy3/JOBnuVPs5ejBGhdbXr9654jeXUVg2RVehK8ciydZxX6Cbu7i
4CAT8i/DdzOml++/w5TNMmK/XGd25XFM3vB/4PAhBpj/ttsbjNrLbC1xAoGAKPKW
y16on5b2yQucyigK0oQOU/xQ3oNZeZms6v207smKLoErE9hFSsv2tHjMbdkCouCI
qIKcP96xN+f7t8GvpOix/HtZXPOqe+baSGb6n7gNP/B+82Mpq3yUDKBqTri4e7le
8r0w159yfxLeOyWlDS+c7M9kZ55oyAiV9uGErxECgYB+v0TRfFgbPytMqZ8+Bzam
3vaBw0ZiXR6FMlOYhnBO3tJId9alwkAPxSRgdYs7c3D3QKTmcYpWQEndap1hCvYD
bD3rX0GlhPw+JA03GhzElomGD/BZbOhXmqJGGrHYrLxTAikwPVv8INmc3nuHd/Uu
Qemz76lvUYLmZhLtnQdV4Q==
-----END PRIVATE KEY-----
)KEY";
static const char CLIENT_EMAIL[] = "firebase-adminsdk-fbsvc@mysmarthome-a8989.iam.gserviceaccount.com";
static const char PROJECT_ID[]   = "mysmarthome-a8989";

FirebaseAuth   auth;
FirebaseConfig config;

/* ─────────── IR GPIO / 物件 ── */
const uint16_t kIrLedPin = D2;  // NodeMCU D2(GPIO4)
IRsend         irsend(kIrLedPin);      // 給「風扇 raw」與「燈 NEC」  
IRHitachiAc344 ac(kIrLedPin, 0);       // Hitachi AC model-0

/* ─────────── 風扇 raw (完全照舊) ── */
#include "ir_raw_fan_ac.h"      // 內含 kFAN_***_SYM 與 AC raw tables

void sendFanCommand(uint16_t symbol) {
    for (uint8_t i = 0; i < 3; i++) {
        irsend.sendSymphony(symbol, 12);
        delay(100);
    }
}
void fanPowerToggleOnce() {
  sendFanCommand(kFAN_PWR_SYM);
}
void fanSpeedToggleOnce() {
  sendFanCommand(kFAN_SPEED_SYM);
}
void fanTimerToggleOnce() {
  sendFanCommand(kFAN_TIMER_SYM);
}
void fanSwingToggleOnce() {
  sendFanCommand(kFAN_SWING_SYM);
}
/* ─────────── 冷氣狀態暫存 ────── */
uint8_t acTemp   = 26;          // 18~32 °C
uint8_t acFanIdx = 0;           // 0~3 依下表
const uint8_t fanTbl[4] = {
  kHitachiAc344FanAuto,
  kHitachiAc344FanHigh,
  kHitachiAc344FanLow,
  kHitachiAc344FanMax
};
const uint8_t kRepeat = 2;      // 每個 AC 指令重送 2 次

inline void acSend() {
  for(uint8_t i=0;i<kRepeat;i++){ ac.send(); delay(120); }
}

/* ─────────── Firebase 路徑 & 連線 ────────── */
const String FAN_PATH   = "/fan/power";
const String AC_PATH    = "/ac/power";
const String LIGHT_PATH = "/light/power";

FirebaseData fbFan, fbAC, fbLight;

/* ========== setup() ========== */
void setup() {
  Serial.begin(115200);

  /* 1. Wi-Fi */
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi");
  while (WiFi.status()!=WL_CONNECTED){delay(300);Serial.print('.');}
  Serial.printf(" connected  IP=%s\n", WiFi.localIP().toString().c_str());

  /* 2. Firebase SA */
  config.database_url                      = "https://" FIREBASE_HOST;
  config.service_account.data.client_email = CLIENT_EMAIL;
  config.service_account.data.project_id   = PROJECT_ID;
  config.service_account.data.private_key  = PRIVATE_KEY;
  Firebase.begin(&config,&auth);
  Firebase.reconnectWiFi(true);

  /* 3. IR */
  irsend.begin();   // Fan / Light
  ac.begin();
  ac.setMode(kHitachiAc344Cool);
  ac.setFan(fanTbl[acFanIdx]);
  ac.setTemp(acTemp);
  ac.setPower(false);

  /* 4. 打開三條 Stream */
  if(!Firebase.beginStream(fbFan, FAN_PATH))
      Serial.println("[ERR] FanStream "+fbFan.errorReason());
  if(!Firebase.beginStream(fbAC,  AC_PATH))
      Serial.println("[ERR] ACStream "+fbAC.errorReason());
  if(!Firebase.beginStream(fbLight, LIGHT_PATH))
      Serial.println("[ERR] LightStream "+fbLight.errorReason());
}

/* 便利函式：讀取並處理一條 Stream */
template<typename F>
void pollStream(FirebaseData& fb, F handler){
  if(Firebase.ready() && Firebase.readStream(fb) && fb.streamAvailable())
      handler(fb.stringData());
}

/* ========== loop() ========== */
void loop() {

  /* ── Fan ── */
  pollStream(fbFan,[&](const String& cmd){
    Serial.println("[Fan ] "+cmd);
    if      (cmd=="FAN_START")      fanPowerToggleOnce();
    else if (cmd=="FAN_STOP")       fanPowerToggleOnce();
    else if (cmd=="FAN_SPEED")      fanSpeedToggleOnce();
    else if (cmd=="FAN_SWING_ON")   fanSwingToggleOnce();
    else if (cmd=="FAN_SWING_OFF")  fanSwingToggleOnce();
    else if (cmd=="FAN_TIMER")      fanTimerToggleOnce();
  });

  /* ── AC ── */
  pollStream(fbAC,[&](const String& cmd){
    Serial.println("[AC  ] "+cmd);
    if      (cmd=="AC_START"){ ac.setPower(true);  acSend(); }
    else if (cmd=="AC_STOP"){  ac.setPower(false); acSend(); }
    else if (cmd=="AC_TEMP_UP" && acTemp<32){
        ac.setTemp(++acTemp); acSend();
    }
    else if (cmd=="AC_TEMP_DN" && acTemp>18){
        ac.setTemp(--acTemp); acSend();
    }
    else if (cmd=="AC_SPEED"){
        acFanIdx = (acFanIdx+1)%4;
        ac.setFan(fanTbl[acFanIdx]); acSend();
    }
  });

  /* ── Light ── */
  pollStream(fbLight,[&](const String& cmd){
    Serial.println("[LED ] "+cmd);
    uint32_t nec = 0xFF8877;              // 32-bit NEC code
    for(uint8_t i=0;i<3;i++){             // 多送 3 次
      irsend.sendNEC(nec,32);
      delay(90);
    }
  });

  delay(40);   // 主迴圈節流
}
