#include <SPI.h>
#include <RF24.h>

#define CE_PIN 4
#define CSN_PIN 5
#define IRQ_PIN 26

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

// Traffic light GPIOs
#define RED_PIN  32
#define YELLOW_PIN 33
#define GREEN_PIN 27

volatile bool ambulanceIncoming = false;

void IRAM_ATTR irqHandler() {
  // Flag is set to true, actual processing is done in loop
  ambulanceIncoming = true;
}

void setup() {
  Serial.begin(115200);

  // Traffic lights
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);

  // Set up radio
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();

  // IRQ pin
  pinMode(IRQ_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IRQ_PIN), irqHandler, FALLING);
}

void loop() {
  if (ambulanceIncoming) {
    ambulanceIncoming = false;

    if (radio.available()) {
      char text[32] = "";
      radio.read(&text, sizeof(text));

      if (strcmp(text, "Ambulance") == 0) {
        Serial.println("AMBULANCE DETECTED!");
        giveGreenSignal();
      }
    }
  }

  normalTrafficCycle(); // You may add a state check to disable during green override
}

void giveGreenSignal() {
  // Force green signal
  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH);
  delay(10000);
}

void normalTrafficCycle() {
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  delay(1000);

  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, HIGH);
  delay(300);

  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH);
  delay(1000);

  digitalWrite(GREEN_PIN, LOW);
}