#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// NRF24L01 Pins
#define CE_PIN 4
#define CSN_PIN 5

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

const char* nrf_msg = "Ambulance";  // Const char array for NRF24

// WiFi and MQTT Credentials
const char* ssid = "Chotraju";
const char* password = "rhemanth123";
const char* mqtt_server = "cadfcd44d9104fc3a57a58ed5043a7f8.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "rhemanth";
const char* mqtt_password = "1Hivemqqt";

const char* topic_book = "/book_ambulance";
const char* topic_booked = "/ambulance_booked";

// GPIO pins
const int bookAmbulancePin = 21;
const int ambulanceBooked = 13;
const int redPin = 16;
const int bluePin = 17;

// LED Blink logic
const int interval = 500;
unsigned long previousMillis = 0;
bool redState = false, blueState = true;

// WiFi and MQTT
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Connect to Wi-Fi
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

// MQTT Callback
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = String((char*)payload);

  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(" | Message: ");
  Serial.println(message);

  if (String(topic) == topic_book && message == "Book Ambulance") {
    Serial.println("Ambulance booking triggered by Node-RED.");
    digitalWrite(bookAmbulancePin, HIGH);
  }
}

// MQTT Reconnect
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("connected.");
      client.subscribe(topic_book);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ambulanceBooked, INPUT);
  pinMode(bookAmbulancePin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  previousMillis = millis();

  // Set initial state (both off for common anode = HIGH)
  digitalWrite(redPin, HIGH);
  digitalWrite(bluePin, HIGH);

  setup_wifi();

  espClient.setInsecure(); // For HiveMQ Cloud
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Initialize NRF24L01
  if (!radio.begin()) {
    Serial.println("NRF24L01 initialization failed!");
  } else {
    Serial.println("NRF24L01 initialized.");
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setRetries(5, 15);
  radio.openWritingPipe(address);
  radio.stopListening();  // Set as transmitter
}

void loop() {
  // Reconnect to MQTT if needed
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 🔁 Continuous blinking of red and blue LEDs
  if ((millis() - previousMillis) >= interval) {
    previousMillis = millis();

    redState = !redState;
    blueState = !blueState;

    // For common anode: LOW = ON, HIGH = OFF
    digitalWrite(redPin, redState ? LOW : HIGH);
    digitalWrite(bluePin, blueState ? LOW : HIGH);
  }

  // 🚨 Send data when button is pressed
  if (digitalRead(ambulanceBooked) == HIGH) {
    Serial.println("Button Pressed. Sending data over NRF24 and MQTT...");

    // NRF24L01 message send
    bool success = radio.write(nrf_msg, strlen(nrf_msg));
    if (success) {
      Serial.println("NRF24L01 message sent successfully.");
    } else {
      Serial.println("Failed to send NRF24L01 message.");
    }

    // MQTT Publish
    String payload = "{\"status\":\"Dispatched\",\"eta\":\"04:30 mins\",\"sos\":\"Alerts sent\"}";
    client.publish(topic_booked, payload.c_str());

    delay(500); // Debounce delay
  }

  delay(100); // Loop pacing
}
