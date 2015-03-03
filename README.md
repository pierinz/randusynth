# randusynth
A "something-to-synth" converter based on Arduino DUE  
Connect a guitar to it and start making noise!

## DEFAULT WIRING CONFIGURATION:
 - connect the guitar/line in/whatever to pin A0. Use a preamp if needed. The input MUST be between 0 and 3.3V.  
DO NOT EXCEED 3.3V
 - connect DAC1 to an amplifier.  
You can follow this: [http://arduino.cc/en/Tutorial/SimpleAudioPlayer]
 - (optional) clipping indicator: connect a LED to pin 10

You can change some of this in the sketch (see the SKETCH CONFIGURATION part)

## LIBRARIES:
You'll need "SplitRadixReal FFT Library" and my own fork of Groovuino:
 - [https://coolarduino.wordpress.com/2014/09/25/splitradixreal-fft-library/]
 - [https://github.com/pierinz/Groovuino]

## CREDITS:
This sketch was built around the "SplitRadixReal FFT Library" example from
[https://coolarduino.wordpress.com/2014/09/25/splitradixreal-fft-library/]  
I just hacked it and added code to fit my needs.
All the "audio input part" and FFT calculations came from this.

The synth sound output functions came from [http://groovuino.blogspot.it/] - [https://github.com/Gaetino/Groovuino]  
I slightly modified the library, now it works on Linux and some variables
are overridable in the sketch - see https://github.com/pierinz/Groovuino

I should mention all people that led me here with their projects:
 - Amanda Ghassaei ([http://www.instructables.com/member/amandaghassaei/])
 - All the Auduino authors ([https://code.google.com/p/tinkerit/wiki/Auduino])
 - All the geek friends that were interested in "this thing"

Thank you!
