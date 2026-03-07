#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 8
#define SCL_PIN 9
#define BTN_PIN 1 

// --- CẤU HÌNH SERVO MỚI ---
#define SERVO_PIN 5
#define PWM_FREQ  50  
#define PWM_RES   12  
// ---------------------------

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum State { IDLE, DROWSY, SLEEP, CURIOUS, EMOTION };
State robotState = IDLE;

unsigned long lastActivityTime = 0, stateTimer = 0;
int emotionType = 0; 
float currentEyeOffset = 0; 
int targetEyeOffset = 0;    
float lerpSpeed = 0.25; 

bool lastBtnState = HIGH;
bool isSmiling = false; 

struct Particle { float x, y, speed; };
Particle tears[2], hearts[6], sleepZ[6]; 

// Hàm bổ trợ quay Servo chuẩn ESP32 mới
void moveServo(int angle) {
  // Map 0-180 độ sang duty cycle cho 12-bit (khoảng 102 đến 512)
  int duty = map(angle, 0, 180, 102, 512); 
  ledcWrite(SERVO_PIN, duty);
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(BTN_PIN, INPUT_PULLUP);
  
  // --- KHỞI TẠO PWM KIỂU MỚI (ESP32 v3.0+) ---
  ledcAttach(SERVO_PIN, PWM_FREQ, PWM_RES);
  moveServo(90); 
  // ------------------------------------------

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(15, 15);
  display.print("ROBOT tuong tac");
  display.setTextSize(2);
  display.setCursor(20, 35);
  display.print("CAM XUC");
  display.display();
  delay(3000); 

  for(int i=0; i<2; i++) tears[i] = { (float)(35 + i*36), 52.0, (float)(random(10, 25)/10.0) };
  for(int i=0; i<6; i++) { 
    hearts[i] = { (float)random(10, 118), (float)random(-64, 0), (float)(random(15, 30)/10.0) }; 
    sleepZ[i] = { (float)random(10, 110), (float)random(-100, 0), (float)(random(8, 15)/10.0) };
  }
  lastActivityTime = millis();
}

void loop() {
  checkTouch();
  updateFSM();
  
  if (abs(targetEyeOffset - currentEyeOffset) > 0.05) {
    currentEyeOffset += (targetEyeOffset - currentEyeOffset) * lerpSpeed;
  }

  // Cập nhật Servo
  int angle = map((int)currentEyeOffset, -15, 15, 75, 105);
  if (robotState == SLEEP) moveServo(90);
  else moveServo(angle);

  renderRobot();
}

// --- GIỮ NGUYÊN TOÀN BỘ CÁC HÀM VẼ (renderRobot, drawEmotions, ...) CỦA BẠN DƯỚI ĐÂY ---

void checkTouch() {
  bool currentBtnState = digitalRead(BTN_PIN);
  if (currentBtnState == LOW && lastBtnState == HIGH) {
    delay(50); 
    if (robotState == SLEEP || robotState == DROWSY) {
      robotState = CURIOUS; stateTimer = millis();
    } else {
      robotState = EMOTION;
      emotionType = (emotionType + 1) % 4;
      stateTimer = millis();
      if(emotionType == 3) for(int i=0; i<6; i++) hearts[i].y = random(-64, 0);
    }
    lastActivityTime = millis();
  }
  lastBtnState = currentBtnState;
}

void updateFSM() {
  unsigned long inactiveTime = millis() - lastActivityTime;
  if (robotState != EMOTION && robotState != CURIOUS) {
    if (inactiveTime > 20000) { robotState = CURIOUS; stateTimer = millis(); }
    else if (inactiveTime > 14000) robotState = SLEEP;
    else if (inactiveTime > 10000) robotState = DROWSY;
    else robotState = IDLE;
  }
  if ((robotState == EMOTION || robotState == CURIOUS) && (millis() - stateTimer > 6000)) { 
    robotState = IDLE; lastActivityTime = millis(); 
  }
}

void renderRobot() {
  display.clearDisplay();
  switch (robotState) {
    case IDLE:
      if (millis() % 1500 < 20) {
        targetEyeOffset = random(-15, 16);
        isSmiling = (random(0, 10) < 3); 
      }
      drawOriginalEyes((int)currentEyeOffset); 
      break;
    case DROWSY: drawDrowsyEyes(); break;
    case SLEEP: drawSleepEyes(); break;
    case CURIOUS: { 
      int curiousDir = (millis() % 600 < 300) ? -15 : 15;
      targetEyeOffset = curiousDir;
      if (millis() % 400 < 80) { 
        display.fillRect(42 + curiousDir - 10, 32, 20, 4, WHITE);
        display.fillRect(86 + curiousDir - 10, 32, 20, 4, WHITE);
      } else drawCuriousEyes(curiousDir);
      break;
    }
    case EMOTION: targetEyeOffset = 0; drawEmotions(); break;
  }
  display.display();
}

void drawOriginalEyes(int offset) {
  if (millis() % 4000 < 150) { 
    display.fillRect(35+offset, 32, 22, 4, WHITE); 
    display.fillRect(71+offset, 32, 22, 4, WHITE);
  } else {
    display.fillRoundRect(35+offset, 22, 22, 28, 10, WHITE); 
    display.fillRoundRect(71+offset, 22, 22, 28, 10, WHITE);
    if (isSmiling) {
      display.fillRect(35+offset, 38, 22, 12, BLACK); 
      display.fillRect(71+offset, 38, 22, 12, BLACK);
    }
  }
}

void drawCuriousEyes(int offset) {
  display.fillCircle(42 + offset, 32, 14, WHITE); 
  display.fillCircle(86 + offset, 32, 14, WHITE);
  display.fillCircle(42 + offset, 32, 3, BLACK); 
  display.fillCircle(86 + offset, 32, 3, BLACK);
}

void drawSleepEyes() {
  display.drawFastHLine(35, 35, 22, WHITE); 
  display.drawFastHLine(71, 35, 22, WHITE);
  display.setTextSize(1);
  for(int i=0; i<6; i++) {
    display.setCursor(sleepZ[i].x, (int)sleepZ[i].y);
    display.print("z");
    sleepZ[i].y += sleepZ[i].speed;
    if(sleepZ[i].y > 64) { sleepZ[i].y = -10; sleepZ[i].x = random(5, 120); }
  }
  display.drawCircle(64, 45, 2, WHITE);
  int d = (millis() / 200) % 12;
  display.fillCircle(64, 48 + d, 1, WHITE);
}

void drawDrowsyEyes() {
  if (millis() % 600 < 100) { 
    display.fillRect(35, 32, 22, 4, WHITE); 
    display.fillRect(71, 32, 22, 4, WHITE);
  } else {
    display.fillRoundRect(35, 30, 22, 8, 3, WHITE); 
    display.fillRoundRect(71, 30, 22, 8, 3, WHITE);
  }
  display.drawCircle(64, 50, 4, WHITE);
}

void drawHeart(int x, int y, int size) {
  display.fillCircle(x - size/2, y, size/2, WHITE);
  display.fillCircle(x + size/2, y, size/2, WHITE);
  display.fillTriangle(x - size, y + size/4, x + size, y + size/4, x, y + size, WHITE);
}

void drawEmotions() {
  switch (emotionType) {
    case 0: { // VUI
      int bounce = (millis() % 800 < 400) ? 0 : -3; 
      display.drawCircleHelper(45, 38 + bounce, 18, 1, WHITE); 
      display.drawCircleHelper(83, 38 + bounce, 18, 1, WHITE);
      display.fillRoundRect(54, 52 + bounce, 20, 8, 4, WHITE);
      break;
    }
    case 1: { // GIẬN
      display.fillTriangle(20, 15, 60, 40, 20, 45, WHITE); 
      display.fillTriangle(108, 15, 68, 40, 108, 45, WHITE);
      display.fillRect(44, 55, 40, 3, WHITE); 
      break;
    }
    case 2: { // BUỒN
      display.fillRoundRect(35, 30, 22, 12, 4, WHITE); 
      display.fillRoundRect(71, 30, 22, 12, 4, WHITE);
      for(int i=0; i<2; i++) {
        display.fillCircle(tears[i].x, tears[i].y, 2, WHITE);
        tears[i].y += tears[i].speed;
        if(tears[i].y > 64) tears[i].y = 45;
      }
      break;
    }
    case 3: { // YÊU
      unsigned long elapsed = millis() - stateTimer;
      if (elapsed < 3000) { 
        drawHeart(42, 35, 8); drawHeart(86, 35, 8);
      } else { 
        int heartSize = (elapsed - 3000) / 40; 
        if (heartSize > 70) heartSize = 70;
        drawHeart(64, 32 - (heartSize/4), heartSize);
      }
      break;
    }
  }
}