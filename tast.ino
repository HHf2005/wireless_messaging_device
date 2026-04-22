/*******************************************************************************
 * 🚀 مشروع جهاز مراسلة لاسلكي - الإصدار V8.1 (مصحح)
 *******************************************************************************/

#include <SPI.h>              
#include <Wire.h>             
#include <Adafruit_GFX.h>     
#include <Adafruit_ILI9341.h> 
#include <U8g2_for_Adafruit_GFX.h> 
#include <LoRa.h>             
#include <Adafruit_AW9523.h>  
#include <Preferences.h>      

// -------------------------------------------------------------
// 🎨 تعريف الألوان
// -------------------------------------------------------------
#define ILI9341_GRAY      0x8410 
#define ILI9341_DARKBLUE  0x0010 
#define ILI9341_DARKGREEN 0x03E0 
#define ILI9341_BLACK     0x0000
#define ILI9341_WHITE     0xFFFF
#define ILI9341_RED       0xF800
#define ILI9341_GREEN     0x07E0
#define ILI9341_BLUE      0x001F
#define ILI9341_CYAN      0x07FF
#define ILI9341_MAGENTA   0xF81F
#define ILI9341_YELLOW    0xFFE0

// -------------------------------------------------------------
// 📘 إعداد الشاشة والخطوط
// -------------------------------------------------------------
#define TFT_CS   15    
#define TFT_DC   4     
#define TFT_RST  2     

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST); 
U8g2_for_Adafruit_GFX u8g2; 

// -------------------------------------------------------------
// 📘 إعداد LoRa
// -------------------------------------------------------------
#define LORA_SCK   18  
#define LORA_MISO  19  
#define LORA_MOSI  23  
#define LORA_CS    5   
#define LORA_RST   17  
#define LORA_DIO0  16  

#define BUZZER_PIN 33  
#define VIB_PIN    32  

// -------------------------------------------------------------
// 📘 إعداد الكيبورد
// -------------------------------------------------------------
Adafruit_AW9523 aw;
const byte ROWS = 5;
const byte COLS = 10;
uint8_t rowPins[ROWS] = {0, 1, 2, 3, 4}; 
uint8_t colPins[COLS] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14}; 

char keys[ROWS][COLS] = { 
  {'>','<','*','&','^','%','$','#','@','!'}, 
  {'/','.','k','M','N','B','V','C','X','Z'}, 
  {'-','L','K','J','H','G','F','D','S','A'},
  {'P','O','I','U','Y','T','R','E','W','Q'},
  {'0','9','8','7','6','5','4','3','2','1'}  
};

bool keyState[ROWS][COLS] = {false}; 

// -------------------------------------------------------------
// ⚙️ المتغيرات
// -------------------------------------------------------------
String inputText = ""; 
int cursorPosition = 0; 
const int MAX_INPUT_LENGTH = 30; 
bool isArabicMode = false; 

#define CHAT_MODE 0       
#define SETTINGS_MODE 1   
int currentMode = CHAT_MODE; 

bool isSoundEnabled = true;     
bool isVibrationEnabled = true; 

String accessCode = "000";         
const int MAX_CODE_LENGTH = 3;     
bool isEditingCode = false;        
String tempCode = "";              
int selectedSetting = 1; 

#define MAX_MESSAGES 50         
const int MESSAGES_ON_SCREEN = 6; 
String chatLog[MAX_MESSAGES];   
int messageCount = 0;           
int scrollOffset = 0;           

bool cursorVisible = true;                   
unsigned long lastCursorToggle = 0;          
const unsigned long CURSOR_INTERVAL = 400;   

Preferences preferences;

// إعلان أولي للدوال لحل مشاكل الترتيب
void drawChatUI();
void drawSettingsUI();

// -------------------------------------------------------------
// 📝 دالة التعريب
// -------------------------------------------------------------
String getArabicChar(char key) {
    switch (key) {
        case 'q': return "ض"; case 'w': return "ص"; case 'e': return "ث"; case 'r': return "ق"; case 't': return "ف";
        case 'y': return "غ"; case 'u': return "ع"; case 'i': return "ه"; case 'o': return "خ"; case 'p': return "ح";
        case 'a': return "ش"; case 's': return "س"; case 'd': return "ي"; case 'f': return "ب"; case 'g': return "ل";
        case 'h': return "ا"; case 'j': return "ت"; case 'k': return "ن"; case 'l': return "م";
        case 'z': return "ئ"; case 'x': return "ء"; case 'c': return "ؤ"; case 'v': return "ر"; 
        case 'b': return "لا"; case 'n': return "ى"; case 'm': return "ة";
        case 'Q': return "ض"; case 'W': return "ص"; case 'E': return "ث"; case 'R': return "ق"; case 'T': return "ف";
        case 'Y': return "غ"; case 'U': return "ع"; case 'I': return "ه"; case 'O': return "خ"; case 'P': return "ح";
        case 'A': return "ش"; case 'S': return "س"; case 'D': return "ي"; case 'F': return "ب"; case 'G': return "ل";
        case 'H': return "أ"; case 'J': return "ت"; case 'K': return "ن"; case 'L': return "م";
        case 'Z': return "ئ"; case 'X': return "ء"; case 'C': return "ؤ"; case 'V': return "ر"; 
        case 'B': return "لا"; case 'N': return "آ"; case 'M': return "ة";
        case ',': return "و"; case '.': return "ز"; case '/': return "ظ"; case ';': return "ك"; case '\'': return "ط";
        default: return String(key); 
    }
}

// -------------------------------------------------------------
// 💾 النظام
// -------------------------------------------------------------
void saveSettings() {
  preferences.begin("messenger", false); 
  preferences.putString("code", accessCode);       
  preferences.putBool("sound", isSoundEnabled);    
  preferences.putBool("vibrate", isVibrationEnabled); 
  preferences.end(); 
}

void loadSettings() {
  preferences.begin("messenger", true); 
  accessCode = preferences.getString("code", "000"); 
  isSoundEnabled = preferences.getBool("sound", true); 
  isVibrationEnabled = preferences.getBool("vibrate", true); 
  preferences.end();
}

void playTone() {
  if (isSoundEnabled) digitalWrite(BUZZER_PIN, HIGH); 
  if (isVibrationEnabled) digitalWrite(VIB_PIN, HIGH); 
  delay(100); 
  digitalWrite(BUZZER_PIN, LOW); 
  digitalWrite(VIB_PIN, LOW); 
}

// -------------------------------------------------------------
// 🎨 الرسم
// -------------------------------------------------------------
// 🔥 تم التعديل: إضافة قيمة افتراضية لـ y لتجنب الخطأ
void showNotification(String text, uint16_t color, int y = 220) {
  tft.fillRect(0, y - 5, 320, 25, ILI9341_BLACK); 
  u8g2.setForegroundColor(color);
  u8g2.setCursor(10, y + 15);
  u8g2.print(text);
}

void drawChatLog() {
  tft.fillRect(0, 10, 320, tft.height() - 60, ILI9341_BLACK); 
  int currentY = 30; 
  int endIndex = min(messageCount, scrollOffset + MESSAGES_ON_SCREEN); 
  
  for (int i = scrollOffset; i < endIndex; i++) {
    if (chatLog[i].startsWith("You:")) u8g2.setForegroundColor(ILI9341_YELLOW);
    else u8g2.setForegroundColor(ILI9341_GREEN);
    
    u8g2.setCursor(5, currentY); 
    u8g2.print(chatLog[i]);
    currentY += 25; 
  }
}

void drawInputText() {
    int TEXT_Y = tft.height() - 25; 
    tft.fillRect(15, TEXT_Y - 20, 300, 25, ILI9341_GRAY); 
    
    u8g2.setCursor(20, TEXT_Y); 
    u8g2.setForegroundColor(ILI9341_WHITE);
    
    String displayStr = inputText;
    if (cursorVisible) displayStr += "_";
    
    tft.fillRect(280, TEXT_Y - 15, 30, 20, isArabicMode ? ILI9341_GREEN : ILI9341_BLUE);
    tft.setCursor(285, TEXT_Y - 12);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.print(isArabicMode ? "AR" : "EN");

    u8g2.print(displayStr);
}

void drawSettingItem(int y, const char* title, const char* value, bool isActive, bool isSelected) {
    uint16_t bgColor = isSelected ? ILI9341_DARKBLUE : ILI9341_BLACK;
    uint16_t borderColor = isSelected ? ILI9341_WHITE : ILI9341_GRAY;
    uint16_t valueColor = isActive ? ILI9341_GREEN : ILI9341_RED;

    tft.fillRect(5, y, 310, 35, bgColor); 
    tft.drawRect(5, y, 310, 35, borderColor); 
    
    u8g2.setForegroundColor(ILI9341_CYAN);
    u8g2.setCursor(15, y + 25);
    u8g2.print(title);
    
    u8g2.setForegroundColor(valueColor);
    u8g2.setCursor(200, y + 25);
    u8g2.print(value);
    
    if (isSelected) tft.drawRect(3, y - 2, 314, 39, ILI9341_YELLOW); 
}

void drawSettingsUI() {
    tft.fillScreen(ILI9341_BLACK);
    u8g2.setForegroundColor(ILI9341_YELLOW);
    u8g2.setCursor(80, 30);
    u8g2.print("SETTINGS"); 
    tft.drawFastHLine(10, 45, 300, ILI9341_GRAY);
    
    int y1 = 60;
    uint16_t codeBgColor = (selectedSetting == 1) ? ILI9341_DARKBLUE : ILI9341_BLACK;
    tft.fillRect(5, y1, 310, 35, codeBgColor);
    tft.drawRect(5, y1, 310, 35, (selectedSetting == 1) ? ILI9341_WHITE : ILI9341_GRAY);
    if (selectedSetting == 1) tft.drawRect(3, y1 - 2, 314, 39, ILI9341_YELLOW);
    
    u8g2.setForegroundColor(ILI9341_CYAN);
    u8g2.setCursor(15, y1 + 25);
    u8g2.print("1. Code: ");
    u8g2.setForegroundColor(ILI9341_WHITE);
    u8g2.print(isEditingCode ? tempCode : accessCode);

    drawSettingItem(110, "2. Sound:", isSoundEnabled ? "ON" : "OFF", isSoundEnabled, selectedSetting == 2);
    drawSettingItem(160, "3. Vibrate:", isVibrationEnabled ? "ON" : "OFF", isVibrationEnabled, selectedSetting == 3);
    
    tft.drawFastHLine(10, 205, 300, ILI9341_GRAY); 
    u8g2.setForegroundColor(ILI9341_WHITE);
    u8g2.setCursor(10, 230);
    u8g2.print("^ Back to Chat");
}

void drawChatUI() {
    tft.fillScreen(ILI9341_BLACK); 
    int INPUT_AREA_H = 45; 
    int INPUT_AREA_Y_START = tft.height() - INPUT_AREA_H; 
    tft.drawFastHLine(0, INPUT_AREA_Y_START - 5, 320, ILI9341_BLUE); 
    tft.fillRoundRect(5, INPUT_AREA_Y_START + 5, 310, INPUT_AREA_H - 10, 8, ILI9341_GRAY); 
    drawInputText(); 
    drawChatLog();   
}

void addToChatLog(String message, bool isSent) {
  String formattedMessage = isSent ? "You: " + message : "Friend: " + message;
  if (messageCount < MAX_MESSAGES) {
      chatLog[messageCount++] = formattedMessage;
  } else {
      for (int i = 0; i < MAX_MESSAGES - 1; i++) chatLog[i] = chatLog[i + 1]; 
      chatLog[MAX_MESSAGES - 1] = formattedMessage; 
  }
  if (messageCount > MESSAGES_ON_SCREEN) scrollOffset = messageCount - MESSAGES_ON_SCREEN; 
  else scrollOffset = 0; 
  if (currentMode == CHAT_MODE) drawChatLog(); 
}

// -------------------------------------------------------------
// ⌨️ المعالجة
// -------------------------------------------------------------
void handleSettingsMode(char key) {
    if (isEditingCode) {
        if (key >= '0' && key <= '9' && tempCode.length() < MAX_CODE_LENGTH) tempCode += key;
        else if (key == '*') { if (tempCode.length() > 0) tempCode.remove(tempCode.length() - 1); } 
        else if (key == '#') { 
            if (tempCode.length() == MAX_CODE_LENGTH) {
                accessCode = tempCode; saveSettings(); isEditingCode = false; 
                showNotification("Saved!", ILI9341_GREEN); drawSettingsUI(); return;
            } else { showNotification("Must be 3!", ILI9341_RED); return; }
        } 
        drawSettingsUI();
        return; 
    }
    
    if (key == '1') selectedSetting = 1;
    else if (key == '2') selectedSetting = 2;
    else if (key == '3') selectedSetting = 3;
    
    if (key == '#') { 
        if (selectedSetting == 1) { 
            isEditingCode = true; tempCode = accessCode; 
            showNotification("Enter Code..", ILI9341_YELLOW); drawSettingsUI(); 
        } 
        else if (selectedSetting == 2) { isSoundEnabled = !isSoundEnabled; saveSettings(); drawSettingsUI(); } 
        else if (selectedSetting == 3) { isVibrationEnabled = !isVibrationEnabled; saveSettings(); drawSettingsUI(); }
    }
    
    if (key == '1' || key == '2' || key == '3') drawSettingsUI(); 
}

void handleChatMode(char key) {
  if (key == '!') { if (scrollOffset > 0) { scrollOffset--; drawChatLog(); } }
  else if (key == '@') { 
      int maxScroll = messageCount - MESSAGES_ON_SCREEN;
      if (maxScroll < 0) maxScroll = 0; 
      if (scrollOffset < maxScroll) { scrollOffset++; drawChatLog(); } 
  }
  else if (key == '%') { inputText = ""; cursorPosition = 0; drawInputText(); }
  else if (key == '*') { 
    if (inputText.length() > 0) {
       int len = inputText.length();
       unsigned char lastChar = inputText.charAt(len-1);
       if (lastChar >= 0x80) { inputText = inputText.substring(0, len - 2); } 
       else { inputText = inputText.substring(0, len - 1); }
       drawInputText();
    }
  }
  else if (key == '&') {
      isArabicMode = !isArabicMode; 
      playTone(); 
      drawInputText();
  }
  else if (key == '#') { 
    if (inputText.length() > 0) {
      LoRa.beginPacket(); LoRa.print(inputText); LoRa.endPacket();
      addToChatLog(inputText, true); 
      inputText = ""; drawInputText();
    }
  }
  else {
      if (key != '>' && key != '<' && key != '^') {
          if (inputText.length() < MAX_INPUT_LENGTH * 2) { 
              if (isArabicMode) inputText += getArabicChar(key); 
              else inputText += key; 
              drawInputText();
          }
      }
  }
}

// -------------------------------------------------------------
// 🚀 التشغيل
// -------------------------------------------------------------
void setup() {
  Serial.begin(115200); 
  Wire.begin(); 
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW); 
  pinMode(VIB_PIN, OUTPUT); digitalWrite(VIB_PIN, LOW); 
  loadSettings(); 
  
  if (!aw.begin(0x58)) { Serial.println("AW9523R Fail!"); while (1); }
  for (int r = 0; r < ROWS; r++) { aw.pinMode(rowPins[r], OUTPUT); aw.digitalWrite(rowPins[r], HIGH); } 
  for (int c = 0; c < COLS; c++) { aw.pinMode(colPins[c], INPUT_PULLUP); } 

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  tft.begin();
  tft.setRotation(3); 
  tft.fillScreen(ILI9341_BLACK);

  u8g2.begin(tft);                 
  u8g2.setFont(u8g2_font_cu12_t_arabic); 
  u8g2.setFontMode(1);                 
  u8g2.setForegroundColor(ILI9341_WHITE);

  if (LoRa.begin(433E6)) { 
      u8g2.setCursor(10, 30); u8g2.print("System Booting...");
      u8g2.setCursor(10, 60); u8g2.print("LoRa OK!"); 
  } else { 
      tft.println("LoRa Failed!"); while (1); 
  } 

  delay(1000);
  drawChatUI(); 
}

void loop() {
  if (currentMode == CHAT_MODE) {
      if (millis() - lastCursorToggle >= CURSOR_INTERVAL) {
        lastCursorToggle = millis(); cursorVisible = !cursorVisible; drawInputText(); 
      }
  }

  char key = '\0'; 
  for (int r = 0; r < ROWS; r++) {
    aw.digitalWrite(rowPins[r], LOW); 
    delayMicroseconds(50); 
    for (int c = 0; c < COLS; c++) {
      if (aw.digitalRead(colPins[c]) == LOW) {
        if (!keyState[r][c]) {
          keyState[r][c] = true; key = keys[r][c]; 
          cursorVisible = true; lastCursorToggle = millis();
        }
      } else { keyState[r][c] = false; }
    }
    aw.digitalWrite(rowPins[r], HIGH); 
    if (key != '\0') break;
  }
  
  if (key == '^') { 
      if (isEditingCode) return; 
      currentMode = (currentMode == CHAT_MODE) ? SETTINGS_MODE : CHAT_MODE;
      if (currentMode == SETTINGS_MODE) drawSettingsUI(); else drawChatUI();
      return; 
  }
  
  if (key != '\0') {
      if (currentMode == CHAT_MODE) handleChatMode(key);
      else handleSettingsMode(key);
  }
  
  int packetSize = LoRa.parsePacket(); 
  if (packetSize) {
    String received = "";
    while (LoRa.available()) received += (char)LoRa.read();
    addToChatLog(received, false); 
    playTone(); 
  }
}