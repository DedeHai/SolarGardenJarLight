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

