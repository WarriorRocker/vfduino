# [Arduino Samsung 20T202DA2JA 2x20 VFD with LCD Smartie](http://www.xodustech.com/development/vfduino)
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

## Code
The following highlights various parts of the code neccessary to drive the VFD and process the IR Remote

### Includes and variable declarations
```cpp
#include //include SPI.h library for the vfd
#include //include IRremote.h library
```
First the appropriate libraries must be loaded. Arduino comes with the SPI.h library, the IRremote.h library (linked in the requirements section) must be added to the Arduino libraries directory to compile in IR Remote support.

```cpp
const int slaveSelectPin = 10; //define additional spi pin for vfd
const int irPin = 8; //define the data pin of the ir reciever
```
Next we define the hardware pins that will be used with this sketch. Arduino Uno uses pins 11, 12, and 13 for SPI communication, pin 10 can be mapped elsewhere and is required to indicate we wish to speak with the VFD.

```cpp
IRrecv irrecv(irPin); //setup the IRremote library in recieve mode
decode_results results; //setup buffer variable for ir commands
unsigned long lastPress = 0; //variable for tracking the last keypress
```
Next we initialize the IR Remote library and setup a variable that will contain the last/current IR code recieved. Lastly the lastPress variable is setup to contain the time of the last IR code recieved, this is used to create a delay for sending the next keypress code over the serial connection.

```cpp
bool firstCommand = true; //set variable to clear display with first command
int flowControl = 0; //setup variable to contain current cursor position
```
The firstCommand variable tracks if there has been a command sent over serial to the sketch. When the first command is received the sketch clears the screen and resets the cursor position. The flowControl variable holds the current position of the cursor, this is used to set the cursor to the 2nd line of the screen when the 21st character in an unbroken (non cursor shifted) stream.

```cpp
//built in screens (each screen uses 42 bytes of flash)
prog_uchar screens[][41] PROGMEM = {
  "--  vfduino v1.0  ----By Travis Brown --",
  "--  Arduino  VFD  ---- waiting for PC --",
};
```
Using a char array stored in flash memory using the PROGMEM directive means undreds of screens can be defined without consuming any additional RAM. A simple function takes this data from flash and copies it into RAM one byte at a time and displays it on the screen.

### Setup and Loop
```cpp
void setup() {
  Serial.begin(9600); //open a serial connection
  irrecv.enableIRIn(); // start the ir receiver
  vfdInitialize(); //initialize and setup the samsung 20T202DA2JA vfd
  vfdDisplayScreen(1); //display the first screen (startup)
}
```
First serial is enabled at 9600 baud, next the IR receiver is enabled, and we call the vfdInitialize() function to setup the VFD screen. Lastly we call the vfdDisplayScreen() function to display the first screen on the VFD.

```cpp
void loop() {
  vfdProcess(); //process any pending requests to update the samsung vfd
  remoteProcess(); //process any pending ir remote commands
  //place additional non-blocking routines here
}
```
For the unititiated the loop() function gets called over and over again once the setup() function finishes as long as the microcontroller has power or is otherwise uninterrupted. Using non-blocking functions in this loop allows us to create a sort of multi-tasking on a device that is single threaded. Two functions are called here vfdProcess() which processes any pending commands for the screen, and remoteProcess() which processes any pending IR codes. Other non-blocking functions can be placed here making it possible to integrate other sketches with the VFD and IR functions.

### VFD Functions
```cpp
void vfdInitialize() {
  pinMode(slaveSelectPin, OUTPUT); //setup spi slave select pin as output
  SPI.begin(); //start spi on digital pins 
  SPI.setDataMode(SPI_MODE3); //set spi data mode 3
  vfdCommand(0x01); //clear all display and set DD-RAM address 0 in address counter
  vfdCommand(0x02); //move cursor to the original position
  vfdCommand(0x06); //set the cursor direction increment and cursor shift enabled
  vfdCommand(0x0c); //set display on,cursor on,blinking off
  vfdCommand(0x38); //set 8bit operation,2 line display and 100% brightness level
  vfdCommand(0x80); //set cursor to the first position of 1st line 
}
```
vfdInitialize() is used to setup the SPI connection to the VFD screen and then send the proper initialization commands to the VFD, these are neccessary as the VFD won't display anything until sent.

```cpp
void vfdProcess() {
  if (Serial.available()) {
    if (firstCommand) {
      vfdCommand(0x01); //clear all display and set DD-RAM address 0 in address counter
      vfdCommand(0x02); //move cursor to the original position
      firstCommand = false;
    }
    byte rxbyte = serialGet();
    if (rxbyte == 254) { //code 0xfd signals matrix orbital command
      vfdProcessCommand(); //call function to process the pending command
    } else {
      if (flowControl == 20) {
        vfdCommand(0xc0); //set cursor to the first position of 2nd line 
      } else if (flowControl == 40) {
        vfdCommand(0x80); //set cursor to the first position of 1st line 
        flowControl = 0; //reset flow control back to start position
      }
      flowControl++;
      vfdData(vfdProcessData(rxbyte)); //process the character and then send as data
    }
  }
}
```
vfdProcess() is called continously by loop() and determines if there is pending data for the screen and calls the command and data functions for the VFD.

```cpp
void vfdProcessCommand() {
  byte temp, temp2;
  switch (serialGet()) {
    //implemented commands for controlling screen
    case 38: //poll keypad
      remoteProcess();
      break;
    case 55: //read module type
      Serial.print(0x56); //report matrix orbital VK202-25-USB
      break;
    case 66: //backlight on (minutes)
      temp = serialGet();
      vfdCommand(0x38);
      break;
    ...
    //not implemented, no extra parameters
    case 35: //read serial number
    case 36: //read version number
  }
}
```
vfdProcessCommand() reads incomming data from serial and maps to the appropriate command/function for the VFD, the command set above emulates a matrix orbital serial display and reports as a 2x20 matrix orbital VFD. Not every command was neccessary to make this compatible with the LCD Smartie matrix.dll driver though care is taken to properly escape the extra parameters these commands may use, otherwise this may generate junk data on the screen.

```cpp
byte vfdProcessData(byte rxbyte) {
  //case: (input) -> return (desired output)
  switch (rxbyte) {
    case 1: //swap cgram(1) with char 0x14
      return 0x14;
    case 2: //swap cgram(2) with char 0x10
      return 0x10;
    ...
    case 186: //swap right bracket with closed circle
      return 0x94;
  }
}
```
vfdProcessData() takes as a parameter one byte of data, the VFD's character map differs from standard ascii and some characters need to be mapped to their proper address in the VFD's character map.

```cpp
void vfdCommand(unsigned char temp_2) {
  digitalWrite(slaveSelectPin, LOW);
  SPI.transfer(0xf8);
  SPI.transfer(temp_2);
  digitalWrite(slaveSelectPin, HIGH);
}
```
vfdCommand() accepts as a parameter one byte of data to send to the VFD preceeded by the byte 0xf8 signaling a command to the VFD screen.

```cpp
void vfdData(unsigned char temp_1) {
  digitalWrite(slaveSelectPin, LOW);
  SPI.transfer(0xfa);
  SPI.transfer(temp_1);
  digitalWrite(slaveSelectPin, HIGH);
}
```
vfdData() accepts as a parameter one byte of data to send to the VFD preceeded by the byte 0xfa signaling that this is data, normally this is a character to display on the screen however when used with other screen commands can contain custom character data for example.

```cpp
void vfdDisplayScreen(int screen) {
  for (int i = 0; i < 20; i++) {
    vfdCommand(0x80 + i);
    vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][i]))));
    vfdCommand(0xc0 + (19 - i));
    vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][20 + (19 - i)]))));
  }
}
```
vfdDisplayScreen() accepts as a parameter the number of the screen you wish to display, index starts at 1. This function uses the built in Arduino function pgm_read_byte() to read one byte of flash from a particular memory address. In this case screens[] is a pointer to the flash memory address containing its data.

```cpp
byte serialGet() {
  int incoming;
  while (!Serial.available()) { }
  incoming = Serial.read();
  return (byte) (incoming &0xff);
}
```
serialGet() pauses the current program execution until there is data available from serial, the first available byte is returned.

### IR Remote functions
```cpp
void remoteProcess() {
  if (irrecv.decode(&results)) { //check if the decoded results contain an ir code
    byte keyPress = remoteProcessKeycode(); //return the proper key pressed based on ir code
    if ((keyPress) && (millis() > (lastPress + 200))) { //check if keypress is outside threshold
      Serial.print((char)keyPress); //print the command received to serial
      lastPress = millis(); //update the last key press time
    }
    irrecv.resume(); //clear the results buffer and start listening for a new ir code
  }
}
```

```cpp
byte remoteProcessKeycode() {
  //keycodes setup for apple mini remote
  switch (results.value) {
    case 2011275437: //play/pause button
      return 65; //ascii character A
    case 2011283629: //menu button
      return 66; //ascii character B
    case 2011254957: //volume up button
      return 67; //ascii character C
    case 2011246765: //volume down button
      return 68; //ascii character D
    case 2011271341: //track left button
      return 69; //ascii character E
    case 2011259053: //track right button
      return 70; //ascii character F
    default:
      return 0; //no valid code received
  }
}
```

