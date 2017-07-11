# Solar Garden Jar Light
A solar powered LED light controlled by an Arduino. It is designed to fit inside most glass jars lighting up your garden or back yard in beatuiful colors. 
The Arduino is connected to an accelerometer to switch the light on and off and to change the color by tilting it. It was designed with easy assembly in mind to be sold as a DIY kit at [Fablab Winti](www.fablabwinti.ch). The voltage regulator as well as the power LED are removed from the Arduino to make it low power consumption consuming only 120µA on average when switched off, making the 500mAh battery last up to half a year when kept in the dark. The solar cell voltage is used to detect low light conditions at dawn and the light does automatically switch on for a few hours if the battery is fully charged. The threshold levels can be adjusted easily in the software.

More information available at https://bashtelorofscience.wordpress.com/ 

License:
Software source code is GNU GPL V3 licensed
Hardware under Creative Commons 4.0 license: CC-BY-NC-SA

[![License: CC BY-NC-SA 4.0](https://licensebuttons.net/l/by-nc-sa/4.0/80x15.png)](https://creativecommons.org/licenses/by-nc-sa/4.0/)

# Software
The PCB was designed in **eagle** version 7.5.0. The firmware project was created using **Arduino IDE**.

# Fotos
![Finished Jar](/images/IMG_20170709_174736.jpg) 
![Outside](/images/IMG_20170709_174619.jpg) 
![Assemblded Lid](/images/IMG_20170709_174505.jpg) 
![Assemblded PCB](/images/IMG_20170709_174442.jpg) 




