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



/*
   Functions to monitor solar cell and battery voltages
   input voltage has a 11:1 voltage divider (10M and 1M resistor) and the internal reference voltage is 1.1V
*/
#define VOLTAGEATDAWN 700 //solar cell voltage threshold indicating it is now dark outside
#define VOLTAGEDAYLIGHT 3000 //solar cell voltage threshold to determine broad daylight
#define BATTERYMINVOLTAGE 3500 //battery voltage in mV when the auto-on at dawn will switch off 
#define BATTERYONVOLTAGE 3700 //battery voltage in mV that is (minimally) required to perform auto-switch on at dawn (set to same as BATTERYMINVOLTAGE to always switch on)
#define BATTERYCRITICALVOLTAGE 3400 //critical battery voltage in mV, below this, accelerometer is not checked anymore, low power interval is maximized (battery is almost empty if this dicharge voltage is reached)


//voltage in [mV]
unsigned int getBatteryVoltage(void)
{
  unsigned int adcValue = analogRead(BATTERYVOLTAGE_PIN);
#ifdef SERIALDEBUG
  unsigned int outvoltage = (unsigned int)(((long)adcValue * 1100 * 11) >> 10);
  Serial.print("battery voltage: ADC = ");
  Serial.print(adcValue);
  Serial.print(" mV = ");
  Serial.println(outvoltage);
#endif
  return (unsigned int)(((long)adcValue * 1100 * 11) >> 10);

}

//voltage in [mV]
unsigned int getSolarVoltage(void)
{
  unsigned int adcValue = analogRead(SOLARVOLTAGE_PIN);
#ifdef SERIALDEBUG
  unsigned int outvoltage = (unsigned int)(((long)adcValue * 1100 * 11) >> 10);
  Serial.print("solar voltage: ADC = ");
  Serial.print(adcValue);
  Serial.print("  mV =");
  Serial.println(outvoltage);
#endif
  return (unsigned int)(((long)adcValue * 1100 * 11) >> 10);
}

bool checkDarkness(void)
{
  if (getSolarVoltage() < (unsigned int)VOLTAGEATDAWN)
    return true;
  else
    return false;
}

bool checkBroadDaylight(void)
{
  if (getSolarVoltage() > (unsigned int)VOLTAGEDAYLIGHT)
    return true;
  else
    return false;
}

bool checkAutoOff(void)
{
  if (getBatteryVoltage() < BATTERYMINVOLTAGE)
    return true;
  else
    return false;
}

bool checkLowVoltage(void)
{
  static uint8_t lowvoltagecounter = 0; //used to detect low voltage multiple times to make sure it is not just a noise spike
  if (getBatteryVoltage() < BATTERYCRITICALVOLTAGE)
  {
    lowvoltagecounter++;
    if (lowvoltagecounter > 3)
    {
      lowvoltagecounter = 4;
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    lowvoltagecounter = 0;
    return false;
  }
}

//map the battery voltage to a brightness level so the light will stay on for the longest time depending on the juice left
uint8_t getLongrunningBrightness(unsigned int batVoltage)
{
   //calculate brightness from battery voltage: at 3.7V use minimum brightness, above 4.1V use high brightness (=AUTOMAXBRIGHTNESS)
        unsigned int brightnessvalue = map(batVoltage, BATTERYONVOLTAGE, 4250,AUTOMINBRIGHTNESS , AUTOMAXBRIGHTNESS);
        brightnessvalue = constrain(brightnessvalue, AUTOMINBRIGHTNESS,AUTOMAXBRIGHTNESS);  

        return (uint8_t)brightnessvalue;
}

//handle voltage checking, determine day and night time, execute about once per minute
void checkVoltages(void)
{

  //check if it's dark, switch on if it is

  //check if it's dark outside (check this once per minute)
  if (checkDarkness() && itsDarkOutside < 30) {
    itsDarkOutside++;
  }
  else if (itsDarkOutside > 0)
  {
    itsDarkOutside--;
  }

  //#ifdef SERIALDEBUG
  //  Serial.print("itsDarkOutside = ");
  //  Serial.println(itsDarkOutside);
  //#endif

  if (running == false) //only check auto-on if not already running
  {
    if (itsDarkOutside > 10) //check if dark for more than 10 minutes and not yet run
    {
      unsigned int currentBatteryVoltage = getBatteryVoltage();
      if (autoOn == false && (currentBatteryVoltage > BATTERYONVOLTAGE) )
      {
        autoOn = true; //automatically turned on the light       
        FastLED.setBrightness(getLongrunningBrightness(currentBatteryVoltage)); //set to lower brightness so the harnessed power lasts longer

#ifdef RANDOMCOLORATWAKEUP
        ledcolor_hsv.h = random(255); //turn on using random color
        ledcolor_hsv.s = 255;  //full color
        ledcolor_hsv.v = 255;  //full brightness (still limited by global 'setBrightness' value)
        ledmode = CANDLEMODE;
#endif
        switchLEDon(true);
        adxl_setup(); //startup the accelerometer
        //clear interrupts
        ADXL_ISR();
        tap_detect = false;
        doubletap_detect = false;
      }
    }
    else if (itsDarkOutside < 5) //daylight
    {
      autoOn = false; //reset for next time it gets dark
    }
  }
  else //light is running, check if it is bright daylight outside, switch off if it is
  {
    if (checkBroadDaylight())
    {
      switchLEDoff(true); //fadeout and shut down
    }

  }

  if (checkLowVoltage())
  {
    voltageLow = true;
  }
  else
  {
    voltageLow = false;
  }

  if (checkAutoOff() && autoOn && voltageLow == false) //check for minimum auto-mode battery voltage (only switch off if voltage is not low, let the main show the warning, then switch off)
  {
    switchLEDoff(true); //fadeout and shut down
  }

}

