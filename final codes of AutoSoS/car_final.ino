#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>

// Wi-Fi Credentials
const char* ssid = "Chotraju";
const char* password = "rhemanth123";

// HiveMQ Broker Info
const char* mqtt_server = "cadfcd44d9104fc3a57a58ed5043a7f8.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "rhemanth";
const char* mqtt_password = "1Hivemqqt";

// MQTT Topics
const char* topic_crash = "/crash_detected";
const char* topic_confirm = "/accident_confirmed";
const char* topic_2min = "/2mintrigger";

// Telegram BOT Token and Chat IDs
String BOTtoken = "7715229379:AAFr0CpIM_ioKFiUMqn4ByGXncGLW00mFWI";
String CHAT_IDS[] = {"1527862279", "1728763056", "5931305208", "5497362560"};
int numberOfChats = 4;

// Pin Definitions
const int crashPin = 19;
const int confirmPin = 5;
const int min2 = 17;

// Latitude and Longitude
float lat = 0.00;
float lng = 0.00;

// Separate clients
WiFiClientSecure mqttSecureClient;
WiFiClientSecure telegramSecureClient;

PubSubClient mqttClient(mqttSecureClient);
UniversalTelegramBot bot(BOTtoken, telegramSecureClient);

//GPS object
TinyGPSPlus gps;
HardwareSerial GPSserial(2); // Use UART2

// Flags
bool sendToFamily = false;
bool sendToSOS = false;

// Setup Wi-Fi
void setup_wifi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// MQTT message callback
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = String((char*)payload);

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == topic_confirm && message == "Accident Confirmed") {
    Serial.println("🚨 Accident confirmed. Triggering alert logic.");
    digitalWrite(confirmPin, HIGH);
    sendToFamily = true;
    delay(500);
    digitalWrite(confirmPin, LOW);
  }

  if (String(topic) == topic_2min && message == "Book Ambulance") {
    Serial.println("🚨 Ambulance booking triggered.");
    digitalWrite(min2, HIGH);
    sendToSOS = true;
    delay(500);
    digitalWrite(min2, LOW);


  }
}

// Send location to family
void sendLocationfam() {
  while (GPSserial.available() > 0) {
    gps.encode(GPSserial.read());
    
    if (gps.location.isUpdated())
    {
      lat = gps.location.lat();
      lng = gps.location.lng();
    }
  }

  String msg = "Location: https://maps.google.com/?q=" + String(lat) + "," + String(lng);
  bot.sendMessage(CHAT_IDS[0], msg, "Markdown");
}

// Send location to SOS contacts
void sendLocationsos() {
  
    while (GPSserial.available() > 0) {
    gps.encode(GPSserial.read());
    
    if (gps.location.isUpdated())
    {
      lat = gps.location.lat();
      lng = gps.location.lng();
    }
  }
  String msg = "Location: https://maps.google.com/?q=" + String(lat) + "," + String(lng);
  for (int i = 1; i < numberOfChats; i++) {
    bot.sendMessage(CHAT_IDS[i], msg, "Markdown");
    delay(500); // Small delay to prevent overload
  }
}

// Reconnect to MQTT if disconnected
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect("ESP32_VEHICLE", mqtt_username, mqtt_password)) {
      Serial.println("connected.");
      mqttClient.subscribe(topic_confirm);
      mqttClient.subscribe(topic_2min);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds.");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(crashPin, INPUT);
  pinMode(confirmPin, OUTPUT);
  pinMode(min2, OUTPUT);
  digitalWrite(confirmPin, LOW);
  digitalWrite(min2, LOW);

  setup_wifi();

  mqttSecureClient.setInsecure();
  telegramSecureClient.setInsecure();

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);

  GPSserial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  // Crash detection
  if (digitalRead(crashPin) == HIGH) {
    Serial.println("💥 Crash detected! Sending MQTT message.");
    mqttClient.publish(topic_crash, "CRASH");
    delay(1000); // Debounce
  }

  // Deferred Telegram messages
  if (sendToFamily) {
    sendLocationfam();
    sendToFamily = false;
  }

  if (sendToSOS) {
    sendLocationsos();
    sendToSOS = false;
  }
}
