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
  if(lowpowerwait)
  {
    lowpowerwait = false;
    sleep_disable(); //disable sleep to make the watchdog reset the system if it expires
    wdt_enable(WDTO_250MS); //enable watchdog during normal operation, will reset the system if it expires (as we are not in sleep mode)
    WDTCSR |= (1<<WDIE); //enable watchdog interrupt
    WDTCSR &= ~(1<< WDE); //disable automatic system reset (bootloader does not support it and will get stuck in infinite reboot-loop)
  }
  else //watchdog triggered during normal operation, something bad has happened (stuck in infinite loop, probably due to I2C error)
  {
    //disable the I2C module (just in case)
    TWCR &= ~(1<< TWEN);
    //start the sketch from the beginning
    asm volatile ("  jmp 0");  
  }
}

void powerDown(uint8_t period)
{
  lowpowerwait = true;
  ADCSRA &= ~(1 << ADEN); //power down the ADC
  wdt_enable(period);
  WDTCSR |= (1 << WDIE);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); \
  cli();        \
  sleep_enable();   \
  //sleep_bod_disable(); //function from core library, is not always available... code copied here
  unsigned char tempreg;                           \
  __asm__ __volatile__("in %[tempreg], %[mcucr]" "\n\t"       \
                       "ori %[tempreg], %[bods_bodse]" "\n\t"     \
                       "out %[mcucr], %[tempreg]" "\n\t"      \
                       "andi %[tempreg], %[not_bodse]" "\n\t"     \
                       "out %[mcucr], %[tempreg]"           \
                       : [tempreg] "=&d" (tempreg)          \
                       : [mcucr] "I" _SFR_IO_ADDR(MCUCR),       \
                       [bods_bodse] "i" (_BV(BODS) | _BV(BODSE)), \
                       [not_bodse] "i" (~_BV(BODSE)));      \

  sei();        \
  sleep_cpu();      \
 //sleep_disable();    //sleep disable is called in the WDT interrupt
  sei();
  ADCSRA |= (1 << ADEN); //power up the ADC
  
}

