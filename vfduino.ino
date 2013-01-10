/*---------------------------------------------------------------*\
|  vfduino - Samsung 20T202DA2JA VFD and remote using LCD Smartie |
|  Version 1.0 Developed and Designed by: Travis Brown            |
\*---------------------------------------------------------------*/

#include <SPI.h> //include SPI.h library for the vfd
#include <IRremote.h> //include IRremote.h library

const int slaveSelectPin = 10; //define additional spi pin for vfd
const int irPin = 2; //define the data pin of the ir reciever

IRrecv irrecv(irPin); //setup the IRremote library in recieve mode
decode_results results; //setup buffer variable for ir commands
unsigned long lastPress = 0; //variable for tracking the last keypress

bool firstCommand = true; //set variable to clear display with first command
int flowControl = 0; //setup variable to contain current cursor position

//built in screens (each screen uses 42 bytes of flash)
prog_uchar screens[][41] PROGMEM = {
  "--  Arduino  VFD  ----By Travis Brown --",
  "  Many screens are   possible using 240 ",
  "   unique and 16    custom characters.  ",
  " Compatible with LCDSmartie using Matrix",
  " Orbital plugin and keypad input with IR",
  "--  Arduino  VFD  ---- waiting for PC --",
  "Sym: `~!@#$%^&*()_-+[]{}\\|:;'\"<>?/,.  ",
  "Num: 0123456789          9876543210 :muN",
  "Alpha: ABCDEFGHIJKLMNOPQRSTUVWXYZ       ",
  "alpha: abcdefghijklmnopqrstuvwxyz       ",
  " Prepare for Jokes     They're Corny!   ",
  " I bet you I could     stop gambling.   ",
  " When is a door not    a door?          ",
  "-> When it's ajar.               <('.')>",
  " What kind of rooms    have no walls?   ",
  "-> Mushrooms.                    <('.'<)",
  " What's brown and    rhymes with Snoop? ",
  "-> Dr. Dre.                      (>'.')>",
  " Why was the pencil   in the toilet?    ",
  "-> It was a #2.                  (>'.'<)",
};

//screen animation sprites
prog_uchar twirl[4] PROGMEM = { 0xb0, 0x8c, 0x7c, 0x2f };

void setup() {
  Serial.begin(9600); //open a serial connection
  irrecv.enableIRIn(); //start the ir receiver
  vfdInitialize(); //initialize and setup the samsung 20T202DA2JA vfd
  vfdDisplayScreen(1); //display the first screen (startup)
  serialAvailableDelay(100); //introduce a 100ms delay to allow serial buffer to fill
  if (!Serial.available()) { //check if serial is ready if not run intro
    runIntro();
  }
}

void loop() {
  vfdProcess(); //process any pending requests to update the samsung vfd
  remoteProcess(); //process any pending ir remote commands
  //place additional non-blocking routines here
}

void runIntro() {
  int numScreens = (sizeof(screens) / sizeof(char) / 41);
  bool breaker = false;
  while (!breaker) {
    for (int i = 2; i < (numScreens + 1); i++) {
      if (breaker = ((serialAvailableDelay(2000)) || (vfdAnimateScreen(i, random(1, 5))))) {
        break;
      }
    }
  }
}

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
    case 86: //GPO off (number)
      temp = serialGet(); //replace with switch for your specific GPO pins
      break;
    case 87: //GPO on (number)
      temp = serialGet(); //replace with switch for your specific GPO pins
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
  return rxbyte; //return the unmodified character byte
}

void vfdData(unsigned char rxbyte) {
  digitalWrite(slaveSelectPin, LOW); //set ss pin low indicating a data stream
  SPI.transfer(0xfa); //signal the next byte of data to contain a data or character to display
  SPI.transfer(rxbyte); //send data or character byte over spi connection
  digitalWrite(slaveSelectPin, HIGH); //set ss pin high indicating end of data stream
}

void vfdCommand(unsigned char rxbyte) {
  digitalWrite(slaveSelectPin, LOW); //set ss pin low indicating a data stream
  SPI.transfer(0xf8); //signal the next byte of data to contain a command for the screen
  SPI.transfer(rxbyte); //send command byte over spi connection
  digitalWrite(slaveSelectPin, HIGH); //set ss pin high indicating end of data stream
}

void vfdDisplayScreen(int screen) {
  for (int i = 0; i < 20; i++) { //loop through the 20 columns of the display
    vfdCommand(0x80 + i); //set cursor to the nth position of the first line
    vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][i])))); //read byte out of flash, process, and send as data
    vfdCommand(0xc0 + (19 - i)); //set cursor to the nth position of the second line
    vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][20 + (19 - i)])))); //read byte out of flash, process, and send as data
  }
}

bool vfdAnimateScreen(int screen, int ani) {
  switch (ani) {
    default: case 1: //left-right twirl swipe
      for (int c = 0; c < 20; c++) {
        for (int l = 0; l < 3; l++) {
          for (int t = 0; t < 4; t++) {
            vfdCommand(0x80 + c);
            vfdData(pgm_read_byte(&(twirl[t])));
            vfdCommand(0xc0 + (19 - c));
            vfdData(pgm_read_byte(&(twirl[t])));
            delay(3);
          }
        }
        vfdCommand(0x80 + c);
        vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][c]))));
        vfdCommand(0xc0 + (19 - c));
        vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][20 + (19 - c)]))));
      }
      break;
    case 2: //inward-outward twirl swipe  
      for (int c = 0; c < 10; c++) {
        for (int l = 0; l < 3; l++) {
          for (int t = 0; t < 4; t++) {
            vfdCommand(0x80 + c);
            vfdData(pgm_read_byte(&(twirl[t])));
            vfdCommand(0x80 + (19 - c));
            vfdData(pgm_read_byte(&(twirl[t])));
            vfdCommand(0xc0 + c);
            vfdData(pgm_read_byte(&(twirl[t])));
            vfdCommand(0xc0 + (19 - c));
            vfdData(pgm_read_byte(&(twirl[t])));
            if (serialAvailableDelay(3)) { return true; }
          }
        }
        vfdCommand(0x80 + c);
        vfdData(0x20);
        vfdCommand(0x80 + (19 - c));
        vfdData(0x20);
        vfdCommand(0xc0 + c);
        vfdData(0x20);
        vfdCommand(0xc0 + (19 - c));
        vfdData(0x20);
      }
      for (int c = 10; c < 20; c++) {
        for (int l = 0; l < 3; l++) {
          for (int t = 0; t < 4; t++) {
            vfdCommand(0x80 + c);
            vfdData(pgm_read_byte(&(twirl[t])));
            vfdCommand(0x80 + (19 - c));
            vfdData(pgm_read_byte(&(twirl[t])));
            vfdCommand(0xc0 + c);
            vfdData(pgm_read_byte(&(twirl[t])));
            vfdCommand(0xc0 + (19 - c));
            vfdData(pgm_read_byte(&(twirl[t])));
            if (serialAvailableDelay(3)) { return true; }
          }
        }
        vfdCommand(0x80 + c);
        vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][c]))));
        vfdCommand(0x80 + (19 - c));
        vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][(19 - c)]))));
        vfdCommand(0xc0 + c);
        vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][20 + c]))));
        vfdCommand(0xc0 + (19 - c));
        vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][20 + (19 - c)]))));
      }
      break;
    case 3: //random twirl character place

      char order[40], r, t;
      for (int i = 0; i < 40; i++) {
        order[i] = i;
      }
      for (int i = 0; i < 40; i++) {
        r = random(0, 40);
        t = order[i];
        order[i] = order[r];
        order[r] = t;
      }
      for (int i = 0; i < 40; i++) {
        for (int l = 0; l < 3; l++) {
          for (int t = 0; t < 4; t++) {
            vfdCommand(0x80 + order[i] + ((order[i] > 19) ? 0x2c : 0));
            vfdData(pgm_read_byte(&(twirl[t])));
            if (serialAvailableDelay(5)) { return true; }
          }
        }
        vfdCommand(0x80 + order[i] + ((order[i] > 19) ? 0x2c : 0));
        vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][order[i]]))));
      }
      break;
    case 4: //inward-outward bar swipe
      for (int c = 0; c < 10; c++) {
        for (int t = 0; t < 5; t++) {
          vfdCommand(0x80 + c);
          vfdData(0x10 + t);
          vfdCommand(0x80 + (19 - c));
          vfdData(0x18 - t);
          vfdCommand(0xc0 + c);
          vfdData(0x10 + t);
          vfdCommand(0xc0 + (19 - c));
          vfdData(0x18 - t);
          if (serialAvailableDelay(10)) { return true; }
        }
      }
      for (int c = 10; c < 20; c++) {
        for (int t = 0; t < 4; t++) {
          vfdCommand(0x80 + c);
          vfdData(0x15 + t);
          vfdCommand(0x80 + (19 - c));
          vfdData(0x13 - t);
          vfdCommand(0xc0 + c);
          vfdData(0x15 + t);
          vfdCommand(0xc0 + (19 - c));
          vfdData(0x13 - t);
          if (serialAvailableDelay(10)) { return true; }
        }
        vfdCommand(0x80 + c);
        vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][c]))));
        vfdCommand(0x80 + (19 - c));
        vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][(19 - c)]))));
        vfdCommand(0xc0 + c);
        vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][20 + c]))));
        vfdCommand(0xc0 + (19 - c));
        vfdData(vfdProcessData(pgm_read_byte(&(screens[(screen - 1)][20 + (19 - c)]))));
      }
      break;
  }
  return false;
}

byte serialGet() {
  int incoming;
  while (!Serial.available()) { }
  incoming = Serial.read();
  return (byte) (incoming &0xff);
}

bool serialAvailableDelay(int ms) {
  for (int i = 0; i < ms; i++) {
    if (Serial.available()) { return true; } //exit function if serial available
    remoteProcess(); //process any pending ir remote commands
    delay(1);
  }
  return false; //return false to indicate serial not available
}
