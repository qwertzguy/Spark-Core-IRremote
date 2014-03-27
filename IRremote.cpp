/*
 * IRremote
 * Version 0.11 August, 2009
 * Copyright 2009 Ken Shirriff
 * For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.html
 *
 * Modified by Paul Stoffregen <paul@pjrc.com> to support other boards and timers
 * Modified  by Mitra Ardron <mitra@mitra.biz> 
 * Added Sanyo and Mitsubishi controllers
 * Modified Sony to spot the repeat codes that some Sony's send
 * Modified by Gaspard van Koningsveld to trim out IRrecv, not using PWM anymore, allow setting of IR LED pin, and make it compatible with the Spark Core v1.0 (STM32F103CB based)
 *
 * Interrupt code based on NECIRrcv by Joe Knapp
 * http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
 * Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
 *
 * JVC and Panasonic protocol added by Kristian Lauszus (Thanks to zenwheel and other people at the original blog post)
 */

#include "IRremote.h"
#include "application.h"

IRsend::IRsend(int irPin) : irPin(irPin) {};

void IRsend::sendNEC(unsigned long data, int nbits)
{
  enableIROut(38);
  mark(NEC_HDR_MARK);
  space(NEC_HDR_SPACE);
  for (int i = 0; i < nbits; i++) {
    if (data & TOPBIT) {
      mark(NEC_BIT_MARK);
      space(NEC_ONE_SPACE);
    } 
    else {
      mark(NEC_BIT_MARK);
      space(NEC_ZERO_SPACE);
    }
    data <<= 1;
  }
  mark(NEC_BIT_MARK);
  space(0);
}

void IRsend::sendSony(unsigned long data, int nbits) {
  enableIROut(40);
  mark(SONY_HDR_MARK);
  space(SONY_HDR_SPACE);
  data = data << (32 - nbits);
  for (int i = 0; i < nbits; i++) {
    if (data & TOPBIT) {
      mark(SONY_ONE_MARK);
      space(SONY_HDR_SPACE);
    } 
    else {
      mark(SONY_ZERO_MARK);
      space(SONY_HDR_SPACE);
    }
    data <<= 1;
  }
}

void IRsend::sendRaw(unsigned int buf[], int len, int hz)
{
  enableIROut(hz);
  for (int i = 0; i < len; i++) {
    if (i & 1) {
      space(buf[i]);
    } 
    else {
      mark(buf[i]);
    }
  }
  space(0); // Just to be sure
}

// Note: first bit must be a one (start bit)
void IRsend::sendRC5(unsigned long data, int nbits)
{
  enableIROut(36);
  data = data << (32 - nbits);
  mark(RC5_T1); // First start bit
  space(RC5_T1); // Second start bit
  mark(RC5_T1); // Second start bit
  for (int i = 0; i < nbits; i++) {
    if (data & TOPBIT) {
      space(RC5_T1); // 1 is space, then mark
      mark(RC5_T1);
    } 
    else {
      mark(RC5_T1);
      space(RC5_T1);
    }
    data <<= 1;
  }
  space(0); // Turn off at end
}

// Caller needs to take care of flipping the toggle bit
void IRsend::sendRC6(unsigned long data, int nbits)
{
  enableIROut(36);
  data = data << (32 - nbits);
  mark(RC6_HDR_MARK);
  space(RC6_HDR_SPACE);
  mark(RC6_T1); // start bit
  space(RC6_T1);
  int t;
  for (int i = 0; i < nbits; i++) {
    if (i == 3) {
      // double-wide trailer bit
      t = 2 * RC6_T1;
    } 
    else {
      t = RC6_T1;
    }
    if (data & TOPBIT) {
      mark(t);
      space(t);
    } 
    else {
      space(t);
      mark(t);
    }

    data <<= 1;
  }
  space(0); // Turn off at end
}

/* Sharp and DISH support by Todd Treece ( http://unionbridge.org/design/ircommand )

The Dish send function needs to be repeated 4 times, and the Sharp function
has the necessary repeat built in because of the need to invert the signal.

Sharp protocol documentation:
http://www.sbprojects.com/knowledge/ir/sharp.htm

Here are the LIRC files that I found that seem to match the remote codes
from the oscilloscope:

Sharp LCD TV:
http://lirc.sourceforge.net/remotes/sharp/GA538WJSA

DISH NETWORK (echostar 301):
http://lirc.sourceforge.net/remotes/echostar/301_501_3100_5100_58xx_59xx

For the DISH codes, only send the last for characters of the hex.
i.e. use 0x1C10 instead of 0x0000000000001C10 which is listed in the
linked LIRC file.
*/
void IRsend::sendSharp(unsigned long data, int nbits) {
  unsigned long invertdata = data ^ SHARP_TOGGLE_MASK;
  enableIROut(38);
  for (int i = 0; i < nbits; i++) {
    if (data & 0x4000) {
      mark(SHARP_BIT_MARK);
      space(SHARP_ONE_SPACE);
    }
    else {
      mark(SHARP_BIT_MARK);
      space(SHARP_ZERO_SPACE);
    }
    data <<= 1;
  }
  
  mark(SHARP_BIT_MARK);
  space(SHARP_ZERO_SPACE);
  delay(46);
  for (int i = 0; i < nbits; i++) {
    if (invertdata & 0x4000) {
      mark(SHARP_BIT_MARK);
      space(SHARP_ONE_SPACE);
    }
    else {
      mark(SHARP_BIT_MARK);
      space(SHARP_ZERO_SPACE);
    }
    invertdata <<= 1;
  }
  mark(SHARP_BIT_MARK);
  space(SHARP_ZERO_SPACE);
  delay(46);
}

void IRsend::sendDISH(unsigned long data, int nbits)
{
  enableIROut(56);
  mark(DISH_HDR_MARK);
  space(DISH_HDR_SPACE);
  for (int i = 0; i < nbits; i++) {
    if (data & DISH_TOP_BIT) {
      mark(DISH_BIT_MARK);
      space(DISH_ONE_SPACE);
    }
    else {
      mark(DISH_BIT_MARK);
      space(DISH_ZERO_SPACE);
    }
    data <<= 1;
  }
}

void IRsend::sendPanasonic(unsigned int address, unsigned long data) {
    enableIROut(35);
    mark(PANASONIC_HDR_MARK);
    space(PANASONIC_HDR_SPACE);
    
    for(int i=0;i<16;i++)
    {
        mark(PANASONIC_BIT_MARK);
        if (address & 0x8000) {
            space(PANASONIC_ONE_SPACE);
        } else {
            space(PANASONIC_ZERO_SPACE);
        }
        address <<= 1;        
    }    
    for (int i=0; i < 32; i++) {
        mark(PANASONIC_BIT_MARK);
        if (data & TOPBIT) {
            space(PANASONIC_ONE_SPACE);
        } else {
            space(PANASONIC_ZERO_SPACE);
        }
        data <<= 1;
    }
    mark(PANASONIC_BIT_MARK);
    space(0);
}

void IRsend::sendJVC(unsigned long data, int nbits, int repeat)
{
    enableIROut(38);
    data = data << (32 - nbits);
    if (!repeat){
        mark(JVC_HDR_MARK);
        space(JVC_HDR_SPACE); 
    }
    for (int i = 0; i < nbits; i++) {
        if (data & TOPBIT) {
            mark(JVC_BIT_MARK);
            space(JVC_ONE_SPACE); 
        } 
        else {
            mark(JVC_BIT_MARK);
            space(JVC_ZERO_SPACE); 
        }
        data <<= 1;
    }
    mark(JVC_BIT_MARK);
    space(0);
}

void IRsend::mark(int time) {
  // Sends an IR mark (frequency burst output) for the specified number of microseconds.
  noInterrupts();
  
  while (time > 0) {
    digitalWrite(irPin, HIGH); // this takes about 3 microseconds to happen
    delayMicroseconds(burstWait);
    digitalWrite(irPin, LOW); // this also takes about 3 microseconds
    delayMicroseconds(burstWait);
 
    time -= burstLength;
  }
  
  interrupts();
}

void IRsend::space(int time) {
  // Sends an IR space (no output) for the specified number of microseconds.
  digitalWrite(irPin, LOW); // Takes about 3 microsecondes
  if (time > 3) {
    delayMicroseconds(time - 3);
  }
}

void IRsend::enableIROut(int khz) {
  // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
  // MAX frequency is 166khz.
  pinMode(irPin, OUTPUT);
  digitalWrite(irPin, LOW);

  // This is the time to wait with the IR LED on and off to make the frequency, in microseconds.
  // The - 3.0 at the end is because digitalWrite() takes about 3 microseconds. Info from:
  // https://github.com/eflynch/sparkcoreiremitter/blob/master/ir_emitter/ir_emitter.ino
  burstWait = round(1.0 / khz * 1000.0 / 2.0 - 3.0);
  // This is the total time of a period, in microseconds.
  burstLength = round(1.0 / khz * 1000.0);
}
