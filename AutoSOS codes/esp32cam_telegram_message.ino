#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "Chotraju";
const char* password = "rhemanth123";

// Telegram BOT Token
String BOTtoken = "7715229379:AAFr0CpIM_ioKFiUMqn4ByGXncGLW00mFWI";

// Array of chat IDs
String CHAT_IDS[] = {"1527862279", "1728763056", "5497362560"}; 
int numberOfChats = 3;

// GPIO Pins
#define TRIGGER_BUTTON_PIN_1 14  // Button for Family
#define TRIGGER_BUTTON_PIN_2 13  // Button for Police and Ambulance
#define FLASH_LED_PIN 4

bool flashState = LOW;
bool sendPhoto = false;

bool lastButtonState1 = LOW;
bool lastButtonState2 = LOW;

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

// CAMERA_MODEL_AI_THINKER pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

void configInitCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    delay(1000);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_CIF);
}

String sendPhotoTelegram(String chat_id) {
  const char* myDomain = "api.telegram.org";
  String getAll = "", getBody = "";

  camera_fb_t * fb = esp_camera_fb_get();  
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }

  Serial.println("Connect to " + String(myDomain));
  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");

    String head = "--electroniclinic\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chat_id + "\r\n--electroniclinic\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--electroniclinic--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;

    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=electroniclinic");
    clientTCP.println();
    clientTCP.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n += 1024) {
      if (n + 1024 < fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      } else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen % 1024;
        clientTCP.write(fbBuf, remainder);
      }
    }

    clientTCP.print(tail);
    esp_camera_fb_return(fb);

    int waitTime = 10000;
    long startTimer = millis();
    bool state = false;

    while ((startTimer + waitTime) > millis()) {
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length() == 0) state = true; 
          getAll = "";
        } else if (c != '\r') {
          getAll += String(c);
        }
        startTimer = millis();
      }
      if (getBody.length() > 0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  } else {
    getBody = "Connection to api.telegram.org failed.";
    Serial.println(getBody);
  }
  return getBody;
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    bool authorized = false;
    for (int j = 0; j < numberOfChats; j++) {
      if (chat_id == CHAT_IDS[j]) {
        authorized = true;
        break;
      }
    }
    if (!authorized) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome " + from_name + "\nUse these commands:\n";
      welcome += "/photo : Take new photo\n";
      welcome += "/flash : Toggle flash\n";
      bot.sendMessage(chat_id, welcome, "");
    } else if (text == "/flash") {
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
    } else if (text == "/photo") {
      sendPhoto = true;
    }
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flashState);

  pinMode(TRIGGER_BUTTON_PIN_1, INPUT);
  pinMode(TRIGGER_BUTTON_PIN_2, INPUT);

  configInitCamera();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  bool currentButtonState1 = digitalRead(TRIGGER_BUTTON_PIN_1);
  bool currentButtonState2 = digitalRead(TRIGGER_BUTTON_PIN_2);

  if (currentButtonState1 == HIGH && lastButtonState1 == LOW) {
    Serial.println("Button 1 pressed: Sending photo to Family");
    sendPhotoTelegram(CHAT_IDS[0]);  // Only Family
  }

  if (currentButtonState2 == HIGH && lastButtonState2 == LOW) {
    Serial.println("Button 2 pressed: Sending photo to Police and Ambulance");
    for (int i = 1; i < numberOfChats; i++) {
      sendPhotoTelegram(CHAT_IDS[i]);  // Police and Ambulance
      delay(2000);
    }
  }

  lastButtonState1 = currentButtonState1;
  lastButtonState2 = currentButtonState2;

  if (sendPhoto) {
    for (int i = 0; i < numberOfChats; i++) {
      sendPhotoTelegram(CHAT_IDS[i]);
      delay(2000);
    }
    sendPhoto = false;
  }

  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
