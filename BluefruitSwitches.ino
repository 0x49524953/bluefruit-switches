#include <avr/io.h>
#include <Keyboard.h>
#include <Adafruit_NeoPixel.h>
#include "Arduino.h"
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "BluefruitConfig.h"

#define ANNOYING_LED            13
#define FACTORYRESET_ENABLE     0
#define NEOPIXEL_VERSION_STRING "Neopixel v2.0"
#define C                       '+'
#define V                       '-'

Adafruit_NeoPixel neopixel(2, 9, NEO_GRB + NEO_KHZ800);
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, 
                             BLUEFRUIT_SPI_IRQ, 
                             BLUEFRUIT_SPI_RST);

const char RESET_BLE[] =        "ATZ";
const char STOP_ADV[] =         "AT+GAPSTOPADV";
const char START_ADV[] =        "AT+GAPSTARTADV";
const char ENABLE_CONN[] =      "AT+GAPCONNECTABLE=1";
const char DISABLE_CONN[] =     "AT+GAPCONNECTABLE=0";
const char ENABLE_CONN_LED[] =  "AT+HWCONNLED=1";
const char DISABLE_CONN_LED[] = "AT+HWCONNLED=0";
const char ENABLE_ACTV_LED[] =  "AT+HWMODELED=mode";
const char DISABLE_ACTV_LED[] = "AT+HWMODELED=disable";

// default color
uint32_t MAGENTA =              neopixel.Color(255,0,255);
uint32_t BLACK =                neopixel.Color(0,0,0);
uint32_t BLUE =                 neopixel.Color(0,0,255);

uint32_t COLORS[] =           { MAGENTA, MAGENTA };
uint32_t COLORO =               BLACK;

bool A() { return bitRead(PINC, 6) ? false : true; }
bool B() { return bitRead(PIND, 7) ? false : true; }

unsigned long time_since_last = 0;
unsigned long last_checked = millis();
bool A_LAST;
bool B_LAST;
bool A_ON;
bool B_ON;
bool A_PRESSED;
bool B_PRESSED;
bool A_RELEASE;
bool B_RELEASE;

void updateButtons() {
  time_since_last = millis() - time_since_last;
  last_checked = millis();
  if (time_since_last < 10) {
    delay(10);
    time_since_last += 10;
  }
  A_LAST = A_ON;
  B_LAST = B_ON;
  A_ON = A();
  B_ON = B();
  
  A_PRESSED = A_ON && (A_LAST == false);
  A_RELEASE = (A_ON == false) && A_LAST;
  B_PRESSED = B_ON && (B_LAST == false);
  B_RELEASE = (B_ON == false) && B_LAST;
}

void setup() {
  // turn off annoying LED
  pinMode(ANNOYING_LED, OUTPUT);
  digitalWrite(ANNOYING_LED, false);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  delay(10);
  
  time_since_last = millis();
  last_checked = millis();
  A_ON = A();
  B_ON = B();
  
  Keyboard.begin();
  Serial.begin(115200);
  delay(2000);
  
  // Start BLE module
  if ( !ble.begin(VERBOSE_MODE) ) {while (true);}

  ble.echo(false);
  ble.verbose(false);
  disableBLE();
  
  // Start Neopixels
  neopixel.begin();
  neopixel.setPixelColor(0, MAGENTA);
  neopixel.setPixelColor(1, MAGENTA);
  neopixel.setBrightness(15);
  neopixel.show();
  neopixel.show();
}

void loop() {
  updateButtons();
  bool changed = A_PRESSED || B_PRESSED || A_RELEASE || B_RELEASE;
  
  // BUTTON A PRESSED ONCE
  if (A_PRESSED && !(B_PRESSED || B_ON)) {
    if (Serial) Serial.println("ZOOMED IN!");
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press(C);
    delay(5);
    Keyboard.releaseAll();
  }

  // BUTTON B PRESSED ONCE
  else if (B_PRESSED && !(A_PRESSED || A_ON)) {
    if (Serial) Serial.println("ZOOMED OUT!");
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press(V);
    delay(5);
    Keyboard.releaseAll();    
  }

  // BOTH BUTTONS PRESSED ONCE, SIMULTANEOUSLY
  else if ((A_PRESSED && B_ON) || (B_PRESSED && A_ON)) {
    Serial.println("BLE REENABLED!");
    enableBLE();
    neopixel.setPixelColor(0, BLUE);
    neopixel.setPixelColor(1, BLUE);
    neopixel.show();

    // Wait for both switches to be released
    do {
      updateButtons();
    } while (A_ON || B_ON);
    
    delay(10);
    
    while (!((A_PRESSED && B_ON) || (B_PRESSED && A_ON))) {
      if ( ble.isConnected() ) {
        int command = ble.read();
        switch (command) {
          case 'V': {commandVersion(); break;}
          case 'S': {commandSetup(); break;}
          case 'C': {commandClearColor(); break;}
          case 'B': {commandSetBrightness(); break;}     
          case 'P': {commandSetPixel(); break;}
          case 'I': {break;}
        }
      }
      updateButtons();
    }
    
    disableBLE();
    if (Serial) Serial.println("BLE DISABLED!");
    
    neopixel.setPixelColor(0, COLORS[0]);
    neopixel.setPixelColor(1, COLORS[1]);
    neopixel.show();
    delay(10);
    
    updateButtons();
    delay(125);
    
    return;
  }

  if (changed) {
    neopixel.setPixelColor(0, (!A_ON) ? COLORS[0] : COLORO);
    neopixel.setPixelColor(1, (!B_ON) ? COLORS[1] : COLORO);
    neopixel.show();
  }
}

void disableBLE() {
    ble.println(STOP_ADV);
    ble.waitForOK();
    ble.println(DISABLE_CONN);
    ble.waitForOK();
    ble.println(DISABLE_ACTV_LED);
    ble.waitForOK();
    ble.println(DISABLE_CONN_LED);
    ble.waitForOK();
    ble.println(RESET_BLE);
    ble.waitForOK();
}

void enableBLE() {
    ble.println(ENABLE_CONN_LED);
    ble.waitForOK();
    ble.println(ENABLE_ACTV_LED);
    ble.waitForOK();
    ble.println(ENABLE_CONN);
    ble.waitForOK();
    ble.println(START_ADV);
    ble.waitForOK();
    ble.println(RESET_BLE);
    ble.waitForOK();
}

void commandVersion() {
  sendResponse(NEOPIXEL_VERSION_STRING);
}

void commandSetup() {
  uint8_t width_ignored =           ble.read();
  uint8_t height_ignored =          ble.read();
  uint8_t stride_ignored =          ble.read();
  uint8_t componentsValue_ignored = ble.read();
  bool is400Hz_ignored =            ble.read();

  bool ignored = width_ignored || height_ignored || stride_ignored || componentsValue_ignored || is400Hz_ignored;
  ignored = true;
  
  if (ignored) sendResponse("OK");
}

void commandSetBrightness() {
   // Read value
  uint8_t brightness = ble.read();
  neopixel.setBrightness(brightness);
  neopixel.show();
  sendResponse("OK");
}

void commandClearColor() {
  // Read color
  uint8_t rgb[3];
  rgb[0] = (ble.available()) ? ble.read() : 0;
  rgb[1] = (ble.available()) ? ble.read() : 0;
  rgb[2] = (ble.available()) ? ble.read() : 0;
  
  uint32_t color = neopixel.Color(rgb[0], rgb[1], rgb[2]);
  
  neopixel.setPixelColor(0, color);
  neopixel.setPixelColor(1, color);
  COLORS[0] = color;
  COLORS[1] = color;
  neopixel.show();

  sendResponse("OK");
}

void commandSetPixel() {
  // Read position
  uint32_t x = ble.read();
  uint8_t y = ble.read(); // ignored
  y = 1;
  
  // Read colors
  uint8_t rgb[3];
  rgb[0] = (ble.available()) ? ble.read() : 0;
  rgb[1] = (ble.available()) ? ble.read() : 0;
  rgb[2] = (ble.available()) ? ble.read() : 0;

  uint32_t color = neopixel.Color(rgb[0], rgb[1], rgb[2]);
  
  neopixel.setPixelColor(x, color);
  COLORS[x] = color;
  neopixel.show();

  if (y) sendResponse("OK");
}

void sendResponse(char const *response) {
    ble.write(response, strlen(response)*sizeof(char));
}
