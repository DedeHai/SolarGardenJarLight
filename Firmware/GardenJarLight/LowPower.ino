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

   Low power functions (library does not fully support ATMEGA168P so I am spinning my own code)
   Run this function to enter power down mode for some time using the watchdog to wake up

   period can be any of these defines:
  #define  WDTO_15MS   0
  #define   WDTO_30MS   1
  #define   WDTO_60MS   2
  #define   WDTO_120MS   3
  #define   WDTO_250MS   4
  #define   WDTO_500MS   5
  #define   WDTO_1S   6
  #define   WDTO_2S   7
  #define   WDTO_4S   8
  #define   WDTO_8S   9



*/


//Arduino does not support the ATMEGA168P so we need to define the BOD disable bits here (they are identical on the 328P so this is no issue)
#define MCUCR   _SFR_IO8(0x35)
#define IVCE    0
#define IVSEL   1
#define PUD     4
#define BODSE   5
#define BODS    6


bool lowpowerwait = false; //set to true during low power wait, used for watchdog


ISR (WDT_vect)
{
  //disable the watchdog timer upon wakeup
  wdt_disable();

  if (lowpowerwait)
  {
    lowpowerwait = false;
    sleep_disable(); //disable sleep mode
    wdt_enable(WDTO_4S); //enable watchdog during normal operation, will restart the sketch if it expires (we are not in sleep mode now)
    WDTCSR |= (1 << WDIE); //enable watchdog interrupt   (WDE flag cannot be disabled! it is auto enabled once the timer runs out!)
  }
  else //watchdog triggered during normal operation, something bad has happened (stuck in infinite loop, probably due to I2C error)
  {
    ADCSRA &= ~(1 << ADEN); //power down the ADC (just in case)
    wdt_enable(WDTO_4S); //initialize the watchdog
    WDTCSR |= (1 << WDIE); //enable watchdog interrupt
    WDTCSR |= (1 << WDCE); //watchdog change enable, must be set to be able to clear WDE bit
    WDTCSR &= ~(1 << WDE);//disable the watchdog resetting the system, instead run this interrupt which will reboot
    wdt_disable(); //disable watchdog 
  
    //disable the I2C module and clear interrupt and the interrupt flag (just in case)
    TWCR &= ~((1 << TWEN) | (1 << TWIE));
    TWCR |= (1 << TWINT);
    adxl_powerdown();
    //start the sketch from the beginning
    asm volatile ("jmp 0");
  }
}

void powerDown(uint8_t period)
{
  lowpowerwait = true;
  ADCSRA &= ~(1 << ADEN); //power down the ADC
  wdt_enable(period);
  WDTCSR |= (1 << WDIE);
  WDTCSR |= (1 << WDCE); //watchdog change enable, must be set to be able to clear WDE bit
  WDTCSR &= ~(1 << WDE); //disable automatic system reset (the bootloader does not support it and will get stuck in infinite reboot-loop, restart the sketch instead)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();
  sleep_enable();
  //sleep_bod_disable(); //function from core library, is not always available... code copied here:
  unsigned char tempreg;
  __asm__ __volatile__("in %[tempreg], %[mcucr]" "\n\t"
                       "ori %[tempreg], %[bods_bodse]" "\n\t"
                       "out %[mcucr], %[tempreg]" "\n\t"
                       "andi %[tempreg], %[not_bodse]" "\n\t"
                       "out %[mcucr], %[tempreg]"
                       : [tempreg] "=&d" (tempreg)
                       : [mcucr] "I" _SFR_IO_ADDR(MCUCR),
                       [bods_bodse] "i" (_BV(BODS) | _BV(BODSE)),
                       [not_bodse] "i" (~_BV(BODSE)));

  sei();
  sleep_cpu();      //go to sleep now
  sleep_disable();    //disable sleep mode after waking up
  sei();
  ADCSRA |= (1 << ADEN); //power up the ADC (used to read voltages)
}

/*note: if arduino gets stuck in bootloader (some problem with I2C communication that happens on some hardware, did not find the reason why)
   need to update the twi stop function in the twi.c file in arduino core with a timeout, use this:

   void twi_stop(void)
  {
  uint16_t timeout;
  // send stop condition
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO);

  // wait for stop condition to be exectued on bus
  // TWINT is not set after a stop condition!
  timeout =0xFFFF;
  while(TWCR & _BV(TWSTO)){
   timeout--;
    if(timeout==0) break;
    continue;
  }

  // update twi state
  twi_state = TWI_READY;
  }
*/


