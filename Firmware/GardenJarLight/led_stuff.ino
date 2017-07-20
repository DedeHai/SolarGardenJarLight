/*************************
  SOLAR GARDEN JAR LIGHT

  by Damian Schneider
 *************************

   Copyright (c) Damian Schneider 2017

   This program is free software: you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free Software
   Foundation, either version 3 of the License, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
   details.

   You should have received a copy of the GNU General Public License along with
   this program. If not, see <http://www.gnu.org/licenses/>.
*/


void updateLEDs(CHSV sendcolor)
{
  fill_solid(leds, NUM_LEDS, sendcolor);
  FastLED.show(); // display this frame
}


//fade out and switch leds of
void switchLEDoff(bool usefadeout)
{
  uint8_t startbrightness = ledcolor_hsv.v;
  uint8_t i;
  if (usefadeout)
  {
    for (i = ledcolor_hsv.v; i > 0; i--)
    {
      ledcolor_hsv.v = i;
      updateLEDs(ledcolor_hsv);
      delay(9);
      wdt_reset(); //kick the watchdog
    }
  }
  ledcolor_hsv.v = startbrightness; //set the global variable back to the previous brightness
  running = false;
  autoOn = true; //was running, let the daylight reset this variable to false (i.e. do not switch back on if it is dark outside)
  pinMode(LEDPWR_PIN, INPUT);
  digitalWrite(LEDPWR_PIN, HIGH); //LEDs OFF  //todo: with a pullup, make this an input and only make it an output when running
  adxl_powerdown(); //set accelerometer power power and I2C pins low
}

//fade in and go to running mode
void switchLEDon(bool usefadein)
{
  //todo: fade in
  pinMode(LEDPWR_PIN, OUTPUT);
  digitalWrite(LEDPWR_PIN, LOW); //LEDs ON
  delay(2); //wait for led voltage to be stable
  running = true;
  minutecounter = 0; //reset minute counter
  //led fade in to currently set brightness
  uint8_t endbrightness = ledcolor_hsv.v;
  uint8_t i;
  if (usefadein)
  {
    for (i = 0; i < endbrightness; i++)
    {
      ledcolor_hsv.v = i;
      updateLEDs(ledcolor_hsv);
      delay(7);
      wdt_reset(); //kick the watchdog
    }
  }

}


void lowVoltageWarning(void) {
  uint8_t i;
  CHSV warnincolor;
  switchLEDon(false);

  warnincolor.h = 0; //blink in red

  warnincolor.s = 255;

  for (i = 0; i < 5; i++) {
    warnincolor.v = 220;
    updateLEDs(warnincolor);
    //  LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
    powerDown(WDTO_60MS);
    warnincolor.v = 0;
    updateLEDs(warnincolor);
    //   LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
    powerDown(WDTO_250MS);
  }
  switchLEDoff(false);
}

