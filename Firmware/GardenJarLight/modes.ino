
/*************************
 SOLAR GARDEN JAR LIGHT 
 
 by Damian Schneider
 *************************

 * Copyright (c) Damian Schneider 2017
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */



/*
   LED modes
*/


//candle like flickering
void candleUpdate(uint8_t basehue) {
  static CHSV fadetocolor;
  static CHSV candlecolor;
  static uint8_t updatecounter = 255;
  static uint8_t randominterval = 0;
  static uint8_t firstrun = 1;

  if (firstrun)
  {
    firstrun = 0;
    candlecolor = ledcolor_hsv;
  }

  if (updatecounter > 10 + randominterval) {
    updatecounter = 0;
    randominterval = random(100);
    //generate new random endcolor:
    fadetocolor.h = basehue + (uint8_t) random(12);
    fadetocolor.s = 255 - (uint8_t) random(50);
    fadetocolor.v = 255 - (uint8_t) random(100);
  }
  //todo: make hue fade at about same speed as value and couple the values

  uint8_t fadediff = fadetocolor.h - candlecolor.h; //the two values never differ more than 12

  if (updatecounter % 5 == 0) //fade hue and saturation slower than brightness, its values do not change by that much
  {
    if (fadediff > 1 && fadediff < 128) //no overflow in subtraction
      candlecolor.h++;
    else if (fadediff > 1 && fadediff > 128) //overflow happened in subtraction
      candlecolor.h--;

    if (candlecolor.s < fadetocolor.s)
      candlecolor.s++;
    else
      candlecolor.s--;
  }
  if (candlecolor.v < fadetocolor.v)
    candlecolor.v++;
  else
    candlecolor.v--;

  updatecounter++;
  updateLEDs(candlecolor);
}


void staticUpdate(void)
{
  updateLEDs(ledcolor_hsv);
}

