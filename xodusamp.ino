/*---------------------------------------------------------------*\
|  XodusAmp v2.0 Teensy 2.0 Sketch                                |
|  Version 1.0 Developed and Designed by: Travis Brown            |
\*---------------------------------------------------------------*/

#include <SPI.h> //include SPI.h library for the vfd
#include <IRremote.h> //include IRremote.h library

const int slaveSelectPin = 0; //define additional spi pin for vfd
const int motorAPin = 4; //define first control pin for volume motor
const int motorBPin = 5; //define second control pin for volume motor
const int channelPin = 6; //define used to select 2 or 4 channel input
const int powerPin = 7; //define pin used to turn amp on and off
const int irPin = 8; //define the data pin of the ir reciever
const int modePin = 21; //define pin to check for current power mode

IRrecv irrecv(irPin); //setup the IRremote library in recieve mode
decode_results results; //setup buffer variable for ir commands
unsigned long lastPress = 0; //variable for tracking the last keypress

bool firstCommand = true; //set variable to clear display with first command
int flowControl = 0; //setup variable to contain current cursor position

int powerThreshold = 900; //threshold for knowing when amp is powered

//built in screens (each screen uses 42 bytes of flash)
prog_uchar screens[][41] PROGMEM = {
  "-- XodusAmp Redux ----By Travis Brown --",
  "--  Arduino  VFD  ----By Travis Brown --",
};

//built in 2nd display line screens (each line uses 21 bytes of flash)
prog_uchar lines[][21] PROGMEM = {
  "     Amplifier On   ", "    Amplifier Off   ",
  "      Volume Up     ", "     Volume Down    ",
  "   2 Channel Input  ", "   4 Channel Input  ",
  "     Amp is Off     ", "   No power to Amp  ",
};

//buffer to hold current data displayed on 2nd display line
char lineBuffer[21] = { "                    " };

void setup() {
  pinMode(modePin, INPUT); //setup modePin as input detecting input voltage to microcontroller
  pinMode(motorAPin, OUTPUT); //setup motorAPin as output connected to motor pin A via h-bridge
  pinMode(motorBPin, OUTPUT); //setup motorBPin as output connected to motor pin B via h-bridge
  pinMode(powerPin, OUTPUT); //setup powerPin as output connected to amp ACC line via h-bridge
  pinMode(channelPin, OUTPUT); //setup channelPin as output connected to relay via NPN transistor
  
  digitalWrite(motorAPin, 0); //set motorAPin to 0 setting motor pin A to ground
  digitalWrite(motorAPin, 0); //set motorBPin to 0 setting motor pin B to ground
  digitalWrite(powerPin, 0); //set powerPin to 0 deactivating the amplifier
  digitalWrite(channelPin, 0); //set channelPin to 0 indicating 4 channel input
  
  Serial.begin(9600); //open a serial connection
  irrecv.enableIRIn(); // start the ir receiver
  vfdInitialize(); //initialize and setup the samsung 20T202DA2JA vfd
  vfdDisplayScreen(1); //display the first screen (startup)
  if (analogRead(modePin) < powerThreshold) { //check if the power input to amp is connected
    vfdDisplayLinePause(8); //display no power to amp 2nd line screen
  }
}

void loop() {
  vfdProcess(); //process any pending requests to update the samsung vfd
  remoteProcess(); //process any pending ir remote commands
  //place additional non-blocking routines here
}

void remoteProcess() {
  if (irrecv.decode(&results)) { //check if the decoded results contain an ir code
    byte keyPress = remoteProcessKeycode(); //return the proper key pressed based on ir code
    if ((keyPress) && (millis() > (lastPress + 200))) { //check if keypress is outside threshold
      remoteProcessCommand(keyPress); //process the command received for amp controls
      Serial.print((char)keyPress); //print the command received to serial
      lastPress = millis(); //update the last key press time
    }
    irrecv.resume(); //clear the results buffer and start listening for a new ir code
  }
}

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

void remoteProcessCommand(byte keyPress) {
  if (analogRead(modePin) >= powerThreshold) { //check if the power input to amp is connected
    if (digitalRead(powerPin)) { //check if the amp is currently powered on
      switch (keyPress) {
        case 65:
          ampTogglePower();
          break;
        case 66:
          ampToggleChannelInput();
          break;
        case 67:
          ampVolumeUp();
          break;
        case 68:
          ampVolumeDown();
          break;
      }
    } else { //amp is currently powered off
      if (keyPress == 65) { //check if power button pressed
        ampTogglePower();
      } else { //power button was not pressed
        vfdDisplayLinePause(7); //display amp is off 2nd line screen
      }
    }
  } else { //amp is not being powered externally
    vfdDisplayLinePause(8); //display no power to amp 2nd line screen
  }
}

void vfdInitialize() {
  pinMode(slaveSelectPin, OUTPUT); //setup spi slave select pin as output
  SPI.begin(); //start spi on digital pins 
  SPI.setDataMode(SPI_MODE3); //set spi data mode 3
  
  vfdCommand(0x01); //clear all display and set DD-RAM address 0 in address counter
  vfdCommand(0x02); //move cursor to the original position
  vfdCommand(0x06); //set the cursor direction increment and cursor shift enabled
  vfdCommand(0x0c); //set display on, cursor on, blinking off
  vfdCommand(0x38); //set 8bit operation, 2 line display, and 100% brightness level
  vfdCommand(0x80); //set cursor to the first position of 1st line 
}

void vfdProcess() {
  if (Serial.available()) { //check if there is data waiting in the serial buffer
    if (firstCommand) { //check if this is the first command received
      vfdCommand(0x01); //clear all display and set DD-RAM address 0 in address counter
      vfdCommand(0x02); //move cursor to the original position
      firstCommand = false; //set check variable to false
    }
    byte rxbyte = serialGet(); //get the first byte of data available in the buffer
    if (rxbyte == 254) { //check if byte equals 0xfd signaling matrix orbital command
      vfdProcessCommand(); //call function to process the pending command
    } else { //data is a byte to display as character on the screen
      if (flowControl == 20) { //check if character is the 21st in the stream
        vfdCommand(0xc0); //set cursor to the first position of 2nd line 
      } else if (flowControl == 40) { //check if character is the 41st in the stream
        vfdCommand(0x80); //set cursor to the first position of 1st line 
        flowControl = 0; //reset flow control counter
      }
      if (flowControl >= 20) { //check if character is on the 2nd line
        lineBuffer[flowControl - 20] = vfdProcessData(rxbyte); //save character to line buffer
      }
      flowControl++; //increment the flow control counter
      vfdData(vfdProcessData(rxbyte)); //process and write data to the screen
    }
  }
}

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
      vfdCommand(0x38); //set 8bit operation, 2 line display, and 100% brightness level
      break;
    case 70: //backlight off
      vfdCommand(0x3b); //set 8bit operation, 2 line display, and 25% brightness level
      break;
    case 71: //set cursor position (column, row)
      temp = (serialGet() - 1);
      flowControl = temp; //set flow control to the new column number
      if (serialGet() == 2) { //check if new position is on 2nd row
        temp += 0x40; //add 64 to n placing cursor on next line
        flowControl += 20; //increment flow control to the next line
      }
      vfdCommand(0x80 + temp); //set cursor to the nth position
      break;
    case 72: //set cursor home
      flowControl = 0; //reset the flow control counter
      vfdCommand(0x80); //set cursor to the first position of 1st line 
      break;
    case 76: //move cursor left
      flowControl--; //decrement the flow control counter
      vfdCommand(0x10); //set cursor position one character to the left
      break;
    case 77: //move cursor right
      flowControl++; //increment the flow control counter
      vfdCommand(0x11); //set cursor position one character to the right
      break;
    case 78: //define custom character (id, data 8 bytes)
      vfdCommand(0x40 + (serialGet() * 8));
      for (temp = 0; temp < 8; temp++) {
        vfdData(serialGet());
      }
      break;
    case 87: //GPO on (number)
      switch (serialGet()) {
        case 1:
          ampTogglePower();
          break;
        case 4:
          ampToggleChannelInput();
          break;
      }
      break;
    case 88: //clear display
      flowControl = 0;
      vfdCommand(0x01); //clear all display and set DD-RAM address 0 in address counter
      vfdCommand(0x80); //set cursor to the first position of 1st line 
      break;

    //not implemented, no extra parameters
    case 35: //read serial number
    case 36: //read version number
    case 59: //exit flow-control mode
    case 65: //auto transmit keypresses on  
    case 67: //auto line-wrap on
    case 68: //auto line-wrap off
    case 69: //clear key buffer   
    case 74: //show underline cursor
    case 75: //hide underline cursor
    case 79: //auto transmit keypresses off
    case 81: //auto scroll on
    case 82: //auto scroll off
    case 83: //show blinking block cursor
    case 84: //hide blinking block cursor
    case 86: //GPO off (number)
    case 96: //auto-repeat mode off
    case 104: //init horizontal bar graph
    case 109: //init medium size digits
    case 115: //init narrow verticle bar graph
    case 118: //init wide verticle bar graph
      break;

    //not implemented, 4 parameters
    case 124: //place horizontal bar graph (column, row, direction, length)
      temp = serialGet();

    //not implemented, 3 parameters
    case 111: //place medium numbers (row, column, digit)
      temp = serialGet();

    //not implemented, 2 parameters
    case 61: //place vertical bar (column, length)
    case 195: //set startup gpo state (number, state)
      temp = serialGet();
      
    //not implemented, 1 parameter
    case 51: //set i2c slave address (address)
    case 57: //change serial speed (speed)
    case 80: //set contrast (contrast)
    case 85: //set keypress debounce time (time)  
    case 126: //set auto repeat mode (mode)
    case 145: //set and save contrast (contrast)
    case 152: //set and save backlight brightness (brightness)
    case 153: //set backlight brightness (brightness)
    case 160: //set screen transmission protocol (protocol)
    case 164: //set custom serial baud rate (rate) 
    case 192: //load custom character bank (bank)
      temp = serialGet();
      break;
      
    //not implemented, custom parameter lengths
    case 64: //set startup message (80 bytes)
      for (temp = 0; temp < 80; temp++) {
      temp2 = serialGet();
      }
      break;
    case 193: //save custom character (bank, id, data 8 bytes)
      for (temp = 0; temp < 10; temp++) {
      temp2 = serialGet();
      }
      break;
    case 194: //save custom startup character (id, data 8 bytes)
      for (temp = 0; temp < 9; temp++) {
      temp2 = serialGet();
      }
      break;
    case 213: //assign keypad codes (down 25 bytes, up 25 bytes)
      for (temp = 0; temp < 50; temp++) {
      temp2 = serialGet();
      }
      break;
  }
}

byte vfdProcessData(byte rxbyte) {
  //case: (input) -> return (desired output)
  switch (rxbyte) {
    case 1: //swap cgram(1) with char 0x14
      return 0x14;
    case 2: //swap cgram(2) with char 0x10
      return 0x10;
    case 3: //swap cgram(3) with char 0x11
      return 0x11;
    case 92: //swap yen symbol with backslash
      return 0x8c;
    case 126: //swap right arrow with tilde
      return 0x8e;
    case 128: //swap 0x80 with dollar sign (no euro in charset)
      return 0x24;
    case 149: //swap open circle with bullet
      return 0xa5;
    case 165: //swap bullet with yen symbol
      return 0x5c;
    case 176: //swap dash with open circle
      return 0x95;
    case 186: //swap right bracket with closed circle
      return 0x94;
  }
  return rxbyte;
}

void vfdData(unsigned char temp_1) {
  digitalWrite(slaveSelectPin, LOW);
  SPI.transfer(0xfa);
  SPI.transfer(temp_1);
  digitalWrite(slaveSelectPin, HIGH);
}

void vfdCommand(unsigned char temp_2) {
  digitalWrite(slaveSelectPin, LOW);
  SPI.transfer(0xf8);
  SPI.transfer(temp_2);
  digitalWrite(slaveSelectPin, HIGH);
}

byte vfdGetData(unsigned char temp) {
  digitalWrite(slaveSelectPin, LOW);
  //SPI.transfer(
  temp = SPI.transfer(0xfb);
  digitalWrite(slaveSelectPin, HIGH);
  return temp;
}

void vfdDisplayScreen(int screen) {
  for (int i = 0; i < 20; i++) {
    vfdCommand(0x80 + i);
    vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][i]))));
    vfdCommand(0xc0 + i);
    lineBuffer[i] = vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][20 + i])));
    vfdData(lineBuffer[i]);
  }
}

void vfdDisplayLine(int line) {
 for (int i = 0; i < 20; i++) {
   vfdCommand(0xc0 + i);
   vfdData(pgm_read_byte(&(lines[(line - 1)][i])));
 } 
}

void vfdDisplayLinePause(int line) {
  vfdDisplayLine(line);
  delay(250);
  vfdCopyFromLineBuffer();
}

void vfdWriteDebug(byte rxbyte) {
  vfdCommand(0x01); //clear all display and set DD-RAM address 0 in address counter
  vfdCommand(0x80); //set cursor to the first position of 1st line 
  char z_str[5] = "    ";
  itoa(rxbyte, z_str, 10);
  for (int i=0; i<5; i++) {
    vfdData(z_str[i]);
  }
}

void vfdCopyFromLineBuffer() {
 for (int i = 0; i < 20; i++) {
   vfdCommand(0xc0 + i);
   vfdData(lineBuffer[i]);
 }
}

byte serialGet() {
  int incoming;
  while (!Serial.available()) { }
  incoming = Serial.read();
  return (byte) (incoming &0xff);
}

void ampVolumeUp() {
  vfdDisplayLine(3);
  digitalWrite(motorAPin, 1);
  remoteRepeatWait();
  digitalWrite(motorAPin, 0);
  vfdCopyFromLineBuffer();
}

void ampVolumeDown() {
  vfdDisplayLine(4);
  digitalWrite(motorBPin, 1);
  remoteRepeatWait();
  digitalWrite(motorBPin, 0);
  vfdCopyFromLineBuffer();
}

void ampToggleChannelInput() {
  if (digitalRead(channelPin)) {
    digitalWrite(channelPin, 0);
    vfdDisplayLinePause(6);
  } else {
    digitalWrite(channelPin, 1);
    vfdDisplayLinePause(5);
  }
}

void ampTogglePower() {
  if (digitalRead(powerPin)) {
    digitalWrite(powerPin, 0);
    vfdDisplayLinePause(2);
  } else {
    digitalWrite(powerPin, 1);
    vfdDisplayLinePause(1);
  }
}

void remoteRepeatWait() {
  do {
    irrecv.resume();
    delay(150);
  } while ((irrecv.decode(&results)) && (results.value == 4294967295));
}
