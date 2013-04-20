#include <Button.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

#define LED_COUNT 60
#define THRESHOLD 1

#define COLOR_MODE 0
#define COUNTDOWN_MODE 1
#define SEIZURE_MODE 2
#define COOLDOWN_MODE 3

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, 5, NEO_GRB + NEO_KHZ800);

uint8_t pixel_index;

long last_time;
long countdown_start;
long cooldown_start;
long countdown_length = 500;
long cooldown_length;

Button button = Button(13, BUTTON_PULLUP_INTERNAL, true, 50);

float brightness = 1.0;
boolean all_on = false;

int draw_mode = COLOR_MODE;

void onPress(Button& b) {
  if(draw_mode == COLOR_MODE) {
    countdown_start = millis();
    draw_mode = COUNTDOWN_MODE;
  }
  if(draw_mode == COOLDOWN_MODE) {
    countdown_start = millis();
    countdown_start = countdown_start - (cooldown_length - (countdown_start - cooldown_start));
    draw_mode = COUNTDOWN_MODE;
  }
}

void onRelease(Button& b) {
  if(draw_mode == COUNTDOWN_MODE) {
    cooldown_start = millis();
    cooldown_length = cooldown_start - countdown_start;
    draw_mode = COOLDOWN_MODE;
  }
}

void onHold(Button& b) {
  if(draw_mode == SEIZURE_MODE)
    draw_mode = COLOR_MODE;
}

void setup()
{
  // Set the MOSI pin as digital output
  // (only for BB RevA)
  //  DDRB |= _BV(2);
  
  Serial.begin(57600);

  strip.begin();
  strip.show();
  last_time = millis();

  button.pressHandler(onPress);
  button.releaseHandler(onRelease);
  button.holdHandler(onHold, 1000); // must be held for at least 1000 ms to trigger
}


uint8_t i = 0;
int j = 0;
int f = 0;
int k = 0;

int count;

void color_loop() {  
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    uint8_t red =   64*(1+sin(i/2.0 + j/4.0       ))*brightness;
    uint8_t green = 64*(1+sin(i/1.0 + f/9.0  + 2.1))*brightness;
    uint8_t blue =  64*(1+sin(i/3.0 + k/14.0 + 4.2))*brightness;
    
    uint32_t pix = green;
    pix = (pix << 8) | red;
    pix = (pix << 8) | blue;
    
    strip.setPixelColor(i, pix);
    
    if ((millis() - last_time > 15) && pixel_index <= LED_COUNT + 1) {
      last_time = millis();
      count = LED_COUNT - pixel_index;
      pixel_index++; 
    }
    
    for (int x = count; x >= 0; x--) {
      strip.setPixelColor(x, strip.Color(0,0,0));
    }
    
  }
  
  strip.show();
  
  j = j + random(1,2);
  f = f + random(1,2);
  k = k + random(1,2);
}

void countdown_loop() {
  long elapsed = millis() - countdown_start;
  if(elapsed > countdown_length) {
    Serial.println("SEIZURE MODE ACTIVATE");
    draw_mode = SEIZURE_MODE;
    last_time = millis();
  } else {
    draw_progress(elapsed, countdown_length);
  }
}

void cooldown_loop() {
  long elapsed = millis() - cooldown_start;
  if(elapsed > cooldown_length) {
    draw_mode = COLOR_MODE;
    last_time = millis();
  } else {
    draw_progress(cooldown_length - elapsed, countdown_length);
  }
}

void draw_progress(long elapsed, long countdown_length) {
  Serial.print("Elapsed: "); Serial.println(elapsed);
  float progress = (float) elapsed / (float) countdown_length;
  int count = round((float)LED_COUNT * progress);
  for (int x = 0; x < count; x++) {
    strip.setPixelColor(x, strip.Color(255,255,255));
  }
  for(int x = count+1; x < LED_COUNT; x++) {
    strip.setPixelColor(x, strip.Color(255,0,0));
  }
  strip.show();
}

void seizure_loop() {
  long elapsed = millis() - last_time;
  uint32_t new_color;
  Serial.print("Seizure time Elapsed: ");
  Serial.println(elapsed);
  if(elapsed > 30) {
    all_on = ~all_on;
    last_time = millis();
  }
  if(all_on)
    new_color = strip.Color(255,255,255);
  else
    new_color = strip.Color(0,0,0);
  for(int x = LED_COUNT; x >= 0; x--) {
    strip.setPixelColor(x, new_color);
  }
  strip.show();
}

void serialLoop() {

  while(true) {

    if(Serial.available() > 2) {

      int buffer[3]; // Buffer to store three incoming bytes used to compile a single LED color
      int x;
      for (x=0; x<3; x++) { // Read three incoming bytes
        int c = Serial.read();
        if (c < 255) buffer[x] = c; // Using 255 as a latch semaphore
        else {
          strip.show();
          pixel_index = 0;
          break;
        }

        if (x == 2) {   // If we received three serial bytes

          uint32_t color = strip.Color(buffer[0], buffer[1], buffer[2]);
          strip.setPixelColor(pixel_index, color);
          pixel_index++;

        }
      }
    }
  }
}

void loop()
{
  // If'n we get some data, switch to passthrough mode
  if(Serial.available() > 0) {
    serialLoop();
  }
  switch(draw_mode) {
    case COLOR_MODE:
      color_loop();
      break;
    case COUNTDOWN_MODE:
      countdown_loop();
      break;
    case COOLDOWN_MODE:
      cooldown_loop();
      break;
    case SEIZURE_MODE:
      seizure_loop();
      break;
  }
  button.process();
}



