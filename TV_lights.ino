/*
 * Arduino interface for the use of WS2812 strip LEDs
 * Uses Adalight protocol and is compatible with Boblight, Prismatik etc...
 * "Magic Word" for synchronisation is 'Ada' followed by LED High, Low and Checksum
 * @author: Wifsimster <wifsimster@gmail.com> 
 * @library: FastLED v3.001
 * @date: 11/22/2015
 */
#include "FastLED.h"
#define NUM_LEDS 66
#define DATA_PIN 12
#define TV_PIN 13

bool tvOn = false;
int hideCount = 0;
int hideTimerStart = 25;
int hideTimer = hideTimerStart;

// Baudrate, higher rate allows faster refresh rate and more LEDs (defined in /etc/boblight.conf)
#define serialRate 115200

// Adalight sends a "Magic Word" (defined in /etc/boblight.conf) before sending the pixel data
uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;

// Initialise LED-array
CRGB leds[NUM_LEDS];
CRGB ledsBuffer[NUM_LEDS];

void ClearLEDS() {
  for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].r = 0;
      leds[i].g = 0;
      leds[i].b = 0;
    }
}

void setup() {
  // Use NEOPIXEL to keep true colors
  pinMode(13, INPUT);
  tvOn = digitalRead(TV_PIN);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  //FastLED.setBrightness(100);
  
  // Initial RGB flash
  LEDS.showColor(CRGB(255, 0, 0));
  delay(500);
  LEDS.showColor(CRGB(0, 255, 0));
  delay(500);
  LEDS.showColor(CRGB(0, 0, 255));
  delay(500);
  LEDS.showColor(CRGB(0, 0, 0));
  
  Serial.begin(serialRate);
  // Send "Magic Word" string to host
  Serial.print("Ada\n");
}

void loop() {
  // Wait for first byte of Magic Word
  for(i = 0; i < sizeof prefix; ++i) {
    waitLoop: while (!Serial.available()) {
      bool tvCheck = digitalRead(TV_PIN);
      //tvCheck = true;
      //tvCheck = 1;
      
      if (tvCheck != tvOn) {
        tvOn = tvCheck;
        hideCount = NUM_LEDS / 2;
        
        if (tvOn) {
          for (int j = 0; j < 3; j++) {
            for (int i = 0; i < 255; i++) {
              LEDS.showColor(CRGB(i, i, i));
              delay(4);
            }

            if (j <= 1) {
              for (int i = 255; i >= 0; i--) {
                LEDS.showColor(CRGB(i, i, i));
                delay(4);
              }
            }
          }

          //memcpy(leds, ledsBuffer, NUM_LEDS * sizeof(struct CRGB));
          
        }
        else {
          int mid = 22;
          for (unsigned i = 0; i <= NUM_LEDS / 2; i++) {
            int left = mid + i;
            int right = mid - i;
            
            left %= NUM_LEDS;
            if (right < 0)
              right += NUM_LEDS;
            
            
            leds[left].r = 0;
            leds[left].g = 0;
            leds[left].b = 0;
            leds[right].r = 0;
            leds[right].g = 0;
            leds[right].b = 0;
            FastLED.show();
            delay(25);
          }
          
        }
      }
    }
    // Check next byte in Magic Word
    if(prefix[i] == Serial.read()) continue;
    // otherwise, start over
    i = 0;
    goto waitLoop;
  }
  
  // Hi, Lo, Checksum  
  while (!Serial.available()) ;;
  hi=Serial.read();
  while (!Serial.available()) ;;
  lo=Serial.read();
  while (!Serial.available()) ;;
  chk=Serial.read();
  
  // If checksum does not match go back to wait
  if (chk != (hi ^ lo ^ 0x55)) {
    i=0;
    goto waitLoop;
  }
  
  memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));
  // Read the transmission data and set LED values
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    byte r, g, b;    
    while(!Serial.available());
    r = Serial.read();
    while(!Serial.available());
    g = Serial.read();
    while(!Serial.available());
    b = Serial.read();
    leds[i].r = r;
    leds[i].g = g;
    leds[i].b = b;
  }

  memcpy(ledsBuffer, leds, NUM_LEDS * sizeof(struct CRGB));

  if (tvOn)
    FastLED.show();
}
