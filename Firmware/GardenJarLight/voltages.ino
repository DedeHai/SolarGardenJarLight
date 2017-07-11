/*
   Functions to monitor solar cell and battery voltages
   input voltage has a 11:1 voltage divider (10M and 1M resistor) and the internal reference voltage is 1.1V
*/
#define VOLTAGEATDAWN 1000 //solar cell voltage threshold indicating it is now dark outside
#define BATTERYMINVOLTAGE 3800 //battery voltage in mV when the auto-on at dawn will switch off (or not even on)
#define BATTERYONVOLTAGE 4000 //battery voltage in mV that is (minimally) required to perform auto-switch on at dawn (set to same as BATTERYMINVOLTAGE to always switch on)
#define BATTERYCRITICALVOLTAGE 3650 //critical battery voltage in mV, below this, accelerometer is not checked anymore, low power interval is maximized (battery is almost empty if this dicharge voltage is reached)




//voltage in [mV]
unsigned int getBatteryVoltage(void)
{
  unsigned int adcValue = analogRead(BATTERYVOLTAGE_PIN);
  #ifdef SERIALDEBUG
  unsigned int outvoltage = (unsigned int)(((long)adcValue * 1100 * 11) >> 10);
  Serial.print("battery voltage = ");
       Serial.print(adcValue);
    Serial.print("  ");
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
    Serial.print("solar voltage = ");
    Serial.print(adcValue);
    Serial.print("  ");
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
  if (checkDarkness() && itsDarkOutside < 60) {
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

if(running == false) //only check auto-on if not already running
{
  if (itsDarkOutside > 10) //check if dark for more than 10 minutes and not yet run
  {

    if (autoOn == false && (getBatteryVoltage() > BATTERYONVOLTAGE) )
    {
      autoOn = true; //automatically turned on the light
      ledcolor_hsv.v = 130; //set to low brightness so the harnessed power lasts longer
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

  if(checkAutoOff() && autoOn) //check for minimum auto-mode battery voltage
  {
    switchLEDoff(true);
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

