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
Accelerometer related stuff
*/

#define ADXL345_DEVICE (0x53)    // Device Address for ADXL345
#define ADXL345_POWER_CTL    0x2D //ADXL345 power control register (needed to set the accelerometer to standby mode as library does not support this)

void adxl_setup(void)
{
#ifdef SERIALDEBUG
  Serial.print("ACC setup...");
  delay(10);
#endif

  //switch power pin on
  pinMode(ACC_PIN, OUTPUT);
  digitalWrite(ACC_PIN, HIGH);
  //wait for module to power up
  powerDown(WDTO_15MS);
  
  //accelerometer setup

  adxl.powerOn();                     // Power on the ADXL345
  powerDown(WDTO_30MS); //wait for power to stabilize
  Wire.setClock(400000); //set hig speed
  
  adxl.setRangeSetting(4);           // Give the range settings
  // Accepted values are 2g, 4g, 8g or 16g
  // Higher Values = Wider Measurement Range
  // Lower Values = Greater Sensitivity

  adxl.setActivityXYZ(1, 1, 0);       // Set to activate movement detection in the axes "adxl.setActivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setActivityThreshold(24);      // 62.5mg per increment   // Set activity   // Inactivity thresholds (0-255)

  adxl.setInactivityXYZ(1, 1, 0);     // Set to detect inactivity in the axes "adxl.setInactivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setInactivityThreshold(9);    // 62.5mg per increment   // Set inactivity // Inactivity thresholds (0-255)
  adxl.setTimeInactivity(30);         // How many seconds of no activity is inactive?

  adxl.setTapDetectionOnXYZ(0, 0, 1); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)

  // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
  adxl.setTapThreshold(40);           // 62.5 mg per increment
  adxl.setTapDuration(15);            // 625 μs per increment
  adxl.setDoubleTapLatency(80);       // 1.25 ms per increment
  adxl.setDoubleTapWindow(200);       // 1.25 ms per increment

  // Set values for what is considered FREE FALL (0-255)
  adxl.setFreeFallThreshold(7);       // (5 - 9) recommended - 62.5mg per increment
  adxl.setFreeFallDuration(30);       // (20 - 70) recommended - 5ms per increment

  // Setting all interupts to take place on INT1 pin
  //adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);"
  // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
  // This library may have a problem using INT2 pin. Default to INT1 pin.

  // Turn on Interrupts for each mode (1 == ON, 0 == OFF)
  adxl.InactivityINT(1);
  adxl.ActivityINT(1);
  adxl.FreeFallINT(1);
  adxl.doubleTapINT(1);
  adxl.singleTapINT(1);
  
#ifdef SERIALDEBUG
  Serial.println(" done");
  delay(10);
#endif

  //attachInterrupt(digitalPinToInterrupt(interruptPin), ADXL_ISR, RISING);   // Attach Interrupt pin if needed (no need to poll the ISR function if pin interrupt is used)
}

void adxl_powerdown(void)
{
  /*
    Wire.beginTransmission(ADXL345_DEVICE);
    Wire.write(ADXL345_POWER_CTL);        //write power control register
    Wire.write(0x07);              //enable standby mode, less than 1uA current consumption
    Wire.endTransmission();
    delay(10);
  */

  //disable the I2C module(not supported by arduino library so just clear the register bit)
  //note: if not disabled the wire library can get stuck in an infinite loop when disabling the pins (done below to save power)
  TWCR &= ~(1<< TWEN);


  //disable the ADXL345 module
  pinMode(ACC_PIN, INPUT);
  digitalWrite(ACC_PIN, LOW);
  
  //Disable I2C lines to save power (are re-initialized in adxl power up sequence)
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  digitalWrite(A4, LOW);
  digitalWrite(A5, LOW);



}

/********************* ISR *********************/
/* Look for Interrupts and Triggered Action    */
void ADXL_ISR() {
  //Serial.println("ISR");
  // getInterruptSource clears all triggered actions after returning value
  // Do not call again until you need to recheck for triggered actions
  byte interrupts = adxl.getInterruptSource();

  // Free Fall Detection
  if (adxl.triggered(interrupts, ADXL345_FREE_FALL)) {
#ifdef SERIALDEBUG
    Serial.println("*** FREE FALL ***");
#endif
    //add code here to do when free fall is sensed
  }

  // Inactivity
  if (adxl.triggered(interrupts, ADXL345_INACTIVITY)) {
#ifdef SERIALDEBUG
    Serial.println("*** INACTIVITY ***");
#endif
    //add code here to do when inactivity is sensed
    gravitycolor_active = false;
  }

  // Activity
  if (adxl.triggered(interrupts, ADXL345_ACTIVITY)) {
#ifdef SERIALDEBUG
    Serial.println("*** ACTIVITY ***");
#endif
    //add code here to do when activity is sensed
    wakeup = true;
    if (gravitycolor_active == false) //if inactive before, go to color changing mode
    {
      led_state = STATE_CHANGECOLOR;
    }

    gravitycolor_active = true;
  }

  // Double Tap Detection
  if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)) {
#ifdef SERIALDEBUG
    Serial.println("*** DOUBLE TAP ***");
#endif
    //add code here to do when a 2X tap is sensed
    doubletap_detect = true;
    if (gravitycolor_active == false) //if inactive before, go to color changing mode
    {
      led_state = STATE_CHANGECOLOR;
    }
    gravitycolor_active = true;
  }

  // Tap Detection
  if (adxl.triggered(interrupts, ADXL345_SINGLE_TAP)) {
#ifdef SERIALDEBUG
    Serial.println("*** TAP ***");
#endif
    //add code here to do when a tap is sensed
    tap_detect = true;
    if (gravitycolor_active == false) //if inactive before, go to color changing mode
    {
      led_state = STATE_CHANGECOLOR;
      tap_detect = false;
    }
    gravitycolor_active = true;

  }
}
