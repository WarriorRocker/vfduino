# Arduino Samsung 20T202DA2JA 2x20 VFD with LCD Smartie
Using the above sketch examples it's possible to control a Samsung 20T202DA2JA 2x20 VFD using LCD Smartie or any application which is compatible with Matrix Orbital serial LCD's. The screen can also be manipulated directly without a computer using a variety of built in animations and functions which store built in screens in flash memory.

![ScreenShot](http://www.xodustech.com/images/xodusampredux/redux-25.jpg)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
![ScreenShot](http://www.xodustech.com/images/xodusampredux/redux-26.jpg)

## Requirements
* [Arduino-IRremote](https://github.com/shirriff/Arduino-IRremote) - Infrared remote library for Arduino: send and receive infrared signals with multiple protocols
 * Note: Only neccessary if you are adding IR remote for keypad input

## Setup with LCD Smartie
LCD Smartie supports additional features of matrix orbital screens including GPO and Keypad support allowing up to 25 different keys.

### Configuring the screen plugin
* Determine the port your Arduino is connected to, this is listed in your Arduino IDE under Tools -> Serial Port
 * or in your Device Manager under COM & LPT Ports
* Launch LCD Smartie and open the Setup screen, select the Plugin tab on the right hand side
* Select "matrix.dll" from the Display Plugin dropdown and change COM1 to your appropriate port (unless COM1 is already correct)
* Select the Screen tab on the right hand side and select "2x20" from the LCD Size dropdown
* Click the OK button and then close LCD Smartie completely
* Launch LCD Smartie and you should now see stuff displayed on the Samsung VFD, if not check port settings and relaunch

### Configuring Keypad
* Launch LCD Smartie and open the Setup screen, select the Actions tab at the top
* By default LCD Smartie comes with several actions already mapped, these "should" be deleted first
* Select the last (or only ) line in the actions table to get a blank action string
* Select the LCD Features tab on the left hand side, and then click Buttons (once)
* Press a button on the keypad of your Arduino, you should see its letter shown in the Last key pressed box
* Double-click Buttons in the left hand side menu to add the last button pressed or enter button name manually
* Next select your condition, normally this in equals (=) sign and the test value which is normally 1
* Lastly select the output action on the right hand side and click Add, click OK at the bottom and your button should now work

## Finding the Screen
The Samsung 20T202DA2JA 2x20 VFD is commonly available from many locations:
* Adafruit - http://www.adafruit.com/products/347
* eBay - http://www.ebay.com/sch/i.html?_nkw=20T202DA2JA
