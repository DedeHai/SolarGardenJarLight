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
#define VOLTAGEATDAWN 900 //solar cell voltage threshold indicating it is now dark outside
#define BATTERYMINVOLTAGE 3800 //battery voltage in mV when the auto-on at dawn will switch off 
#define BATTERYONVOLTAGE 3900 //battery voltage in mV that is (minimally) required to perform auto-switch on at dawn (set to same as BATTERYMINVOLTAGE to always switch on)
#define BATTERYCRITICALVOLTAGE 3600 //critical battery voltage in mV, below this, accelerometer is not checked anymore, low power interval is maximized (battery is almost empty if this dicharge voltage is reached)


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

bool checkAutoOff(void)
{
  if (getBatteryVoltage() < BATTERYMINVOLTAGE)
    return true;
  else
    return false;
}

bool checkLowVoltage(void)
{
  if (getBatteryVoltage() < BATTERYCRITICALVOLTAGE)
    return true;
  else
    return false;
}

//handle voltage checking, determine day and night time, execute about once per minute
void checkVoltages(void)
{

#ifdef AUTOONATDAWN
  //check if it's dark, switch on if it is

  //check if it's dark outside (check this once per minute)
  if (checkDarkness() && itsDarkOutside < 30) {
    itsDarkOutside++;
  }
  else if (itsDarkOutside > 0)
  {
    itsDarkOutside--;
  }

#ifdef SERIALDEBUG
  Serial.print("itsDarkOutside = ");
  Serial.println(itsDarkOutside);
#endif

  if (running == false) //only check auto-on if not already running
  {
    if (itsDarkOutside > 10) //check if dark for more than 10 minutes and not yet run
    {
      if (autoOn == false && (getBatteryVoltage() > BATTERYONVOLTAGE) )
      {
        autoOn = true; //automatically turned on the light
        FastLED.setBrightness(AUTOONBRIGHTNESS); //set to lower brightness so the harnessed power lasts longer
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

  if (checkAutoOff() && autoOn) //check for minimum auto-mode battery voltage
  {
    switchLEDoff(true); //fadeout and shut down
  }

#endif

  if (checkLowVoltage())
  {
    voltageLow = true;
  }
  else
  {
    voltageLow = false;
  }
}

