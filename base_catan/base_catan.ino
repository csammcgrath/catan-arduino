/**
 * Author's TODO:
 * 
 * [X]: Refactor program
 * [X]: Add/cleanup comments
 * [X]: Fix game flow (reduce EEPROM storage/reads)
 * [X]: Possibly convert the resources into structs (this will probably require a major refactor)
 * [ ]: Test out with LED strip and ensure that everything is working as expected
 */

#include <Adafruit_NeoPixel.h>    // handle individually addressable LEDs
#include <EEPROM.h>               // used for persisting game state when turned off
#include <time.h>                 // for srand (seed for randomization)
#include <Array.h>                // Arrays (documentation: https://github.com/janelia-arduino/Array)

#define PIN 1                     // arduino pin for the led strip
#define NUMPIXELS 66              // 57 for resource tiles and 9 for ports

#define DELAY_VALUE 50            // time (ms) to pause between re-rendering the LEDs
#define TIME_SWITCH_BOARD 300000  // time (ms) to switch board

#define SWITCH_PIN 7              // randomizer button/switch is on this pin

/**
 * We may handle multiple "games" (5/6 players, Seafarers, C&K, etc) 
 * in one code. I'm leaning towards having different programs for 
 * different "games" to save memory but I'm adding this here just in
 * case if I get lazy in the future.
 */
#define BASE_PORT_SIZE 9
#define BASE_RESOURCE_SIZE 19

/**
 * Global variable to store last time (in ms) we looped
 * 
 * This is for the game variant where the board shuffles 
 * every X milliseconds. This # of milliseconds is defined 
 * in TIME_SWITCH_BOARD (global variables)
 */
unsigned int lastLoopMilli = 0;

/**
 * Setting up NeoPixel library with # of pixels and pin
 */
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


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
  brick() : rgb(169, 17, 1), isHidden(false) {};

  RGB rgb;
  bool isHidden;
};

struct wheat {
  wheat() : rgb(254, 223, 0), isHidden(false) {};

  RGB rgb;
  bool isHidden;
};

struct ore {
  ore() : rgb(157, 157, 157), isHidden(false) {};

  RGB rgb;
  bool isHidden;
};

struct sheep {
  sheep() : rgb(152, 251, 152), isHidden(false) {};

  RGB rgb;
  bool isHidden;
};

struct wood {
  wood() : rgb(0, 168, 119), isHidden(false) {};

  RGB rgb;
  bool isHidden;
};

struct desert {
  desert() : rgb(243, 180, 139), isHidden(false) {};

  RGB rgb;
  bool isHidden;
};

struct port {
  port() : rgb(255, 255, 255), isHidden(false) {};

  RGB rgb;
  bool isHidden;
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

  /**
   * shuffleBoard()
   * 
   * Main driver behind shuffling and storing updated locations in EEPROM
   */
  void shuffleBoard() {
    randomSeed(analogRead(A0));
    shuffleTiles();
    storeGame();
  }

  /**
   * Shuffle all of the tiles :)
   */
  void shuffleTiles() {
    randomize();
  }

  /**
   * @param vals[] <int[]> - array to be shuffled
   * @param n <int> - length of array because C++ sucks and doesn't store array size
   * Randomizer that takes in an array
   * 
   * Note to reader:
   * The `srand(time(NULL))` basically uses the internal clock to control the seed,
   * which is the starting value. For more information regarding this and why I'm 
   * approaching this as the way, please see:
   * https://mathbits.com/MathBits/CompSci/LibraryFunc/rand.htm
   */
  void randomize() {
    srand(time(NULL)); // uses the internal clock to control the seed
  
    for (int i = 0; i < BASE_RESOURCE_SIZE - 1; i++) {
      int j = rand() % (i + 1);
  
      swap(&resources.at(i), &resources.at(j));
    }

    for (int i = 0; i < BASE_PORT_SIZE - 1; i++) {
      int j = rand() % (i + 1);
  
      swap(&resources.at(i), &resources.at(j));
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
  
  /**
   * Stores the game into EEPROM just in case if someone bumps the plug
   * 
   * Note to reader: This function as well as readGame() used to use this
   * technique: https://roboticsbackend.com/arduino-store-int-into-eeprom/.
   * 
   * I'm not a huge fan of bitwise "magic" even though there's a cost in 
   * performance for doing that. I prefer to take a hit to performance for 
   * the returns in readability. I expect to come back to this in a few weeks/months 
   * to add support for other expansions.
   */
  void storeGame() {
    int resourceOffset = 0;
    int portOffset = sizeof(resources);
  
    EEPROM.put(resourceOffset, resources);
    EEPROM.put(portOffset, ports);
  }
  
  /**
   * Please refer to storeGame() block comments for more information
   * regarding this function.
   */
  void readGame() {
    int resourceOffset = 0;
    int portOffset = sizeof(resources);
  
    EEPROM.get(resourceOffset, resources);
    EEPROM.get(portOffset, ports);
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

void setupGame() {
  for (int i = 0; i < NUMPIXELS; i++) {
    int colorIndex = i / 3;
    int color = catan.resources.at(colorIndex);

    // ports
    if (i > 56) {
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
  pinMode(SWITCH_PIN, INPUT);
  buttonState = digitalRead(SWITCH_PIN);
  catan.readGame();
  pixels.begin(); 
}

/**
 * Function that gets called every time.
 * 
 * It gets called every x milliseconds. This is defined through 
 * DELAY_VALUE global variable. 
 */
void loop() {
  setupGame();
  
  buttonState = digitalRead(SWITCH_PIN);

  // user pressed button
  if (buttonState == HIGH) {
    catan.shuffleBoard();
  }

  // check timer
  if (millis() - lastLoopMilli >= TIME_SWITCH_BOARD) {
     catan.shuffleBoard();
  }
  
  delay(DELAY_VALUE);
}
