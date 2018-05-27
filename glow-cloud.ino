#include <FastLED.h>

#ifndef PSTR
#define PSTR // Make Arduino Due happy
#endif

// debug or animation modes
// TODO: set this with a jumper to an input pin
bool debugMode = false;					// turns on debug() statements


// FastLED constants
#define NUM_LEDS							750
#define MAXSPRITES						20

#define NUM_COLORSETS					5
#define NUM_COLORS_PER_SET		9

// Data pins
#define PIR_SENSOR_1_PIN		 3
#define PIR_SENSOR_2_PIN		 4
#define NEOPIXEL_DATA_PIN		 6								// Pin for neopixels

// Test pattern refresh rate
#define TEST_PATTERN_FRAME_DELAY_IN_MS				1

// Array representing the strip.
CRGB leds[NUM_LEDS];
// -------
// By defining a single element in memory which SHOULD BE immediately before the large pixel array,
// I've effectively created the (-1)th element of the array. This frees me from having to check
// that I'm writing pixels to that area and will unknowingly blow something up, and saves a costly
// comparison (which is false ALMOST always) each time through each Sprite's Update() method.
// TODO: Do this properly with malloc()/calloc().

CRGB colorSets[NUM_COLORSETS][NUM_COLORS_PER_SET];

// Function prototypes.
void resetStrip(void);
void debug(int);
void debugNeg(int);
void debugN(int, int);
void stripcpy(CRGB *, CRGB *, int, int, int);
void createColorsets(void);
void createAnimationFrames(void);

class Sprite {
	public:
		Sprite() {
			lastUpdateTime = 0;
			done = false;
		}

		virtual ~Sprite() {
		}

		virtual bool Update() = 0;

		bool allowCreation() {
				return true;		// Always true, no reason to veto this one.
		}

		boolean UpdateNow() {
			if (millis() - lastUpdateTime >= updateInterval) {
				lastUpdateTime = millis();
				return true;
			} else {
				return false;
			}
		}

		void MarkDone() {
				this->done = true;
		}

		bool IsDone() {
				return this->done;
		}

	protected:
		uint32_t lastUpdateTime;
		int updateInterval;
		boolean done;
};

class SpriteVector {
	private:
		Sprite **sprites;
		int maxCapacity;
		int count;

	public:
		SpriteVector(int maxCap) {
			this->count = 0;
			this->maxCapacity = maxCap;
			sprites = new Sprite*[maxCap];

			for (int i = 0; i < maxCap; i++) {
				sprites[i] = NULL;
			}
		}

		~SpriteVector() {
		}

		Sprite *Get(int i) {
			if (i < this->count) {
				return sprites[i];
			} else {
				return NULL;
			}
		}

		boolean Add(Sprite *sprite) {
			if (count >= this->maxCapacity) {
				return false;
			}

			sprites[count] = sprite;
			++count;

			return true;
		}

		int Count() {
			return this->count;
		}

		boolean RemoveAt(int i) {
			Sprite *ptr = sprites[i];
			sprites[i] = NULL;
			delete ptr;

			for (int j = i + 1; j < count; j++) {
					sprites[j - 1] = sprites[j];
			}
			sprites[count - 1] = NULL;

			--this->count;

			return true;
		}
};

class SpriteManager {
	private:
		boolean updatedSomething = false;
		SpriteVector* spriteVector;

	public:
		SpriteManager() {
				spriteVector = new SpriteVector(MAXSPRITES);
		}

		~SpriteManager() {
			 // Don't bother. Should never be called.
		}

		int SpriteCount() {
			return spriteVector->Count();
		}

		void Update() {
				updatedSomething = false;

				for (int i = 0; i < this->SpriteCount(); i++) {
						updatedSomething |= spriteVector->Get(i)->Update();
				}

				if (updatedSomething) {
						FastLED.show();
				}

				this->Clean();
		}

		bool Add(Sprite *newSprite) {
			bool x = spriteVector->Add(newSprite);
			return x;
		}

		// Garbage collection. Remove any sprites that have finished their animation
		// from the SpriteVector, in order to make room for others.
		void Clean() {
				for (int i = this->SpriteCount() - 1; i >= 0; i--) {
						if (spriteVector->Get(i)->IsDone()) {
								spriteVector->RemoveAt(i);
						}
				}
		}
};

class W1V1Sprite : public Sprite {
	private:
		int currentPixel;
		CRGB color;

	public:
		W1V1Sprite() {
				this->currentPixel = -1;
				this->color = CRGB::White;
		}

		W1V1Sprite(int startPixel, CRGB startColor) {
				// Minus one because we'll increment it as the first step in Update().
				this->currentPixel = startPixel - 1;
				this->color = startColor;
		}

		~W1V1Sprite() {
		}

		boolean UpdateNow() {
			if (millis() - lastUpdateTime >= TEST_PATTERN_FRAME_DELAY_IN_MS) {
				lastUpdateTime = millis();
				return true;
			} else {
				return false;
			}
		}

		bool Update() {
			if (this->UpdateNow()) {
				currentPixel++;

				if (currentPixel >= NUM_LEDS) {
						leds[currentPixel - 1] = CRGB::Black;
						this->MarkDone();
				}

				leds[currentPixel] = this->color;
				if (currentPixel > 0) {
						leds[currentPixel - 1] = CRGB::Black;
				}

				return true;
			}

			return false;
		}
};

SpriteManager *spriteManager;

bool isBooted;
bool testSpritesCreated;

int starttime = millis();

void setup() {
		createColorsets();
		createAnimationFrames();

		isBooted = false;
		testSpritesCreated = false;

		spriteManager = new SpriteManager();

		resetStrip();
}

int counter = 0;

void loop() {
	if (! isBooted) {
		if (! testSpritesCreated) {
			spriteManager->Add(new W1V1Sprite(10, 0x750787));
			spriteManager->Add(new W1V1Sprite( 8, 0x004dff));
			spriteManager->Add(new W1V1Sprite( 6, 0x008026));
			spriteManager->Add(new W1V1Sprite( 4, 0xffed00));
			spriteManager->Add(new W1V1Sprite( 2, 0xff8c00));
			spriteManager->Add(new W1V1Sprite( 0, 0xe40303));

			testSpritesCreated = true;
		}

		spriteManager->Update();

		return;
	}
	
	// this is where we use sensors or other input to spawn sprites
	
	spriteManager->Update();
}

/* ---Utility functions --- */

void resetStrip() {
		FastLED.addLeds<NEOPIXEL, NEOPIXEL_DATA_PIN>(leds, NUM_LEDS);

		for (int i = 0; i < NUM_LEDS; i++) {
			leds[i] = CRGB::Black;
		}

		FastLED.show();
}

void debug(int number) {
	if (!debugMode) return;
		fill_solid(leds, NUM_LEDS, CRGB::Black);
		fill_solid(leds, number < NUM_LEDS ? number : NUM_LEDS, 0x202020);
		FastLED.show();
}

void debugNeg(int number) {
	if (!debugMode) return;
		fill_solid(leds + NUM_LEDS - number, number < NUM_LEDS ? number : NUM_LEDS, 0x202020);
}

void debugN(int startPos, int number) {
	if (!debugMode) return;
		fill_solid(leds + startPos, number < (NUM_LEDS - startPos) ? number : NUM_LEDS - startPos, 0x202020);
}

// TODO: faeries go both ways
void stripcpy(CRGB *leds, CRGB *source, int start, int width, int patternSize) {
		// We would be writing off the end anyway, so just bail. Nothing to do.
		if (start >= NUM_LEDS) {
				return;
		}

		// The position on the strip where the pattern will begin.
		int actualStartPosition = start >= 0 ? start : 0;

		int startPlusWidth = start + width;

		// The position on the strip AFTER the last pixel drawn.
		int actualEndPosition = startPlusWidth <= NUM_LEDS ? startPlusWidth : NUM_LEDS;

		// How many bytes into the source do we start copying?
		int patternStart = start >= 0 ? 0 : -start;

		// This may be negative, which indicates we'd finish writing before we get to the start.
		int actualBytes = actualEndPosition - actualStartPosition;

		if (actualBytes > 0) {
				memcpy(leds + actualStartPosition, (patternStart <= patternSize) ? source + patternStart : source, actualBytes * sizeof(CRGB));
		}
}

void createColorsets() {
// Blue.
		colorSets[0][0] = CRGB::Black;
		colorSets[0][1] = 0x000000;
		colorSets[0][2] = 0x000000;
		colorSets[0][3] = 0x000001;
		colorSets[0][4] = 0x000003;
		colorSets[0][5] = 0x010106;
		colorSets[0][6] = 0x02020C;
		colorSets[0][7] = 0x050519;
		colorSets[0][8] = 0x0A0A33;

// Yellow-green.
#if NUM_COLORSETS > 1
		colorSets[1][0] = 0x000000;
		colorSets[1][1] = 0x000000;
		colorSets[1][2] = 0x000100;
		colorSets[1][3] = 0x010200;
		colorSets[1][4] = 0x030500;
		colorSets[1][5] = 0x060A01;
		colorSets[1][6] = 0x0D1503;
		colorSets[1][7] = 0x1B2A06;
		colorSets[1][8] = 0x36540C;
#endif

#if NUM_COLORSETS > 2
// Amber.
		colorSets[2][0] = 0x000000;
		colorSets[2][1] = 0x000000;
		colorSets[2][2] = 0x000000;
		colorSets[2][3] = 0x010100;
		colorSets[2][4] = 0x030300;
		colorSets[2][5] = 0x060601;
		colorSets[2][6] = 0x0C0C02;
		colorSets[2][7] = 0x191905;
		colorSets[2][8] = 0x33330A;
#endif

#if NUM_COLORSETS > 3
// Reddish-purple.
		colorSets[3][0] = 0x000000;
		colorSets[3][1] = 0x000000;
		colorSets[3][2] = 0x010001;
		colorSets[3][3] = 0x030102;
		colorSets[3][4] = 0x060204;
		colorSets[3][5] = 0x0C0508;
		colorSets[3][6] = 0x180A11;
		colorSets[3][7] = 0x311523;
		colorSets[3][8] = 0x632A47;
#endif

#if NUM_COLORSETS > 4
// Original purple.
		colorSets[4][0] = 0x000000;
		colorSets[4][1] = 0x000000;
		colorSets[4][2] = 0x010001;
		colorSets[4][3] = 0x030102;
		colorSets[4][4] = 0x060305;
		colorSets[4][5] = 0x0C060A;
		colorSets[4][6] = 0x180C14;
		colorSets[4][7] = 0x311828;
		colorSets[4][8] = 0x633051;
#endif
}

void createAnimationFrames() {}