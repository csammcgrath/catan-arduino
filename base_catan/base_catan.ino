/**
 * Author's TODO:
 * 
 * [X]: Refactor program
 * [X]: Add/cleanup comments
 * [ ]: Fix game flow (reduce EEPROM storage/reads)
 * [ ]: Possibly convert the resources into structs (this will probably require a major refactor)
 * [ ]: Test out with LED strip and ensure that everything is working as expected
 */

#include <Adafruit_NeoPixel.h>    // handle individually addressable LEDs
#include <EEPROM.h>               // used for persisting game state when turned off
#include <time.h>                 // for srand (seed for randomization)

#define PIN 1                     // arduino pin for the led strip
#define NUMPIXELS 66              // 57 for resource tiles and 9 for ports
#define INCREMENT_SIZE 1          // controls how quickly the 3:1 ports will strobe

#define DELAY_VALUE 50            // time (ms) to pause between re-rendering the LEDs
#define TIME_SWITCH_BOARD 300000  // time (ms) to switch board

#define SWITCH_PIN 7              // randomizer button/switch is on this pin

/**
 * We may handle multiple "games" (5/6 players, Seafarers, C&K, etc) 
 * in one code. I'm leaning towards having different programs for 
 * different "games" to save memory but I'm adding this here just in
 * case if I get lazy in the future.
 */
#define BASE_RESOURCE_SIZE 19
#define BASE_PORT_SIZE 9

/**
 * Global variable to store last time (in ms) we looped
 * 
 * This is for the game variant where the board shuffles 
 * every X milliseconds. This # of milliseconds is defined 
 * in TIME_SWITCH_BOARD (global variables)
 */
unsigned int lastLoopMilli = 0;

/**
 * Original author didn't use a button at first so this will 
 * probably be incorrect. I won't know until we start messing
 * around with the led strip/
 */
bool shuffleButton = 0; // button to "shuffle" the board

/**
 * Setting up NeoPixel library with # of pixels and pin
 */
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

/**
 * This is definitely ugly AF and I would very much rather 
 * convert this into a data structure to handle all things. I looked
 * into using structs of some kind but will require an abstract class
 * to serve as interface to represent a common "resource". Also, we 
 * are limited to a certain # of bytes for memory
 */
int brick[3] = { 169, 17, 1 };      // 0
int wheat[3] = { 254, 223, 0 };     // 1
int ore[3] = { 157, 157, 157 };     // 2
int sheep[3] = { 152, 251, 152 };   // 3
int wood[3] = { 0, 168, 119 };      // 4
int desert[3] = { 243, 180, 139 };  // 5
int port[3] = { 255, 255, 255 };    // 6

// these arrays will be shuffled, the numbers correspond with the resource
int resources[19] = { 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5 };
int ports[9] = { 0, 1, 2, 3, 4, 6, 6, 6, 6 };

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
void randomize(int vals[], int n) {
  srand(time(NULL)); // uses the internal clock to control the seed

  for (int i = 0; i < n - 1; i++) {
    int j = rand() % (i + 1);

    swap(&vals[i], &vals[j]);
  }
}

/**
 * Shuffle all of the tiles :)
 */
void shuffleTiles() {
  randomize(resources, BASE_RESOURCE_SIZE);
  randomize(ports, BASE_PORT_SIZE);
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

/**
 * Initialization
 */
void setup() {
  pinMode(SWITCH_PIN, INPUT);
  shuffleButton = digitalRead(SWITCH_PIN);
  readGame();
  pixels.begin();  
}

/**
 * Function that gets called every time.
 * 
 * It gets called every x milliseconds. This is defined through 
 * DELAY_VALUE global variable. 
 */
void loop() {
  for (int i = 0; i < NUMPIXELS; i++) {
    int colorIndex = i / 3;
    int color = resources[colorIndex];

    // ports
    if (i > 56) {
      colorIndex = i - 57;
      color = ports[colorIndex];
    }

    // here is where the numerical associations with the resource colors are used
    switch(color) {
      case 0:
        pixels.setPixelColor(i, pixels.Color(brick[0], brick[1], brick[2]));
        break;
      case 1:
        pixels.setPixelColor(i, pixels.Color(wheat[0], wheat[1], wheat[2]));
        break;
      case 2:
        pixels.setPixelColor(i, pixels.Color(ore[0], ore[1], ore[2]));
        break;
      case 3:
        pixels.setPixelColor(i, pixels.Color(sheep[0], sheep[1], sheep[2]));
        break;
      case 4:
        pixels.setPixelColor(i, pixels.Color(wood[0], wood[1], wood[2]));
        break;
      case 5:
        pixels.setPixelColor(i, pixels.Color(desert[0], desert[1], desert[2]));
        break;
      case 6:
        pixels.setPixelColor(i, pixels.Color(port[1], port[2], port[3]));
        break;
    }
    
    pixels.show();

    bool newSwitchValue = digitalRead(SWITCH_PIN);

    // user pressed button
    if (newSwitchValue != shuffleButton) {
      shuffleButton = newSwitchValue;
      shuffleBoard();
      storeGame();
      readGame();
    }

    // check timer
    if (millis() - lastLoopMilli >= TIME_SWITCH_BOARD) {
       shuffleBoard();
       storeGame();
       readGame();
    }
  }
  
  delay(DELAY_VALUE);
}
