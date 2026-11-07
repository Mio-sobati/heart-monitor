#include <Wire.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "MAX30105.h"
#include "heartRate.h"

// ==========================================
// 1. پایه‌ها
// ==========================================
#define CANCEL_BTN_PIN    10 
#define BUZZER_PIN        1  
#define VIBRATOR_PIN      21 

#define SIM800_RX_PIN     4  
#define SIM800_TX_PIN     5  

#define GYRO_RX_PIN       2  
#define GYRO_TX_PIN       3  

#define BUZZER_ON   LOW
#define BUZZER_OFF  HIGH

// ==========================================
// 2. متغیرها
// ==========================================
Preferences preferences;
String phone1;
String wifiPass;
int maxHR, minHR;
int fallSens, alarmDelay;
int panicSens, panicHR, panicDur;
bool isConfigMode = false;

MAX30105 particleSensor;
const byte RATE_SIZE = 4; 
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute = 0;
int beatAvg = 0;
long rawIR = 0;

HardwareSerial sim800(0); 
HardwareSerial gyroSerial(1); 

unsigned char re_buf[8];
unsigned char counter = 0;
float pitch = 0.0, yaw = 0.0, roll = 0.0;

float lastPitch = 0.0, lastRoll = 0.0;
unsigned long lastAngleCheckTime = 0;
int fallState = 0; 
unsigned long fallTimer = 0;
bool isFirstAngleRead = true; 

bool possiblePanic = false;
unsigned long panicTimer = 0;

bool isHrAbnormal = false;
unsigned long hrAbnormalTimer = 0;
String hrReason = "", hrSReason = "";

bool isAlarmTriggered = false;
bool inCooldown = false;
unsigned long alarmStartTime = 0, cooldownStartTime = 0;
String currentEmergencyReason = "", smsReason = "";

int btnClickCount = 0;
unsigned long lastBtnClickTime = 0;
bool lastBtnState = HIGH;

WebServer server(80);
DNSServer dnsServer; 
const byte DNS_PORT = 53;

// ==========================================
// 3. توابع پیامک
// ==========================================
String convertToHEX(String asciiString) {
  String hexStr = "";
  for (int i = 0; i < asciiString.length(); i++) {
    hexStr += "00"; 
    int c = asciiString[i];
    if (c < 16) hexStr += "0"; 
    String hexChar = String(c, HEX);
    hexChar.toUpperCase();
    hexStr += hexChar;
  }
  return hexStr;
}

void sendSMS(String phoneNumber, String message) {
  if (phoneNumber.length() < 10) return; 
  sim800.print("AT\r\n"); delay(200); 
  sim800.print("AT+CMGF=1\r\n"); delay(300); 
  sim800.print("AT+CSCS=\"UCS2\"\r\n"); delay(300);
  sim800.print("AT+CSMP=17,167,0,8\r\n"); delay(300);
  
  String phoneHex = convertToHEX(phoneNumber);
  String msgHex = convertToHEX(message);
  
  sim800.print("AT+CMGS=\""); sim800.print(phoneHex); sim800.print("\"\r\n"); 
  delay(1000);
  sim800.print(msgHex); delay(500);
  sim800.write(26); delay(3000); 
}

// ==========================================
// 4. توابع وب‌سرور 
// ==========================================
void loadSettings() {
  preferences.begin("medical_app", false);
  wifiPass = preferences.getString("wifiPass", "12345678");
  phone1 = preferences.getString("phone1", "09120000000");
  maxHR = preferences.getInt("maxHR", 120);
  minHR = preferences.getInt("minHR", 50);
  fallSens = preferences.getInt("fallSens", 40);
  alarmDelay = preferences.getInt("alarmDelay", 15);
  panicSens = preferences.getInt("panicSens", 15);
  panicHR = preferences.getInt("panicHR", 100);
  panicDur = preferences.getInt("panicDur", 10);
  preferences.end();
}

void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Heart Monitor</title>
    <style>
      @import url('https://fonts.googleapis.com/css2?family=Vazirmatn:wght@400;600;800&display=swap');
      :root {
        --bg-grad: linear-gradient(135deg, #fff0f0 0%, #ffffff 50%, #ffe0e0 100%);
        --card-bg: rgba(255, 255, 255, 0.7);
        --text-primary: #410002;
        --text-secondary: #93000a;
        --input-bg: rgba(255,255,255,0.9);
        --border-col: #ffb4ab;
        --nav-bg: rgba(255,255,255,0.85);
        --btn-bg: #ba1a1a;
      }
      [data-theme="dark"] {
        --bg-grad: linear-gradient(135deg, #200000 0%, #100000 50%, #2a0000 100%);
        --card-bg: rgba(30, 0, 0, 0.6);
        --text-primary: #ffdad6;
        --text-secondary: #ffb4ab;
        --input-bg: rgba(10,0,0,0.8);
        --border-col: #93000a;
        --nav-bg: rgba(30,0,0,0.85);
        --btn-bg: #93000a;
      }
      body { font-family: 'Vazirmatn', Tahoma, sans-serif; text-align: center; direction: rtl; margin: 0; padding: 0; background: var(--bg-grad); background-attachment: fixed; color: var(--text-primary); padding-bottom: 90px; transition: all 0.4s;}
      
      .top-bar { display: flex; justify-content: space-between; padding: 15px 25px; }
      .icon-btn { background: var(--card-bg); border: 1px solid var(--border-col); color: var(--text-primary); border-radius: 50%; width: 45px; height: 45px; font-size: 20px; cursor: pointer; backdrop-filter: blur(10px); box-shadow: 0 4px 10px rgba(0,0,0,0.1); }
      .icon-btn.lang-btn { width: auto; border-radius: 20px; padding: 0 15px; font-weight: bold; font-size: 14px;}
      
      .tab-content { display: none; padding: 0 20px; animation: fadeIn 0.3s;}
      .tab-content.active { display: block; }
      @keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }
      .card { max-width: 400px; margin: 10px auto; background: var(--card-bg); backdrop-filter: blur(25px); -webkit-backdrop-filter: blur(25px); border: 1px solid var(--border-col); padding: 20px; border-radius: 28px; box-shadow: 0 8px 32px rgba(0,0,0,0.08); transition: all 0.4s;}
      h2 { color: var(--btn-bg); margin-top: 0; font-weight: 800; font-size: 20px;}
      .bpm-display { font-size: 65px; font-weight: 800; color: var(--btn-bg); margin: 10px 0; text-shadow: 2px 2px 15px rgba(186,26,26,0.3);}
      
      .category-box { border: 1.5px solid var(--border-col); border-radius: 20px; padding: 15px; margin-bottom: 20px; text-align: right; background: rgba(0,0,0,0.02);}
      .category-box h3 { margin-top: 0; font-size: 15px; color: var(--text-primary); font-weight: 800; border-bottom: 1px solid var(--border-col); padding-bottom: 8px;}
      
      label { font-size: 13px; font-weight: 600; color: var(--text-secondary); display: block; margin-bottom: 5px; margin-top: 10px;}
      input { width: 100%; box-sizing: border-box; padding: 12px; background: var(--input-bg); border: 1px solid var(--border-col); border-radius: 12px; font-size: 16px; direction: ltr; color: var(--text-primary);}
      
      button.primary { background: var(--btn-bg); color: white; border: none; padding: 16px; font-size: 16px; border-radius: 100px; cursor: pointer; width: 100%; font-weight: 800; margin-top: 10px; box-shadow: 0 4px 12px rgba(186,26,26,0.25);}
      .btn-outline { background: transparent; color: var(--text-primary); border: 2px solid var(--border-col); padding: 12px; border-radius: 16px; width:100%; cursor: pointer; font-weight:bold; margin-top: 10px;}
      
      .bottom-nav { position: fixed; bottom: 20px; left: 50%; transform: translateX(-50%); width: 90%; max-width: 380px; background: var(--nav-bg); backdrop-filter: blur(20px); -webkit-backdrop-filter: blur(20px); border: 1px solid var(--border-col); display: flex; justify-content: space-around; padding: 8px 5px; box-shadow: 0 10px 30px rgba(0,0,0,0.15); z-index: 1000; border-radius: 100px;}
      .nav-item { flex: 1; text-align: center; color: var(--text-secondary); font-size: 11px; font-weight: 600; cursor: pointer; padding: 8px 0; border-radius: 50px; transition: 0.3s;}
      .nav-item.active { color: white; font-weight: 800; background: var(--btn-bg); box-shadow: 0 4px 10px rgba(186,26,26,0.3);}
      .nav-icon { font-size: 20px; display: block; margin-bottom: 2px;}
      
      .raw-data { font-size: 14px; background: var(--input-bg); padding: 15px; border-radius: 16px; margin-top: 15px; text-align: left; direction: ltr; font-family: monospace; color: var(--text-secondary);}
    </style>
  </head>
  <body data-theme="light">
    
    <div class="top-bar">
      <button class="icon-btn" onclick="toggleTheme()" id="themeBtn">🌙</button>
      <button class="icon-btn lang-btn" onclick="toggleLang()" id="langBtn">EN / فا</button>
    </div>

    <div id="tab-dash" class="tab-content active">
      <div class="card">
        <h2 data-en="💓 Live Dashboard" data-fa="💓 داشبورد زنده">💓 داشبورد زنده</h2>
        <p style="font-size:12px; color:var(--text-secondary);" data-en="Current Heart Rate" data-fa="ضربان قلب فعلی شما">ضربان قلب فعلی شما</p>
        <div class="bpm-display" id="liveBPM">--</div>
        <p style="font-weight:bold;" id="liveStatus" data-fa="در حال دریافت..." data-en="Loading...">در حال دریافت...</p>
      </div>
    </div>
    
    <div id="tab-set" class="tab-content">
      <div class="card">
        <h2 data-fa="⚙️ تنظیمات گجت" data-en="⚙️ Gadget Settings">⚙️ تنظیمات گجت</h2>
        <form action="/save" method="GET">
          
          <div class="category-box">
            <h3 data-fa="🌐 شبکه و ارتباط" data-en="🌐 Network & Comms">🌐 شبکه و ارتباط</h3>
            <label data-fa="موبایل اورژانس:" data-en="Emergency Mobile:">موبایل اورژانس:</label>
            <input type="text" name="phone" value="PHONE_VAL">
            <label data-fa="رمز جدید وای‌فای (حداقل 8 حرف):" data-en="WiFi Password (Min 8 Chars):">رمز جدید وای‌فای (حداقل 8 حرف):</label>
            <input type="text" name="wifi" value="WIFI_VAL" minlength="8">
          </div>

          <div class="category-box">
            <h3 data-fa="❤️ هشدار ضربان قلب" data-en="❤️ Heart Rate Alarms">❤️ هشدار ضربان قلب</h3>
            <p style="font-size:11px; margin-top:0; color:gray;" data-fa="(آلارم پس از 2 دقیقه پایداری فعال می‌شود)" data-en="(Alarm triggers after 2 mins stability)">(آلارم پس از 2 دقیقه پایداری فعال می‌شود)</p>
            <label data-fa="حداکثر ضربان (BPM):" data-en="Max Heart Rate (BPM):">حداکثر ضربان (BPM):</label>
            <input type="number" name="maxHR" value="MAX_VAL">
            <label data-fa="حداقل ضربان (BPM):" data-en="Min Heart Rate (BPM):">حداقل ضربان (BPM):</label>
            <input type="number" name="minHR" value="MIN_VAL">
          </div>

          <div class="category-box">
            <h3 data-fa="😰 تخصصی پنیک اتک" data-en="😰 Panic Attack Config">😰 تخصصی پنیک اتک</h3>
            <label data-fa="آستانه ضربان پنیک (BPM):" data-en="Panic HR Threshold (BPM):">آستانه ضربان پنیک (BPM):</label>
            <input type="number" name="pHR" value="PHR_VAL">
            <label data-fa="حساسیت بی‌تحرکی (زاویه):" data-en="Immobility Sens (Angle):">حساسیت بی‌تحرکی (زاویه):</label>
            <input type="number" name="panic" value="PANIC_VAL">
            <label data-fa="زمان تایید استرس (ثانیه):" data-en="Verification Time (Sec):">زمان تایید استرس (ثانیه):</label>
            <input type="number" name="pDur" value="PDUR_VAL">
          </div>

          <div class="category-box">
            <h3 data-fa="🤸 تشخیص سقوط" data-en="🤸 Fall Detection">🤸 تشخیص سقوط</h3>
            <label data-fa="حساسیت برخورد و شتاب:" data-en="Impact & Accel Sensitivity:">حساسیت برخورد و شتاب:</label>
            <input type="number" name="fall" value="FALL_VAL">
            <label data-fa="زمان طلایی لغو آلارم (ثانیه):" data-en="Alarm Cancel Time (Sec):">زمان طلایی لغو آلارم (ثانیه):</label>
            <input type="number" name="delay" value="DELAY_VAL">
          </div>

          <button type="submit" class="primary" data-fa="💾 ذخیره تنظیمات" data-en="💾 Save Settings">💾 ذخیره تنظیمات</button>
        </form>
      </div>
    </div>

    <div id="tab-sys" class="tab-content">
      <div class="card">
        <h2 data-fa="📊 عیب‌یابی و شبکه" data-en="📊 Diagnostics & Net">📊 عیب‌یابی و شبکه</h2>
        <div class="raw-data">
          BPM_Avg: <span id="rawBpm">0</span><br>
          Gyro_Pitch: <span id="rawPitch">0.0</span>°<br>
          Gyro_Roll: <span id="rawRoll">0.0</span>°<br>
          IR_Value: <span id="rawIr">0</span>
        </div>
        <p style="font-weight:bold; color:var(--text-secondary); font-size:13px;" id="simTestRes" data-fa="وضعیت آنتن: منتظر تست..." data-en="Signal: Waiting for test...">وضعیت آنتن: منتظر تست...</p>
        <button class="btn-outline" onclick="testSim()" data-fa="📡 بررسی قدرت آنتن شبکه" data-en="📡 Check Network Signal">📡 بررسی قدرت آنتن شبکه</button>
      </div>

      <div class="card">
        <h2 data-fa="🧪 شبیه‌سازی سخت‌افزاری" data-en="🧪 Hardware Sim">🧪 شبیه‌سازی سخت‌افزاری</h2>
        <button class="btn-outline" onclick="fetch('/sim_sms');" data-fa="✉️ تست پیامک" data-en="✉️ Test SMS">✉️ تست پیامک</button>
        <button class="btn-outline" onclick="fetch('/sim_fall');" data-fa="🤸 شبیه‌سازی سقوط" data-en="🤸 Simulate Fall">🤸 شبیه‌سازی سقوط</button>
        <button class="btn-outline" onclick="fetch('/sim_panic');" data-fa="😰 شبیه‌سازی پنیک اتک" data-en="😰 Simulate Panic">😰 شبیه‌سازی پنیک اتک</button>
      </div>
    </div>

    <div id="tab-info" class="tab-content">
      <div class="card" style="direction:ltr; text-align:center; padding: 40px 20px;">
        <h2 style="font-size:32px; margin-bottom:10px;">V28.0</h2>
        <span style="font-size:16px; font-weight:600; color:var(--text-secondary);">
          made with ❤️ by <span style="font-weight:800; color:var(--btn-bg);">Mio so...</span>
        </span>
      </div>
    </div>

    <div class="bottom-nav">
      <div class="nav-item active" onclick="openTab('tab-dash', this)">
        <span class="nav-icon">🩺</span><span data-fa="داشبورد" data-en="Dash">داشبورد</span>
      </div>
      <div class="nav-item" onclick="openTab('tab-set', this)">
        <span class="nav-icon">⚙️</span><span data-fa="تنظیمات" data-en="Settings">تنظیمات</span>
      </div>
      <div class="nav-item" onclick="openTab('tab-sys', this)">
        <span class="nav-icon">🛠️</span><span data-fa="سیستم" data-en="System">سیستم</span>
      </div>
      <div class="nav-item" onclick="openTab('tab-info', this)">
        <span class="nav-icon">ℹ️</span><span data-fa="درباره" data-en="About">درباره</span>
      </div>
    </div>

    <script>
      let isEn = false;
      function toggleLang() {
        isEn = !isEn;
        document.body.style.direction = isEn ? 'ltr' : 'rtl';
        document.body.style.textAlign = isEn ? 'left' : 'center';
        document.getElementById('langBtn').innerText = isEn ? "فا / EN" : "EN / فا";
        document.querySelectorAll('[data-fa]').forEach(el => {
          el.innerText = el.getAttribute(isEn ? 'data-en' : 'data-fa');
        });
      }

      function toggleTheme() {
        const body = document.body;
        const currentTheme = body.getAttribute('data-theme');
        if (currentTheme === 'light') {
          body.setAttribute('data-theme', 'dark');
          document.getElementById('themeBtn').innerText = '☀️';
        } else {
          body.setAttribute('data-theme', 'light');
          document.getElementById('themeBtn').innerText = '🌙';
        }
      }

      function openTab(tabId, el) {
        document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
        document.querySelectorAll('.nav-item').forEach(n => n.classList.remove('active'));
        document.getElementById(tabId).classList.add('active');
        el.classList.add('active');
      }

      function testSim() {
        document.getElementById('simTestRes').innerText = isEn ? "Connecting to cell tower..." : "در حال ارتباط با دکل مخابراتی...";
        fetch('/test_sim').then(r => r.text()).then(t => { document.getElementById('simTestRes').innerText = t; });
      }

      function updateData() {
        fetch('/api/data').then(r => r.json()).then(data => {
            document.getElementById('liveBPM').innerText = data.bpm > 0 ? data.bpm : "--";
            document.getElementById('rawBpm').innerText = data.bpm;
            document.getElementById('rawPitch').innerText = data.pitch;
            document.getElementById('rawRoll').innerText = data.roll;
            document.getElementById('rawIr').innerText = data.ir;
            
            let status = isEn ? "Normal 🟢" : "حالت عادی 🟢";
            if(data.ir < 10000) status = isEn ? "Sensor not on arm 🟡" : "سنسور روی بازو نیست 🟡";
            else if (data.alarm) status = isEn ? "⚠️ ALARM TRIGGERED!" : "⚠️ هشدار فعال است!";
            document.getElementById('liveStatus').innerText = status;
            
            setTimeout(updateData, 800); 
        }).catch(e => { setTimeout(updateData, 2000); });
      }
      updateData(); 
    </script>
  </body>
  </html>
  )rawliteral";

  html.replace("WIFI_VAL", wifiPass);
  html.replace("PHONE_VAL", phone1);
  html.replace("MAX_VAL", String(maxHR));
  html.replace("MIN_VAL", String(minHR));
  html.replace("FALL_VAL", String(fallSens));
  html.replace("PANIC_VAL", String(panicSens));
  html.replace("PHR_VAL", String(panicHR));
  html.replace("PDUR_VAL", String(panicDur));
  html.replace("DELAY_VAL", String(alarmDelay));

  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("phone")) {
    preferences.begin("medical_app", false);
    if(server.hasArg("wifi")) {
      String wp = server.arg("wifi");
      if(wp.length() >= 8) preferences.putString("wifiPass", wp);
    }
    preferences.putString("phone1", server.arg("phone"));
    preferences.putInt("maxHR", server.arg("maxHR").toInt());
    preferences.putInt("minHR", server.arg("minHR").toInt());
    preferences.putInt("fallSens", server.arg("fall").toInt());
    preferences.putInt("panicSens", server.arg("panic").toInt());
    preferences.putInt("panicHR", server.arg("pHR").toInt());
    preferences.putInt("panicDur", server.arg("pDur").toInt());
    preferences.putInt("alarmDelay", server.arg("delay").toInt());
    preferences.end();
    
    loadSettings(); 
    digitalWrite(BUZZER_PIN, BUZZER_ON); delay(150); digitalWrite(BUZZER_PIN, BUZZER_OFF); delay(100);
    digitalWrite(BUZZER_PIN, BUZZER_ON); delay(150); digitalWrite(BUZZER_PIN, BUZZER_OFF);

    server.send(200, "text/html", R"rawliteral(<!DOCTYPE HTML><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'><body style="font-family:Tahoma; text-align:center; direction:rtl; padding:40px; background:#e6f4ea; color:#137333;"><h2>✅ ذخیره شد / Saved!</h2><button onclick="window.location.href='/'" style="padding:15px 30px; border-radius:100px; border:none; background:#137333; color:white; font-size:16px;">بازگشت / Return</button></body></html>)rawliteral");
  }
}

void handleApiData() {
  String json = "{";
  json += "\"bpm\":" + String(beatAvg) + ",";
  json += "\"pitch\":" + String(pitch) + ",";
  json += "\"roll\":" + String(roll) + ",";
  json += "\"ir\":" + String(rawIR) + ",";
  json += "\"alarm\":" + String(isAlarmTriggered ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleTestSim() {
  while(sim800.available()) sim800.read(); 
  sim800.print("AT\r\n"); delay(100);
  sim800.print("AT+CSQ\r\n"); 
  long startT = millis();
  String res = "";
  while(millis() - startT < 2000) {
    if(sim800.available()) res += (char)sim800.read();
  }
  
  int csqIndex = res.indexOf("+CSQ: ");
  if(csqIndex >= 0) {
    int commaIndex = res.indexOf(",", csqIndex);
    if (commaIndex > csqIndex) {
      int rssi = res.substring(csqIndex + 6, commaIndex).toInt();
      String q = "";
      if (rssi == 99 || rssi == 0) q = "🔴 0/31 (No Network)";
      else if (rssi < 10) q = "🟠 " + String(rssi) + "/31 (Weak)";
      else if (rssi < 20) q = "🟡 " + String(rssi) + "/31 (Medium)";
      else q = "🟢 " + String(rssi) + "/31 (Excellent)";
      server.send(200, "text/plain", q);
      return;
    }
  }
  if (res.indexOf("OK") >= 0) server.send(200, "text/plain", "🟡 Sim Connected but NO Signal.");
  else server.send(200, "text/plain", "❌ Error: Module not responding.");
}

void handleSimSms() { sendSMS(phone1, "TEST SMS from Heart Monitor"); server.send(200, "text/plain", "OK"); }
void handleSimFall() { isAlarmTriggered = true; alarmStartTime = millis(); currentEmergencyReason = "Fall Detected (Sim)"; smsReason = "Fall"; server.send(200, "text/plain", "OK"); }
void handleSimPanic() { isAlarmTriggered = true; alarmStartTime = millis(); currentEmergencyReason = "Panic Attack (Sim)"; smsReason = "Panic_Attack"; server.send(200, "text/plain", "OK"); }

// ==========================================
// 5. تابع ژیروسکوپ
// ==========================================
void readGyroData() {
  while (gyroSerial.available()) {
    re_buf[counter] = (unsigned char)gyroSerial.read();
    if (counter == 0 && re_buf[0] != 0xAA) continue; 
    counter++;
    if (counter == 8) {
      counter = 0; 
      if (re_buf[7] == 0x55) {
        yaw   = (int16_t)(re_buf[1] << 8 | re_buf[2]) / 100.0;
        pitch = (int16_t)(re_buf[3] << 8 | re_buf[4]) / 100.0;
        roll  = (int16_t)(re_buf[5] << 8 | re_buf[6]) / 100.0;
        
        if (isFirstAngleRead) {
          lastPitch = pitch;
          lastRoll = roll;
          isFirstAngleRead = false;
        }
      }
    }
  }
}

// ==========================================
// 6. Setup
// ==========================================
void setup() {
  pinMode(CANCEL_BTN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VIBRATOR_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, BUZZER_OFF); digitalWrite(VIBRATOR_PIN, LOW);

  loadSettings();

  delay(100); 
  if (digitalRead(CANCEL_BTN_PIN) == LOW) {
    isConfigMode = true;
    for(int i=0; i<3; i++) {
      digitalWrite(BUZZER_PIN, BUZZER_ON); delay(100); 
      digitalWrite(BUZZER_PIN, BUZZER_OFF); delay(100);
    }
  } else {
    isConfigMode = false;
    digitalWrite(BUZZER_PIN, BUZZER_ON); delay(500); 
    digitalWrite(BUZZER_PIN, BUZZER_OFF);
  }

  gyroSerial.setRxBufferSize(1024);
  gyroSerial.begin(115200, SERIAL_8N1, GYRO_RX_PIN, GYRO_TX_PIN);
  gyroSerial.write(0xA5); gyroSerial.write(0x52); 
  
  sim800.begin(115200, SERIAL_8N1, SIM800_RX_PIN, SIM800_TX_PIN);

  Wire.begin(8, 9);
  if (particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    byte ledBrightness = 80;   
    byte sampleAverage = 4;    
    byte ledMode = 2;          
    int sampleRate = 400;      
    int pulseWidth = 411;      
    int adcRange = 8192;       

    particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
    particleSensor.setPulseAmplitudeRed(80); 
    particleSensor.setPulseAmplitudeIR(80); 
    particleSensor.setPulseAmplitudeGreen(0); 
  }

  if (isConfigMode == true) {
    WiFi.mode(WIFI_AP);
    WiFi.setTxPower(WIFI_POWER_8_5dBm); 
    
    // 🌟 1. تغییر آی‌پی گجت به 8.8.8.8 (آی‌پی سرور گوگل)
    IPAddress apIP(8, 8, 8, 8);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    
    WiFi.softAP("Heart_Monitor", wifiPass.c_str()); 
    
    // 🌟 2. استارت DNS سرور روی آی‌پی جدید
    dnsServer.start(DNS_PORT, "*", apIP);
    
    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.on("/api/data", handleApiData);
    server.on("/test_sim", handleTestSim);
    server.on("/sim_sms", handleSimSms);
    server.on("/sim_fall", handleSimFall);
    server.on("/sim_panic", handleSimPanic);
    
    server.onNotFound([]() {
      server.sendHeader("Location", String("http://8.8.8.8"), true);
      server.send(302, "text/plain", "");
    });
    
    server.begin();
  }
}

// ==========================================
// 7. Loop
// ==========================================
void loop() {
  if (isConfigMode) {
    dnsServer.processNextRequest(); 
    server.handleClient(); 
  }

  readGyroData();

  rawIR = particleSensor.getIR();
  if (rawIR < 10000) {
    beatsPerMinute = 0; beatAvg = 0; rateSpot = 0;
    for(byte i = 0 ; i < RATE_SIZE ; i++) rates[i] = 0;
    isHrAbnormal = false; 
  } else if (checkForBeat(rawIR) == true) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);
    
    if (beatsPerMinute < 160 && beatsPerMinute > 40) {
      if (beatAvg == 0 || abs((int)beatsPerMinute - beatAvg) < 30) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;
        beatAvg = 0; byte validSamples = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++) { if (rates[x] > 0) { beatAvg += rates[x]; validSamples++; } }
        if (validSamples > 0) beatAvg /= validSamples;
      }
    }
  }

  readGyroData();

  bool currentBtnState = digitalRead(CANCEL_BTN_PIN);
  if (currentBtnState == LOW && lastBtnState == HIGH) {
    delay(30); 
    if (digitalRead(CANCEL_BTN_PIN) == LOW) {
      if (isAlarmTriggered) {
        isAlarmTriggered = false; 
        inCooldown = true; cooldownStartTime = millis();
        digitalWrite(BUZZER_PIN, BUZZER_OFF); digitalWrite(VIBRATOR_PIN, LOW);
        sendSMS(phone1, "ALERT CANCELED!\nEvent: " + smsReason + "\nStatus: Patient pressed the button. I am OK.");
        btnClickCount = 0; fallState = 0; possiblePanic = false; isHrAbnormal = false;
      } else {
        if (millis() - lastBtnClickTime > 2000) btnClickCount = 1; else btnClickCount++;
        lastBtnClickTime = millis();
        digitalWrite(BUZZER_PIN, BUZZER_ON); delay(40); digitalWrite(BUZZER_PIN, BUZZER_OFF);
        
        if (btnClickCount >= 5) {
          digitalWrite(BUZZER_PIN, BUZZER_ON); digitalWrite(VIBRATOR_PIN, HIGH);
          sendSMS(phone1, "SOS EMERGENCY!\nEvent: Manual Help Request\nStatus: Patient pressed the help button 5 times!");
          digitalWrite(BUZZER_PIN, BUZZER_OFF); digitalWrite(VIBRATOR_PIN, LOW);
          btnClickCount = 0; inCooldown = true; cooldownStartTime = millis();
        }
      }
    }
  }
  lastBtnState = currentBtnState;

  bool dangerDetected = false;
  String reason = "", sReason = "";

  if (millis() > 8000) {
    
    if (beatAvg > 0) {
      if (beatAvg >= panicHR && abs(pitch) < panicSens && abs(roll) < panicSens) {
        if (!possiblePanic) { possiblePanic = true; panicTimer = millis(); } 
        else if (millis() - panicTimer > (panicDur * 1000)) {
          dangerDetected = true; reason = "Panic Attack"; sReason = "Panic_Attack"; possiblePanic = false;
        }
      } else {
        possiblePanic = false; 
        if (beatAvg > maxHR) {
          if (!isHrAbnormal) { isHrAbnormal = true; hrAbnormalTimer = millis(); hrReason = "High Heart Rate"; hrSReason = "High_HR"; } 
          else if (millis() - hrAbnormalTimer > 120000) { dangerDetected = true; reason = hrReason; sReason = hrSReason; isHrAbnormal = false; }
        } else if (beatAvg < minHR) {
          if (!isHrAbnormal) { isHrAbnormal = true; hrAbnormalTimer = millis(); hrReason = "Low Heart Rate"; hrSReason = "Low_HR"; } 
          else if (millis() - hrAbnormalTimer > 120000) { dangerDetected = true; reason = hrReason; sReason = hrSReason; isHrAbnormal = false; }
        } else { isHrAbnormal = false; }
      }
    }

    if (millis() - lastAngleCheckTime > 200) {
      float deltaPitch = abs(pitch - lastPitch);
      if (deltaPitch > 180) deltaPitch = 360 - deltaPitch; 
      float deltaRoll = abs(roll - lastRoll);
      if (deltaRoll > 180) deltaRoll = 360 - deltaRoll;

      if (fallState == 0) {
        if (deltaPitch >= fallSens || deltaRoll >= fallSens) { fallState = 1; fallTimer = millis(); }
      } else if (fallState == 1) {
        if (deltaPitch >= 12 || deltaRoll >= 12) fallState = 0; 
        else if (millis() - fallTimer > 6000) { dangerDetected = true; reason = "Fall Detected (Unconscious)"; sReason = "Fall"; fallState = 0; }
      }
      lastPitch = pitch; lastRoll = roll; lastAngleCheckTime = millis();
    }
  } else {
    lastPitch = pitch; lastRoll = roll; lastAngleCheckTime = millis();
  }

  if (inCooldown) {
    if (millis() - cooldownStartTime > 60000) inCooldown = false; 
  }

  if (dangerDetected && !isAlarmTriggered && !inCooldown) {
    isAlarmTriggered = true; alarmStartTime = millis();
    currentEmergencyReason = reason; smsReason = sReason;
    fallState = 0; possiblePanic = false; isHrAbnormal = false;
  }

  if (isAlarmTriggered) {
    if (millis() % 1000 < 500) { digitalWrite(BUZZER_PIN, BUZZER_ON); digitalWrite(VIBRATOR_PIN, HIGH); } 
    else { digitalWrite(BUZZER_PIN, BUZZER_OFF); digitalWrite(VIBRATOR_PIN, LOW); }

    if ((millis() - alarmStartTime) > (alarmDelay * 1000)) {
      digitalWrite(BUZZER_PIN, BUZZER_OFF); digitalWrite(VIBRATOR_PIN, LOW);
      sendSMS(phone1, "EMERGENCY!\nEvent: " + smsReason + "\nStatus: NO RESPONSE from patient!");
      isAlarmTriggered = false; inCooldown = true; cooldownStartTime = millis();
    }
  }
}