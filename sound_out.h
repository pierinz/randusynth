/* Audio output functions */

// Include guard
#ifndef __SOUND_OUT_H
#define __SOUND_OUT_H

#ifdef TONE_SIMPLE
// TONE_SIMPLE outputs a simple square wave ignoring source amplitude
// -> Good for testing, not intended for production use

// Store here digital pin state
volatile byte buzz=0;

// Simple square wave (frequency determined by timer)
// -> simple tone() implementation for Arduino Due
void buzzer(){
	digitalWrite(tonepin,buzz);
	buzz=!buzz;
}
#else

#define NUM_OSC 1
#define SAMPLE_RATE 22000.0

// Initialize synth variables
Osc osc;

// Calculate next step of the synth wave(s) and write to DAC
void synth(){
	int32_t ulOutput=2048;

	osc.next();
	//Serial.print("osc: ");
	//Serial.println(osc.output());

	ulOutput += (osc.output());

	if(ulOutput>4095) ulOutput=4095;
	if(ulOutput<0) ulOutput=0;
	dacc_write_conversion_data(DACC_INTERFACE, ulOutput);
}
#endif

#endif
