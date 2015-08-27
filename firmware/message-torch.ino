#include "application.h"


#include "neopixel.h"
#define	PIXEL_COUNT		300
#define	PIXEL_PIN		D0
#define	PIXEL_TYPE		WS2812B
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);


const uint16_t levels = 20; // Hot fix: had to reduce number of LEDs because new spark.core FW has less RAM free for user's app, otherwise crashes.
const uint16_t ledsPerLevel = 15; // approx

const uint16_t numLeds = ledsPerLevel*levels; // total number of LEDs


// global parameters
int brightness = 255; // overall brightness
byte fade_base = 140; // crossfading base brightness level


// torch parameters
uint16_t cycle_wait = 1; // 0..255

byte flame_min = 100; // 0..255
byte flame_max = 220; // 0..255

byte random_spark_probability = 2; // 0..100
byte spark_min = 200; // 0..255
byte spark_max = 255; // 0..255

byte spark_tfr = 50; // 0..256 how much energy is transferred up for a spark per cycle
uint16_t spark_cap = 200; // 0..255: spark cells: how much energy is retained from previous cycle

uint16_t up_rad = 40; // up radiation
uint16_t side_rad = 30; // sidewards radiation
uint16_t heat_cap = 0; // 0..255: passive cells: how much energy is retained from previous cycle

byte red_bg = 0;
byte green_bg = 0;
byte blue_bg = 0;
byte red_bias = 5;
byte green_bias = 0;
byte blue_bias = 0;
int red_energy = 256;
int green_energy = 150;
int blue_energy = 0;


void renderText();
void resetEnergy();
void calcNextEnergy();
void calcNextColors();
void injectRandom();


// Utilities
// =========
uint16_t random(uint16_t aMinOrMax, uint16_t aMax = 0) {
	if (aMax==0) {
		aMax = aMinOrMax;
		aMinOrMax = 0;
	}

	uint32_t r = aMinOrMax;
	aMax = aMax - aMinOrMax + 1;
	r += rand() % aMax;

	return r;
}
inline void reduce(byte &aByte, byte aAmount, byte aMin = 0) {
	int r = aByte-aAmount;

	if (r<aMin)
		aByte = aMin;
	else
		aByte = (byte)r;
}
inline void increase(byte &aByte, byte aAmount, byte aMax = 255) {
	int r = aByte+aAmount;

	if (r>aMax)
		aByte = aMax;
	else
		aByte = (byte)r;
}


byte currentEnergy[numLeds]; // current energy level
byte nextEnergy[numLeds]; // next energy level
byte energyMode[numLeds]; // mode how energy is calculated for this point


enum {
	torch_passive = 0, // just environment, glow from nearby radiation
	torch_nop = 1, // no processing
	torch_spark = 2, // slowly looses energy, moves up
	torch_spark_temp = 3, // a spark still getting energy from the level below
};



void setup() {
	strip.begin();
	strip.show();

	Spark.function("function", fnRouter);
}


void loop() {
	doTorch();
}


int fnRouter(String command) {
	command.trim();
	command.toUpperCase();

	if(command.substring(0, 9)=="FLAMERAD,") {
        	up_rad = command.substring(9).toInt();

	        return up_rad;


	// Lazy way to reboot
	} /* else if(command.equals("REBOOT")) {
		resetFlag = true;
		return 1;


	// Set red
	} else if(command.substring(0, 7)=="SETRED,") {
		color[0] = command.substring(7).toInt();
		intervalEffect = 0;

		return color[0];


	// Set green
	} else if(command.substring(0, 9)=="SETGREEN,") {
		color[1] = command.substring(9).toInt();
		intervalEffect = 0;

		return color[1];


	// Set blue
	} else if(command.substring(0, 8)=="SETBLUE,") {
		color[2] = command.substring(8).toInt();
		intervalEffect = 0;

		return color[2];


	// Set RGB
	} else if(command.substring(0, 7)=="SETRGB,") {
		color[0] = command.substring(7, 10).toInt();
		color[1] = command.substring(11, 14).toInt();
		color[2] = command.substring(15, 18).toInt();

		intervalEffect = 0;

		return 1;


	// Random color
	} else if(command.equals("RANDOMCOLOR")) {
		randomColor();
		intervalEffect = 0;

		return 1;


	// Set effect mode
	} else if(command.substring(0, 10)=="SETEFFECT,") {
		EFFECT_MODE = command.substring(10).toInt();
		intervalEffect = 0;

		return EFFECT_MODE;


	// Get effect mode
	} else if(command.equals("GETEFFECTMODE")) {
		return EFFECT_MODE;


	// Get pixel color
	} else if(command.substring(0, 14)=="GETPIXELCOLOR,") {
		return strip.getPixelColor(command.substring(14).toInt());


	// Set rainbow effect delay
	} else if(command.substring(0, 16)=="SETRAINBOWDELAY,") {
		RAINBOW_DELAY = command.substring(16).toInt();
		intervalEffect = RAINBOW_DELAY;

		return RAINBOW_DELAY;

	// Get rainbow effect delay
	} else if(command.equals("GETRAINBOWDELAY")) {
		return RAINBOW_DELAY;

	// Get the value of red
	} else if(command.equals("GETRED")) {
		return color[0];

	// Get the value of green
	} else if(command.equals("GETGREEN")) {
		return color[1];

	// Get the value of blue
	} else if(command.equals("GETBLUE")) {
		return color[2];

	// Set the brightness
	} else if(command.substring(0, 14)=="SETBRIGHTNESS,") {
		color_brightness = command.substring(14).toInt();
		strip.setBrightness(color_brightness);

		return color_brightness;

	// Get the brightness
	} else if(command.equals("GETBRIGHTNESS")) {
		return color_brightness;
	} */

	return -1;
}


void doTorch() {
	injectRandom();
	calcNextEnergy();
	calcNextColors();
}


void injectRandom() {
	// random flame energy at bottom row
	for (uint16_t i=0; i<ledsPerLevel; i++) {
		currentEnergy[i] = random(flame_min, flame_max);
		energyMode[i] = torch_nop;
	}

	// random sparks at second row
	for (uint16_t i=ledsPerLevel; i<2*ledsPerLevel; i++) {
		if (energyMode[i]!=torch_spark && random(100)<random_spark_probability) {
			currentEnergy[i] = random(spark_min, spark_max);
			energyMode[i] = torch_spark;
		}
	}
}


void resetEnergy() {
	for (uint16_t i=0; i<numLeds; i++) {
		currentEnergy[i] = 0;
		nextEnergy[i] = 0;
		energyMode[i] = torch_passive;
	}
}



void calcNextEnergy() {
	uint16_t i = 0;

	for (uint16_t y=0; y<levels; y++) {
		for (uint16_t x=0; x<ledsPerLevel; x++) {
			byte e = currentEnergy[i];
			byte m = energyMode[i];

			switch (m) {
				case torch_spark: {
					// loose transfer up energy as long as the is any
					reduce(e, spark_tfr);

					// cell above is temp spark, sucking up energy from this cell until empty
					if (y<levels-1) {
						energyMode[i+ledsPerLevel] = torch_spark_temp;
					}
					break;
				}
				case torch_spark_temp: {
					// just getting some energy from below
					byte e2 = currentEnergy[i-ledsPerLevel];

					if (e2<spark_tfr) {
						// cell below is exhausted, becomes passive
						energyMode[i-ledsPerLevel] = torch_passive;
						// gobble up rest of energy
						increase(e, e2);
						// loose some overall energy
						e = ((int)e*spark_cap)>>8;
						// this cell becomes active spark
						energyMode[i] = torch_spark;
					} else {
						increase(e, spark_tfr);
					}
					break;
				}
				case torch_passive: {
					e = ((int)e*heat_cap)>>8;
					increase(e, ((((int)currentEnergy[i-1]+(int)currentEnergy[i+1])*side_rad)>>9) + (((int)currentEnergy[i-ledsPerLevel]*up_rad)>>8));
				}
				default:
				break;
			}

			nextEnergy[i++] = e;
		}
	}
}



void calcNextColors() {
	for (uint16_t i=0; i<numLeds; i++) {
		//if (textLayer[i]>0) {
			// text is overlaid in light green-blue
			//leds.setColorDimmed(i, red_text, green_text, blue_text, (brightness*textLayer[i])>>8);
		//} else {
			uint16_t e = nextEnergy[i];
			currentEnergy[i] = e;
			if (e>230) {
				strip.setPixelColor(i, strip.Color(170, 170, e));
				strip.setBrightness(brightness);
			} else {
				//leds.setColor(i, e, (340*e)>>9, 0);
				if (e>0) {
					byte r = red_bias;
					byte g = green_bias;
					byte b = blue_bias;
					increase(r, (e*red_energy)>>8);
					increase(g, (e*green_energy)>>8);
					increase(b, (e*blue_energy)>>8);
					strip.setPixelColor(i, strip.Color(r, g, b));
					strip.setBrightness(brightness);
				} else {
					// background, no energy
					strip.setPixelColor(i, strip.Color(red_bg, green_bg, blue_bg));
					strip.setBrightness(brightness);
				}
			}
		//}
	}

	strip.show();
}