/*
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
  Notes & Calculations:

  This code was tested and working using Arduino 1.6.8 and Arduino 1.8.5
  It works on Arduinos using ATmega328 and ATmega168P (Arduino Pro-Mini)

  IMPORTANT: to avoid constant reboots upon an I2C error (which sometimes can happen) replace the twi.c file in your arduino folder 
  (YOURARDUINOLOCATION\hardware\arduino\avr\libraries\Wire\src\utility) with the twi.c file in the zip file provided.
  it contains my bugfix to not get the arduino stuck in an infinite loop when the accelerometer does not respond

  solar cell power:
  the solar cell delivers around 3-5mA when charging in the shade or light overcast. In full sunlight, it is probably more like 50mA.
  when charging for about 10 hours on an average of 5mA this equals only 50mAh which powers the LEDs on reduced brightness for about two hours (assuming 25mA current). todo: test this some more

  Note: need to mention that only Atmega 328P and ATmega 168PA work with this code (tested, all processors with micro power work but may need an adjusted low power function)

  Power consumption:
  -the ATMEGA368P boards and ATMEGA168PA boards use about 5uA  during low power sleep
  -the finished circuit consumes 8.5uA during sleep, 8.5mA when running (without LED on) and about 0.25mA with the accelerometer running and the MCU in low power sleep
    the run times during wakeup are: 7.6+0.3ms in run mode 820us in power up mode (charging the cap of the accelerometer at about 17mA) and 250ms in low power with the accelerometer on
    the wake period is followed by 1000ms of low power sleep at 8.5uA
    that totals in approximately 9.5ms*8.5mA+250ms*0.25mA+1000*0.0085mA = 152mAms resulting in an average current of 120uA

  At the average current of 120uA a 500mAh battery will last almost half a year if not re-charged (consumption of 2.88mAh per day)
  A solar panel delivering just 5mA for a few hours a day is enough to slowly charge the battery!
  Small 5.5V solar panels deliver a lot more power than this if placed in sunlight. Even on an overcast sky the 5mA are easily reached.


  todo:
  -could add a battery charge monitoring function that checks the voltage difference in intervals of 1h and if it never rises goes into an ultra low power mode, saving the battery if there is no power source available (i.e. it is not placed somewhere bright)

*/

#include <Wire.h>
#include <FastLED.h>

#include <SparkFun_ADXL345.h>         // SparkFun ADXL345 Library
//low power stuff (cannot use low power library as it does not support ATMEGA168P devices)
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>


ADXL345 adxl = ADXL345();

//#define SERIALDEBUG 1 //serial output debugging data if defined
#define RANDOMCOLORATWAKEUP 1 //if defined, a random color is assigned at auto-wakeup

#define AUTOPOWEROFFTIME 200 //time in minutes after which the light turns off automatically 

#define LED_PIN     7 //LED data pin
#define LEDPWR_PIN  6 //LED power pin (inverting, low means on)
#define ACC_PIN     5 //power pin for accelerometer
#define BATTERYVOLTAGE_PIN A0
#define SOLARVOLTAGE_PIN A1

#define COLOR_ORDER GRB
#define CHIPSET     WS2812B
#define NUM_LEDS    3 //number of leds connected-

#define BRIGHTNESS  180 //manual-on brightness setting
#define AUTOMAXBRIGHTNESS  200 //auto-on maximum brightness setting
#define AUTOMINBRIGHTNESS  20 //auto-on minimum brightness setting


#define STATE_CHANGECOLOR     0
#define STATE_CHANGEBRIGHTNES     1
#define STATE_CHANGESATURATION     2

//LED modes:
#define STATICMODE 0
#define CANDLEMODE 1 //if a mode is added, change LASTMODE to the new mode (used for mode counting)
#define LASTMODE CANDLEMODE

//global variables
uint8_t led_state;
bool running = false; //low power mode if not running, set to true once ADXL tap interrupt is detected, led will then be switched on
bool gravitycolor_active = true; //if active, tilting changes the color. is set inactive after 30s of idling
bool wakeup = false;
bool autoOn = false; //set to true once the light is switched on automatically, set back to false once it is no more dark outside (at dusk), also disabled on manual on
byte itsDarkOutside = 0; //counts up if darkness is detected (by checking solar panel voltage), down if not
bool tap_detect = false;
bool doubletap_detect = false;

bool voltageLow = false;
long ontimeCounter = 0; //counter to track on-time
unsigned int minutecounter = 0; //approximately counts the on-time minutes (derived from ontimeCounter)
int lowPowerCheckCounter = 0; //counter to check voltages when not running (for auto-on mode), incremented after low power sleep (~every 2 seconds)
byte switchoffcounter = 0; //switch off if turned upside down for more than one second

byte ledmode = 0; //start off with the first mode

//LED HSV color
CHSV ledcolor_hsv; //LED hsv color
CRGB leds[NUM_LEDS]; //data array for the RGB colors (these are sent out to the leds)




void setup() {

  MCUSR = 0; //reset MCU status reset register (must be cleared to disable watchdog reset, this may also be called in arduino core, did not check)
  WDTCSR |= (1 << WDCE); //watchdog change enable, must be set to be able to clear WDE bit
  WDTCSR &= ~(1 << WDE); //disable watchdog resetting the system (must be cleared after WDRF is cleared in MCUSR)
  wdt_disable(); //disable the watchdog timer (in case the watchdog interrupt did reset the system)

#ifdef SERIALDEBUG
  Serial.begin(115200);
  Serial.println("Garden Jar Light V1.0");
  delay(10);
#endif

  //blink the LED on the arduino on bootup (to check if it is constantly rebooting, hunting a bug where the light does not react to shaking until reset is pressed)
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  delay(200);
  digitalWrite(13, LOW);
  pinMode(13, INPUT);

  pinMode(LEDPWR_PIN, INPUT);
  digitalWrite(LEDPWR_PIN, HIGH); //LEDs OFF  //todo: with a pullup, make this an input and only make it an output when running
  ledcolor_hsv.h = 18;
  ledcolor_hsv.s = 255;
  ledcolor_hsv.v = 255;
  analogReference(INTERNAL);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(BRIGHTNESS);
  randomSeed(getBatteryVoltage()+getBatteryVoltage()+getBatteryVoltage());
  powerDown(WDTO_2S); //wait 2 seconds in low power mode in case the ADXL345 is broken and the system reboots due to watchdog timeout this will save a lot of power, preventing the battery to die

}

void loop()
{
  static uint8_t delaycounter = 0; //counter for delayed tilting color-update

  if (running)
  {
    wdt_reset(); //kick the watchdog timer signalling the main loop is still running
    ontimeCounter++; //increment the on-time, main loop runs at 44Hz in normal mode and at 300Hz in candle mode

    // Accelerometer Readings
    int x, y, z;
    adxl.readAccel(&x, &y, &z);         // Read the accelerometer values and store them in variables declared above x,y,z


    if (z > 50) //jar is upside down (electronics facing upwards)
    {
      switchoffcounter++;
    }
    else
    {
      switchoffcounter = 0;
    }

    // Output Results to Serial
#ifdef SERIALDEBUG

    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.print(", ");
    Serial.println(z);
#endif

    ADXL_ISR(); //check accelerometer interrupts by polling

    if (tap_detect)
    {
      tap_detect = false;
      led_state++;
      if (led_state > STATE_CHANGESATURATION) led_state = STATE_CHANGECOLOR;
    }

    if (doubletap_detect)
    {
      doubletap_detect = false;
      tap_detect = false;
      led_state = STATE_CHANGECOLOR; //go to color changing mode upon double tap
      ledmode++;
      if (ledmode > LASTMODE)
      {
        ledmode = 0;
        //also set high brightness and full saturation
        ledcolor_hsv.v = 255;
        ledcolor_hsv.s = 255;
      }


    }
    if (gravitycolor_active)
    {
      int tilting;
      if (abs(x) >= abs(y)) tilting = x;
      else tilting = y;
      tilting = tilting / 4;
      tilting = constrain(tilting, -15, 15); //limit x so no overflows happen below



      if (tilting > 10)
      {
        if (delaycounter > (15 - tilting))
        {
          delaycounter = 0;
          if (led_state == STATE_CHANGECOLOR)
          {
            ledcolor_hsv.h++;
          }
          if (led_state == STATE_CHANGEBRIGHTNES)
          {
            if (ledcolor_hsv.v < 254) //no overflow permitted!
              ledcolor_hsv.v += 2;
          }
          if (led_state == STATE_CHANGESATURATION)
          {
            if (ledcolor_hsv.s < 251
               ) //no overflow permitted!
              ledcolor_hsv.s += 5;
          }
        }
      }
      else if ( tilting < -10)
      {
        if (delaycounter > (15 + tilting))
        {
          delaycounter = 0;
          if (led_state == STATE_CHANGECOLOR)
          {
            ledcolor_hsv.h--;
          }
          if (led_state == STATE_CHANGEBRIGHTNES)
          {
            if (ledcolor_hsv.v > 90) //no overflow permitted, no low values permitted
              ledcolor_hsv.v -= 2;
          }
          if (led_state == STATE_CHANGESATURATION)
          {
            if (ledcolor_hsv.s > 30) //no overflow permitted, no zero permitted
              ledcolor_hsv.s -= 5;
          }
        }
      }
    }

    switch (ledmode)
    {
      case STATICMODE:
        staticUpdate();
        powerDown(WDTO_15MS);
        if (ontimeCounter % (44 * 60) == 0) //execuded about once every minute, normal mode runs at 44Hz
        {
          minutecounter++;
          checkVoltages(); //check battery voltage once per minute
        }
        break;
      case CANDLEMODE:
        candleUpdate(ledcolor_hsv.h); //candle with current led color as base hue
        delay(3);
        if (ontimeCounter % (300 * 60) == 0) //candle mode runs at about 300Hz
        {
          minutecounter++;
          checkVoltages(); //check battery voltage once per minute

        }
        break;
      //add other modes here
      default:
        staticUpdate();
        break;
    }



    delaycounter++;

    if (switchoffcounter > 50) //switch led off if turned upside down for some time
    {
      switchLEDoff(true);
      // LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); //sleep so the light can be set back down after flipping it upside down
      powerDown(WDTO_4S);
    }


    if (minutecounter > AUTOPOWEROFFTIME)
    {
      switchLEDoff(true);
    }

    if (voltageLow)
    {
      lowVoltageWarning(); //blink and shutdown
    }

  }


  /*****************************
     LOW POWER MODE STARTS HERE
   *****************************/

  else //low power mode, check accelerometer every 2 seconds
  {

#ifdef SERIALDEBUG
    delay(10); //wait for serial prints to finish
#endif
    wakeup = false; //reset wakeup variable
    if (voltageLow == false) //battery voltage is ok, check accelerometer
    {
      adxl_setup(); //startup the accelerometer
      ADXL_ISR(); //check accelerometer interrupts by polling
      wakeup = false;
      powerDown(WDTO_250MS);
      ADXL_ISR(); //check accelerometer interrupts by polling (sets wakeup if shaking is detected)
    }
    if (wakeup)
    {
      FastLED.setBrightness(getLongrunningBrightness(getBatteryVoltage()));
      //FastLED.setBrightness(BRIGHTNESS); //set default brightness
      switchLEDon(true);
      powerDown(WDTO_2S); //wait for accelerometer to calm down
      ADXL_ISR(); //clear interrupts
      led_state = STATE_CHANGECOLOR;
      tap_detect = false;
      doubletap_detect = false;
      autoOn = false; //this is a manual on

    }
    else //no wakup shaking detected, go back to sleep (or turn led on if it is getting dark outside)
    {
#ifdef SERIALDEBUG
      Serial.println("no wakeup signal");
      delay(50); //wait for serial prints to finish
      return;//do not go to sleep
#endif
      adxl_powerdown(); //set accelerometer power power and I2C pins low
      if (voltageLow)
      {
        powerDown(WDTO_8S);
      }
      else
      {
        powerDown(WDTO_1S);
      }

      lowPowerCheckCounter++;

      if (lowPowerCheckCounter > 60) //check about once every minute (or every 8 minutes if battery voltage is low)
      {

        lowPowerCheckCounter = 0;
        checkVoltages(); //check input voltages (and determine day and night time, switch on if dark for some time)
        if (voltageLow)
        {
          lowVoltageWarning(); //blink as a warning every 8 minutes if voltage is low
        }

      }
    }

  }
}



