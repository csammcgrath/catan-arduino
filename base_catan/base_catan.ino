#include <Adafruit_NeoPixel.h>    // handle individually addressable LEDs
#include <time.h>                 // for srand (seed for randomization)
#include <Array.h>                // Arrays (documentation: https://github.com/janelia-arduino/Array)

#define PIN 1                     // arduino pin for the led strip
#define NUMPIXELS 66              // 57 for resource tiles and 9 for ports

#define DELAY_VALUE 50            // time (ms) to pause between re-rendering the LEDs
#define SHUFFLE_TIMES_NUM 5       // Shuffle board 8 times to produce "loading" vibe
#define BUTTON_PIN 7              // randomizer button/switch is on this pin

/**
 * We may handle multiple "games" (5/6 players, Seafarers, C&K, etc) 
 * in one code. I'm leaning towards having different programs for 
 * different "games" to save memory but I'm adding this here just in
 * case if I get lazy in the future.
 */
#define BASE_PORT_SIZE 9
#define BASE_RESOURCE_SIZE 19

/**
 * Setting up NeoPixel library with # of pixels and pin
 */
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

boolean oldButtonState = HIGH;

//////////////////////////////////////////////////////////////
// ABSTRACT CLASSES
//////////////////////////////////////////////////////////////

struct rgb {
  rgb() : r(255), g(255), b(255) {};
  rgb(int red, int green, int blue) {
    r = red;
    g = green;
    b = blue;
  };
  
  int r;
  int g;
  int b;
};

/**
 * Define all abstract structs here
 */
typedef struct rgb RGB;

//////////////////////////////////////////////////////////////
// Catan hexes
//////////////////////////////////////////////////////////////

struct brick {
  brick() : rgb(255, 0, 0) {};

  RGB rgb;
};

struct wheat {
  wheat() : rgb(254, 223, 0) {};

  RGB rgb;
};

struct ore {
  ore() : rgb(25, 25, 150) {};

  RGB rgb;
};

struct sheep {
  sheep() : rgb(57,255,20) {};

  RGB rgb;
};

struct wood {
  wood() : rgb(0, 255, 0) {};

  RGB rgb;
};

struct desert {
  desert() : rgb(200, 76, 0) {};

  RGB rgb;
};

struct port {
  port() : rgb(255, 255, 255) {};

  RGB rgb;
};

/**
 * Definition of all Catan hexes
 */
typedef struct brick Brick;
typedef struct ore Ore;
typedef struct wheat Wheat;
typedef struct sheep Sheep;
typedef struct wood Wood;
typedef struct desert Desert;
typedef struct port Port;

//////////////////////////////////////////////////////////////
// Catan
//////////////////////////////////////////////////////////////

struct catan {
  catan() {};
  
  Array<int, BASE_PORT_SIZE> ports = {{ 0, 1, 2, 3, 4, 6, 6, 6, 6 }};
  Array<int, BASE_RESOURCE_SIZE> resources = {{ 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5 }};

  /*
   * Note to reader:
   * The `srand(time(NULL))` basically uses the internal clock to control the seed,
   * which is the starting value. For more information regarding this and why I'm 
   * approaching this as the way, please see:
   * https://mathbits.com/MathBits/CompSci/LibraryFunc/rand.htm
   */
  void shuffleBoard() {
    randomSeed(analogRead(A0));
  
    for (int i = 0; i < BASE_RESOURCE_SIZE; i++) {
      int j = random(0, BASE_RESOURCE_SIZE);
  
      swap(&resources.at(i), &resources.at(j));
    }

    randomSeed(analogRead(A0));
    for (int i = 0; i < BASE_PORT_SIZE; i++) {
      int j = random(0, BASE_PORT_SIZE);
  
      swap(&ports.at(i), &ports.at(j));
    }
  }
  
  /**
   * @param first <int*> - first item 
   * @param second <int*> - second item
   * Helper function that helps us swap values efficiently using pointers
   */
  void swap(int *first, int *second) {
    int temp = *first; // save first value
    *first = *second;
    *second = temp;
  }
};

typedef struct catan Catan;

Brick brick;    // 0
Ore ore;        // 1
Wheat wheat;    // 2
Sheep sheep;    // 3
Wood wood;      // 4
Desert desert;  // 5
Port port;      // 6 - Represents 3:1 port

Catan catan;

int buttonState = 0;

/**
 * Setting up the game through the led strip
 */
void setupGame() {
  for (int i = 0; i < pixels.numPixels(); i++) {
    int colorIndex = i / 3;
    int color = catan.resources.at(colorIndex);

    // ports
    if (i > 56) {
      // "reset" index ot 0 to follow the port array
      colorIndex = i - 57;
      color = catan.ports.at(colorIndex);
    }

    // here is where the numerical associations with the resource colors are used
    switch (color) {
      case 0:
        pixels.setPixelColor(i, pixels.Color(brick.rgb.r, brick.rgb.g, brick.rgb.b));
        break;
      case 1:
        pixels.setPixelColor(i, pixels.Color(ore.rgb.r, ore.rgb.g, ore.rgb.b));
        break;
      case 2:
        pixels.setPixelColor(i, pixels.Color(wheat.rgb.r, wheat.rgb.g, wheat.rgb.b));
        break;
      case 3:
        pixels.setPixelColor(i, pixels.Color(sheep.rgb.r, sheep.rgb.g, sheep.rgb.b));
        break;
      case 4:
        pixels.setPixelColor(i, pixels.Color(wood.rgb.r, wood.rgb.g, wood.rgb.b));
        break;
      case 5:
        pixels.setPixelColor(i, pixels.Color(desert.rgb.r, desert.rgb.g, desert.rgb.b));
        break;
      case 6:
        pixels.setPixelColor(i, pixels.Color(port.rgb.r, port.rgb.g, port.rgb.b));
        break;
    }
    
    pixels.show();
  }
}

/**
 * Initialization
 */
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Signify to the user that the game is ready to be created
  pixels.begin(); 
}

/**
 * Function that gets called every time.
 * 
 * It gets called every x milliseconds. This is defined through 
 * DELAY_VALUE global variable. 
 */
void loop() {
  boolean newButtonState = digitalRead(BUTTON_PIN);

  // user pressed button
  if (newButtonState == LOW && oldButtonState == HIGH) {
    //short delay to debounce button 
    delay(20);

    // double check to see if button is still low after debounce
    newButtonState = digitalRead(BUTTON_PIN);

    // The user has truly pushed the button
    if (newButtonState == LOW) {
      

      for (int i = 0; i < SHUFFLE_TIMES_NUM; i++) {
        catan.shuffleBoard();
        setupGame();
        delay(125);
      }
    }
  }
  
  delay(DELAY_VALUE);
}
