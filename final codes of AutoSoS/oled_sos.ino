#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define INPUT_PIN 2
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create display object using I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // -1 = no reset pin

void setup() {
  Wire.begin(21, 22);  // SDA = GPIO21, SCL = GPIO22

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C is common I2C address
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  pinMode(INPUT_PIN, INPUT);
  display.clearDisplay();
  display.display();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
}

void loop()
{
  if(digitalRead(INPUT_PIN) == HIGH)
  {
    while(1)
    {
        display.setCursor(30, 0);
        display.println("SOS");
        display.setCursor(15, 40);
        display.println("Alert");
        display.display();
        delay(500);
        display.clearDisplay();
        display.display();
        delay(500);
    }

  }
}
