#include <FastLED.h>

#define LED_PIN     3
#define CLOCK_PIN   4
#define COLOR_ORDER BGR
//GRB
#define CHIPSET     APA102
//WS2811
#define NUM_LEDS    56

#define BRIGHTNESS  50
#define FRAMES_PER_SECOND 60
#define NUM_FLAME_PALETTES 5

CRGB leds[NUM_LEDS];
CRGBPalette16 flamePalettes[NUM_FLAME_PALETTES]; 
int currentFlamePalette = 0;

// Here are the button pins:
#define BUTTON_IN_PIN 18 //A4 //A4 / 18
#define BUTTON_OUT_PIN 19 //A5 //A5 / 19

int mode = 1;
bool buttonDown = false;

void setup() {
  delay(1000); // sanity delay
  FastLED.addLeds<CHIPSET, LED_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );

  //Set up button for pressing!
  pinMode(BUTTON_IN_PIN, INPUT);
  pinMode(BUTTON_OUT_PIN, OUTPUT);
  digitalWrite(BUTTON_OUT_PIN, HIGH);

  // These are other ways to set up the color palette for the 'fire'.
  flamePalettes[0] = CRGBPalette16( CRGB::DarkRed, CRGB::Red, CRGB::Yellow, CRGB::WhiteSmoke);
  flamePalettes[1] = CRGBPalette16( CRGB::DarkBlue, CRGB::Blue, CRGB::Aqua,  CRGB::WhiteSmoke);
  flamePalettes[2] = CRGBPalette16( CRGB::LightBlue, CRGB::DarkBlue, CRGB::Orange);
  flamePalettes[3] = CRGBPalette16( CRGB::DarkGreen, CRGB::Green, CRGB::Purple, CRGB::Pink);
  flamePalettes[4] = CRGBPalette16( CRGB::Purple, CRGB::MediumOrchid, CRGB::Cyan);
}

void loop()
{
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy( random());

  // Fourth, the most sophisticated: this one sets up a new palette every
  // time through the loop, based on a hue that changes every time.
  // The palette is a gradient from black, to a dark color based on the hue,
  // to a light color based on the hue, to white.
  //
  //   static uint8_t hue = 0;
  //   hue++;
  //   CRGB darkcolor  = CHSV(hue,255,192); // pure hue, three-quarters brightness
  //   CRGB lightcolor = CHSV(hue,128,255); // half 'whitened', full brightness
  //   gPal = CRGBPalette16( CRGB::Black, darkcolor, lightcolor, CRGB::White);


  Fire2012WithPalette(); // run simulation frame, using palette colors
  
  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  
  if(readButton()) {
    if(!buttonDown) {
      currentFlamePalette++;
    }
    if(currentFlamePalette >= NUM_FLAME_PALETTES) {
      currentFlamePalette = 0;
    }
    buttonDown = true; 
  } else {
    buttonDown = false;
  }
}

bool readButton() {
  return digitalRead(BUTTON_IN_PIN);
}

void setStripColor(CRGB color) {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

#define COOLING  30
#define SPARKING 60

void Fire2012WithPalette()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      byte colorindex = scale8( heat[j], 240);
      leds[j] = ColorFromPalette( flamePalettes[currentFlamePalette], colorindex);
    }
}

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  // found here: http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

