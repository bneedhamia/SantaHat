/*
 * An Arduino Mini Pro based flashing Santa Hat.
 * A 24-hour-wonder project, so it's a bit of a hack.
 * 
 * This sketch blinks a set of LEDs that are glued into the puff ball
 * at the top of a Santa Hat (a red gnome hat with white trim and a white
 * ball on top).
 * 
 * Parts Required:
 * - 1 Arduino Pro Mini
 * - 1 battery power supply (I used 2 AA batteries, to produce 3V power)
 * - 1 power switch (I used a Molex connector as a switch)
 * - 4 red LEDs
 * - 4 100 ohm resistors, each connected to one LED. (at 3v = 30mA current)
 * - A Santa Hat
 * - glue gun, soldering, heat shrink tubing, needle and thread, etc.
 * 
 * To Use, simply connect the power supply to the Arduino and don the hat.
 * 
 * Copyright (C) 2016 Bradford H. Needham, North Plains, Oregon, USA
 * Licensed under the GNU General Public License V2, a copy of which
 * should have been supplied with this file.
 */
// 80 column marker follows:
//345678901234567890123456789012345678901234567895123456789612345678971234567898

/*
 * I/O Pins:
 * Indexed by a position (0..3 clockwise), contains the pin number of
 * the corresponding LED.
 * 
 * Change these pin numbers as necessary, depending on how your circuit
 * is wired.
 */
const int NUM_LEDS = 4;
const int P[] = { 3, 4, 6, 5};

/*
 * LED duty cycle (time on vs. off) constants.
 * 
 * US_PER_FRAME = the total time per unit of duty cycle
 *   (on time + off time), in microseconds. Chosen to
 *   reduce the perception that the lights are blinking quickly.
 * NUM_LED_LEVELS = number of brightness levels; number of entries in US_ON[].
 * US_ON[] = indexed by a desired brightness (0..7), contains
 *   the number of microseconds the LED should be on per Frame
 *   to produce that desired level of brightness.
 *   See the file brightness.ods to see how these numbers were calculated.
 */
const unsigned int US_PER_FRAME = 15000;
const int NUM_LED_LEVELS = 32;
const int US_ON[NUM_LED_LEVELS] = {
  // Minimum brightness (off) and power consumption
  0,
  1,
  7,
  22,
  49,
  91,
  151,
  233,
  338,
  470,
  631,
  824,
  1052,
  1316,
  1619,
  1965,
  2354,
  2789,
  3273,
  3808,
  4397,
  5040,
  5742,
  6503,
  7326,
  8213,
  9166,
  10188,
  11280,
  12445,
  13684,
  15000
 // Maximum brightness (and maximum power consumption)
};

void setup() {
  for (int i = 0; i < NUM_LEDS; ++i) {
    pinMode(P[i], OUTPUT);
  }
}
/*
 * Fade from one set of LEDs to another.
 *   from = the LEDs that begin at full brightness and fade to darkness.
 *     bit 0 = 1 means P[0], bit 1 = 1 means P[1], etc.
 *   to = the LEDs that begin dark and fade on to full brightness.
 *   mSecs = the time, in milliseconds, for the fade to take place.
 */
void fade(int from, int to, int mSecs) {
  float uSecs = (float) mSecs * 1000;
  
  // Round the time to a whole number of frames.
  unsigned int nFrames = (uSecs + (US_PER_FRAME / 2)) / US_PER_FRAME;
  
  float levelPerFrame = (float) NUM_LED_LEVELS / nFrames;
  
  float onLevelF = (float) NUM_LED_LEVELS;
  for (int frame = 0; frame < nFrames; ++frame) {

    /*
     * Convert the floating level into an integer level in our range,
     * then convert that level into a number of microseconds that
     * the from and to LEDs are to be on in this frame.
     */
    int onLevel = (int) (onLevelF + 0.5);
    if (onLevel < 0) {
      onLevel = 0;
    }
    if (onLevel > NUM_LED_LEVELS - 1) {
      onLevel = NUM_LED_LEVELS - 1;
    }
    unsigned int uSFrom = US_ON[(int) onLevel];
    unsigned int uSTo = US_ON[(NUM_LED_LEVELS - 1) - (int) onLevel];

    /*
     * Run the From and To duty cycles concurrently.
     * There are potentially 3 phases:
     * 1) From and To LEDs are on
     * 2) only one of From or To are on.
     * 3) neither From nor To are on.
     * Any of these 3 phases could be of zero microseconds duration.
     */

    // Phase 1: From and To are on; others are off.
    
    unsigned int uSAll = uSFrom;
    if (uSAll > uSTo) {
      uSAll = uSTo;
    }
    
    if (uSAll > 0) {
      int all = from | to;
      for (int ledNum = 0; ledNum < NUM_LEDS; ++ledNum) {
        if ((all & (1 << ledNum))) {
          digitalWrite(P[ledNum], HIGH);
        } else {
          digitalWrite(P[ledNum], LOW);
        }
      }
      
      delayMicroseconds(uSAll);
    }

    // Phase 2: either From or To are on; others are off
    unsigned int uSSome;
    if (uSFrom > uSAll || uSTo > uSAll) {
      
      if (uSFrom > uSTo) { // From is on; To and others are off
        uSSome = uSFrom - uSTo;
        
        for (int ledNum = 0; ledNum < NUM_LEDS; ++ledNum) {
          if ((from & (1 << ledNum))) {
            digitalWrite(P[ledNum], HIGH);
          } else {
            digitalWrite(P[ledNum], LOW);
          }
        }

      } else { // uSFrom < uSTo.  To is on; From and others are off.
        uSSome = uSTo - uSFrom;
        
        for (int ledNum = 0; ledNum < NUM_LEDS; ++ledNum) {
          if ((to & (1 << ledNum))) {
            digitalWrite(P[ledNum], HIGH);
          } else {
            digitalWrite(P[ledNum], LOW);
          }
        }
  
      }
      delayMicroseconds(uSSome);
    }

    // Phase 3: everything is off
    if (US_PER_FRAME > uSAll + uSSome) {
      unsigned int uSLeft = US_PER_FRAME - (uSAll + uSSome);

      for (int ledNum = 0; ledNum < NUM_LEDS; ++ledNum) {
        digitalWrite(P[ledNum], LOW);
      }

      delayMicroseconds(uSLeft);
    }

    // Finally, update the level
    onLevelF -= levelPerFrame;
  }
}

/*
 * move a single light counter-clockwise through all the LEDs.
 *   mSecs = time, in milliseconds, to accomplish the effect.
 * NOTE: Assumes P[0] is already on full, so that you can call
 * clockwise multiple times.
 */
void antiClockwise(int mSecs) {
  int mSPer = (mSecs + (4 / 2)) / 4;
  
  fade(0x1, 0x2, mSPer);
  fade(0x2, 0x4, mSPer);
  fade(0x4, 0x8, mSPer);
  fade(0x8, 0x1, mSPer);
}

/*
 * move a single light clockwise through all the LEDs.
 *   mSecs = time, in milliseconds, to accomplish the effect.
 * NOTE: Assumes P[0] is already on full, so that you can call
 * clockwise multiple times.
 */
void clockwise(int mSecs) {
  int mSPer = (mSecs + (4 / 2)) / 4;
  
  fade(0x1, 0x8, mSPer);
  fade(0x8, 0x4, mSPer);
  fade(0x4, 0x2, mSPer);
  fade(0x2, 0x1, mSPer);
}

void throb(int mSecs) {
  fade(0x0, 0xF, mSecs / 2);
  fade(0xF, 0x0, mSecs / 2);
}
void loop() {
  int i;

  for (i = 0; i < 10; ++i) {
    throb(2000);
  }
  
  fade(0x0, 0x1, 500);
  for (i = 0; i < 10; ++i) {
    clockwise(1000);
  }
  for (i = 0; i < 10; ++i) {
    antiClockwise(1000);
  }
  fade(0x1, 0x0, 500);
  
  delay(3000);
}
