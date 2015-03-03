/*
 * RanduSynth - a "something-to-synth" converter based on Arduino DUE
 * 
 * Connect a guitar to it and start making noise!
 * 
 * DEFAULT WIRING CONFIGURATION:
 * -	connect the guitar/line in/whatever to pin A0. Use a preamp if needed. The input MUST be between 0 and 3.3V.
 * 		DO NOT EXCEED 3.3V
 * -	connect DAC1 to an amplifier. You can follow this: http://arduino.cc/en/Tutorial/SimpleAudioPlayer
 * -	(optional) clipping indicator: connect a LED to pin 10
 * 
 * CREDITS:
 * This sketch was built around the "SplitRadixReal FFT Library" example from
 * https://coolarduino.wordpress.com/2014/09/25/splitradixreal-fft-library/
 * I just hacked it and added code to fit my needs.
 * All the "audio input part" and FFT calculations came from this.
 * 
 * The synth sound output functions came from http://groovuino.blogspot.it/ - https://github.com/Gaetino/Groovuino
 * I slightly modified the library, now it works on Linux and some variables
 * are overridable in the sketch - see https://github.com/pierinz/Groovuino
 * 
 * I should mention all people that led me here with their projects:
 * Amanda Ghassaei (http://www.instructables.com/member/amandaghassaei/)
 * All the Auduino authors (https://code.google.com/p/tinkerit/wiki/Auduino)
 * All the geek friends that were interested in "this thing"
 * Thank you!
 */


/********** SKETCH CONFIGURATION ***********/
// Enable debug (via serial messages), remove in production to decrease memory footprint
#define DEBUG

// Use simple square wave instead of synth, ignoring source amplitude.
// Good for testing, not intended for production use
// If you use this, by default pin 13 will be used instead of DAC1
//#define TONE_SIMPLE

// Clipping pin (LED)
// If the input is being clipped (source voltage > 3.3 or less than 0) turn on the light
byte clipping=10;

#ifdef TONE_SIMPLE
// Output square wave on this pin. Wire to a buzzer or something similar. Don't forger a resistor!
byte tonepin=13;
#endif

// Analog output on DAC1. Can't change this now. Don't forget a buffer or a resistor!!!!!

// Analog input on A0. Can't change this now. Remember the DC offset and the board limits!

// Sample rate. This will affect resolution. Don't change this if you don't know what you are doing.
#define SMP_RATE 9000UL

// Resolution (Hz) is SMP_RATE/FFT_SIZE. Change this if you change the other two.
#define RESOLUTION 4.394

/*******************************************/



// Now, the code

#include <SplitRadixRealP.h>

#define CLK_MAIN 84000000UL
#define TMR_CNTR CLK_MAIN / (2 *SMP_RATE)

// FFT_SIZE IS DEFINED in Header file Radix4.h
// #define FFT_SIZE 2048
#define MIRROR FFT_SIZE / 2
#define INP_BUFF FFT_SIZE

volatile uint16_t sptr = 0;
volatile int16_t flag = 0;

uint16_t inp[2][INP_BUFF]= { 0 }; // DMA likes ping-pongs buffer

int f_r[FFT_SIZE]= { 0 };
// This is commented out because is less accurate
//int out1[MIRROR]={ 0 }; // Magnitudes
int out2[MIRROR]={ 0 }; // Magnitudes

// We must remove the DC offset before the FFT calculation
const int dc_offset= 2047;

uint8_t print_inp=0; // print switch

unsigned long time_start;
unsigned int time_hamng, time_revbn, time_radix, time_gainr, time_sqrtl, time_sqrtl2;

SplitRadixRealP radix;


// Workaround, Timer0 is used by ADC
#define USING_SERVO_LIB true
#include <DueTimer.h>

#include "v_tables.h"
#include "funcs.h"

#ifndef TONE_SIMPLE
// Includes in header files won't work sometimes
#include <osc.h>
#endif
#include "sound_out.h"

#ifdef DEBUG
// Trace execution time
unsigned long thebeginning=0;
unsigned long theend=0;
#endif

#ifndef TONE_SIMPLE
// Initialize synth variables
int oldlogmagn=0;
#endif

// If TONE_SIMPLE, the frequency. Else, the MIDI note index.
int freq=0, oldfreq=0;

/** -- **/
#ifdef DEBUG
void cmd_print_help(void){
	Serial.println("\n  Listing of all available CLI Commands\n");
	Serial.println("\t\"?\" or \"h\": print this menu");
	Serial.println("\t\"d\": print out execution time and other info");
	Serial.println("\t\"s\": print out synth info");
	Serial.println("\t\"x\": print out adc array");
	Serial.println("\t\"f\": print out fft array");
	Serial.println("\t\"o\": print out magnitude array");
	Serial.println("\t\"m\": print out time measurements results");
}
#endif

void setup()
{
#ifdef DEBUG
	Serial.begin (115200);
#endif
	adc_setup();
	tmr_setup();

	pinMode( 2, INPUT); //

	pinMode(clipping,OUTPUT);
#ifdef TONE_SIMPLE
	pinMode(13,OUTPUT);
#else
	// Init sinth
	createSineTable();
	createSawTable();
	createSqTable();

	osc.init();
	//Sinewave
	osc.setWaveform(0,60);
	//Volume 0 for osc 0
	osc.setVolOsc(0, 0);
	//Lowest note, in freq and vol (mute)
	osc.setNote(0, 0);
	
	// Set up DAC
	analogWrite(DAC1,0);

	// Attach timer
	Timer6.attachInterrupt(synth).setFrequency(SAMPLE_RATE).start();
#endif
}

void loop()
{
	int z, magn=0, logmagn=0, top=0;

	if (flag){ // We got all the data from ADC
		// Trace time
		thebeginning=micros();
		// Reset clipping LED
		digitalWrite(clipping,0);

		// Perform mystical calculations
		FFTcompute();

		// Find fundamental
		for (z=0; z < MIRROR; z++){
			if (out2[z] > magn){
				// This is the magnitude
				magn=out2[z];
				// This is the MIDI note index
				top=z;
			}
		}

#ifndef TONE_SIMPLE
		// Scale magnitude from hash table
		logmagn=t10log10x8[magn];
#endif

		// Skip useless calculations
		if (out2[top] < 5){
			freq=0;
		}
		else{
#ifdef TONE_SIMPLE
			// Find frequency from hash table
			freq=pitches[top];
#else
			// Find MIDI note from hash table
			freq=midinotes[top];
#endif
		}
		// Trace time
		theend=micros();
	

		// If the frequency has changed, change the output, make sound
		if (freq!=oldfreq){
			if (freq==0){
#ifdef TONE_SIMPLE
			Timer6.stop();
			digitalWrite(13,0)
#else
			osc.setNote(0, 0);
#endif
			}
			else{
#ifdef TONE_SIMPLE
				Timer6.attachInterrupt(buzzer).setFrequency(freq).start();
#else
				osc.setNote(freq, magn*2);
#endif
			}
			oldfreq=freq;
		}

#ifndef TONE_SIMPLE
		// If the magnitude has changed, change output volume
		if (oldlogmagn!=logmagn) {
                        // Remove the popping sound (which sound? remove the IF and discover by yourself!)
			if (freq != 0){
				osc.setVolOsc(0,logmagn*8);
				oldlogmagn=logmagn;
			}
		}
#endif

#ifdef DEBUG
// Serial console. Type "h" for help.
		while (Serial.available()) {
			uint8_t input = Serial.read();
			switch(input) {
				case '\r':
				break;

				case 'd': // Wow so much debug data
#ifdef TONE_SIMPLE
					Serial.print("Frequecy from hash table: ");
#else
					Serial.print("MIDI note index: ");
#endif
					Serial.print(freq);
					Serial.print(" / Real frequency: ");
					Serial.print(top*SMP_RATE/FFT_SIZE);
					Serial.print(" / FFT index: ");
					Serial.print(top);
					Serial.print(" / Magnitude (linear): ");
					Serial.print(magn);
					Serial.print(" / Magnitudine (logarhitmic): ");
					Serial.print(logmagn);
					Serial.print("/ Execution time (µs): ~");
					Serial.println(theend-thebeginning);
				break;

#ifndef TONE_SIMPLE
				case 's': // Synth debug data
					Serial.print("Synth volume: ");
					Serial.print(osc.volosc[0]);
					Serial.print(" / synth note: ");
					Serial.println(osc.fFrequency);
				break;
#endif

				case 'x': // Print ADC data
					print_inp = 1;
				break;

				case 'f': // Print FFT
					Serial.print("\n\tReal: ");
					prnt_out2( f_r, MIRROR);
				break;

				case 'o': // Print magnitude
					Serial.print("\n\tMagnitudes: ");
					//prnt_out2( out1, MIRROR);
					prnt_out2( out2, MIRROR);
				break;

				case 'm': // FFT related functions time measurements
					Serial.print("\tHamng ");
					Serial.print(time_hamng);
					Serial.print("\tRevb ");
					Serial.print(time_revbn);
					Serial.print("\tSplitRR ");
					Serial.print(time_radix);
					Serial.print("\tGainR ");
					Serial.print(time_gainr);
					Serial.print("\tSqrt ");
					Serial.print(time_sqrtl);
					Serial.print("\tSqrt2 ");
					Serial.println(time_sqrtl2);
				break;

				case '?':
				case 'h':
					cmd_print_help();
				break;

				default: // -------------------------------
					Serial.print("Unexpected: ");
					Serial.print((char)input);
					cmd_print_help();
			}
			Serial.print("> ");
		}
#endif
                // Wait for other data
		flag = 0;
	}
}
