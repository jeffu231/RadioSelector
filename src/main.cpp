/**
    main.cpp
    Purpose: Manage the Mic / Keyer Input

    @author Jeff Uchitjil
    @version 1.0 05/29/20 
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono9pt7b.h>

#include "main.h"

#define DEBUG 0
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Data pin configurations on MAX4820
#define MAX_SCLK 2      // SCLK, Connector pin 6 MAX4820  
#define MAX_CS 3        // CS, Connector pin 4 MAX4820
#define MAX_SDA 4       // DIN, Connector pin 5 MAX4820
#define MAX_RESET 5     // RESET, Connector pin 3 MAX4820

//Input button pins
#define micButtonPin 6
#define keyerButtonPin 7

//Define the radios here.
//  name, hasMic, hasKeyer 
Radio radios[4] = { {"IC-756",true,true}, {"TS-850",true,true}, {"TS-790",true,true}, {"K2",true,true} }; 

//Define the callsign for the header here
const char callsign[] = "KB9KLD";

const char keyerLabel[] = "Key ";
const char micLabel[] = "Mic ";

const int displayCells = 20;
const int maxNameLength = displayCells / 2 - 4;

int stateStoredAddress = 0;
int stateStoreAddress = sizeof(int);

InputState inputState;

int lastMicButtonState = LOW;
int lastKeyerButtonState = LOW;
bool micButtonEnabled = false;
bool keyerButtonEnabled = false;
byte numRadios;

const byte micStateRef = 0b10000000;
const byte keyerStateRef = 0b00001000;
byte micState = micStateRef;
byte keyerState = keyerStateRef;

const int outputDelay = 50;     // Delay in data output, here 50µs

/**
    Arduino Setup block
*/
void setup() {

#if DEBUG==1
    Serial.begin(9600);
#endif
  
  //determine how many radios are defined
  numRadios = (sizeof(radios)/sizeof(*radios));
  
  initializeDisplay();

  if (numRadios > 4) {
    display.println("Too Many");
    display.println("Radios");
    display.display();
    for (;;); // Don't proceed, loop forever
  }

  pinMode(MAX_RESET, OUTPUT);
  pinMode(MAX_CS, OUTPUT);
  pinMode(MAX_SDA, OUTPUT);
  pinMode(MAX_SCLK, OUTPUT);
  
  pinMode(micButtonPin, INPUT);
  pinMode(keyerButtonPin, INPUT);

  digitalWrite(MAX_RESET, LOW);   // reset all outputs
  delayMicroseconds (outputDelay);
  digitalWrite(MAX_CS, HIGH);     // do not select a block
  delayMicroseconds (outputDelay);
  digitalWrite(MAX_SDA, LOW);     // SDA/DIN set to low
  delayMicroseconds (outputDelay);;
  digitalWrite(MAX_SCLK, LOW);    // SCLK set to low
  delayMicroseconds (outputDelay);
  digitalWrite(MAX_RESET, HIGH);  // Enable blocks for programming
  delayMicroseconds (outputDelay);


  int micButtonState = digitalRead(micButtonPin);
  if (micButtonState == LOW) {
    restoreInputState();
  } else {
  #if DEBUG==1
    Serial.println("Clearing state!");
  #endif
    clearStoredInputState();
    displayResetMessage();
    while (digitalRead(micButtonPin) == HIGH) {
      delay(10);
    }
  }

  setActiveMicRadio();
  setActiveKeyerRadio();
  setButtonsEnable();
  updateDisplay();
}

/**
    Main loop
*/
void loop() {
 
  int micButtonState = digitalRead(micButtonPin);
  int keyerButtonState = digitalRead(keyerButtonPin);
  if (micButtonEnabled && micButtonState == HIGH) {
    moveToNextMicRadio();
    updateDisplay();
    delay(1000); //give the user a chance to release the button
  } else if (micButtonEnabled && lastMicButtonState == HIGH) {
    setActiveMicRadio();
  }

  if (keyerButtonEnabled && keyerButtonState == HIGH) {
    moveToNextKeyerRadio();
    updateDisplay();
    delay(1000); //give the user a chance to release the button
  } else if (keyerButtonEnabled && lastKeyerButtonState == HIGH) {
    setActiveKeyerRadio();
  }

  lastMicButtonState = micButtonState;
  lastKeyerButtonState = keyerButtonState;
}

/**
    Writes the state to the EEProm so we can restart where we left off.
*/
void storeInputState() {
  EEPROM.put(stateStoreAddress, inputState);
  EEPROM.put(stateStoredAddress, 1);
}

/**
    Clear any stored state
*/
void clearStoredInputState() {
  inputState = {};
  EEPROM.put(stateStoreAddress, inputState);
  EEPROM.put(stateStoredAddress, 0);
}

/**
    Restore the saved state from the EEProm
*/
void restoreInputState() {
  
  int stateStored;

  EEPROM.get(stateStoredAddress, stateStored);
  if (stateStored == 1) {
    EEPROM.get(stateStoreAddress, inputState);
  } 

}

/**
    Enables or disables the Mic or Keyer selection button.
    If there are no radios that support a Mic or Keyer, disable the selection button
    and turn off the ports so the input is not attached to anything
*/
void setButtonsEnable(){
  for(int i=0;i<numRadios;i++){
    if(radios[i].hasMic){
      micButtonEnabled=true;
    }
    if(radios[i].hasKeyer){
      keyerButtonEnabled=true;
    }
  }

  if(!micButtonEnabled)
  {
    setMicDisabled();
  }
  if(!keyerButtonEnabled)
  {
    setKeyerDisabled();
  }
}

/**
    Advance the Mic to the next radio in the list
    Saves the updated state to the EEProm
*/
void moveToNextMicRadio() {
  if (inputState.activeMicRadio >= 3) {
    inputState.activeMicRadio = -1;
  }
  inputState.activeMicRadio++;
  while(radios[inputState.activeMicRadio].hasMic == false){
    moveToNextMicRadio();
  }
  
  storeInputState();
}

/**
    Advance the Keyer to the next radio in the list
    Saves the updated state to the EEProm
*/
void moveToNextKeyerRadio() {
  if (inputState.activeKeyerRadio >= 3) {
    inputState.activeKeyerRadio = -1;
  } 
  inputState.activeKeyerRadio++;
  while(radios[inputState.activeKeyerRadio].hasKeyer == false){
    moveToNextKeyerRadio();
  }
  storeInputState();
}

/**
    Gets the active Radio object for the Keyer
    @return the active radio object for the keyer
*/
Radio *getActiveKeyerRadio() {
   if(inputState.activeKeyerRadio < numRadios){
      return &radios[inputState.activeKeyerRadio];
   }else{
      return &radios[0];
   }
}

/**
    Gets the active Radio object for the Mic
    @return the active radio object for the mic
*/
Radio *getActiveMicRadio() {
   if(inputState.activeMicRadio < numRadios){
      return &radios[inputState.activeMicRadio];
   }else{
      return &radios[0];
   }
   
}

/**
    Connects the Mic to the active Radio by toggling the IO state for it.
*/
void setActiveMicRadio() {
  setMicOn(inputState.activeMicRadio);
}

/**
    Connects the Keyer to the active Radio by toggling the IO state for it.
*/
void setActiveKeyerRadio() {
  setKeyerOn(inputState.activeKeyerRadio);
}

/*
    Initializes the display
*/
void initializeDisplay(){

  // Initiate the LCD:
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

#if DEBUG==1
  Serial.println(F("SSD1306 allocation failed"));
#endif
  
    for (;;); // Don't proceed, loop forever
  }
  
    // Clear the display buffer and set up our fonts
  display.clearDisplay();
  display.setFont(&FreeMono9pt7b);
  display.setTextSize(1); //font 1-8
  display.setTextColor(WHITE);

  //Measure up to configure where the first lines starts with custom font
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(callsign, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(0, h);
  display.println("Booting..");
  display.display();
}

/**
    Update the display to reflect the current state
*/
void updateDisplay() {
  display.clearDisplay();
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(callsign, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2 , h);
  display.setTextColor(WHITE);
  display.println(callsign);
  display.setCursor(0, h * 2 + 15);

  //Update the Mic Radio name
  char radioLine[11];  //11 chars long with this font, but should calculate it.
  strcpy(radioLine, micLabel);
  Radio *radio = getActiveMicRadio();
  strcat(radioLine, micButtonEnabled?radio->nm:"None");  
  display.println(radioLine);

  //now update the Keyer Radio
  radioLine[0] = 0;
  strcpy(radioLine, keyerLabel);
  radio = getActiveKeyerRadio();
  strcat(radioLine, keyerButtonEnabled?radio->nm:"None");  
  display.println(radioLine);
  display.display();
}

/**
    Update the display with the reset messages
*/
void displayResetMessage() {
  display.clearDisplay();
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("Reset", 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2 , h);
  display.setTextColor(WHITE);
  display.println("Reset");
  display.display();
}

//IO State management

/**
    Connects the Mic to Radio port at the given index
    @param index if the radio port to set active
*/
void setMicOn(int index) {
  micState = micStateRef >> index;
  writeOutput(micState | keyerState);
}

/**
    Enable or disable the entire Mic from any radio port.
*/
void setMicDisabled() {
  micState = 0;
  writeOutput(micState | keyerState);
}

/**
    Connects the Keyer to Radio port at the given index
*/
void setKeyerOn(int index) {
  keyerState = keyerStateRef >> index;
  writeOutput(micState | keyerState);
}

/**
    Enable or disable the entire Mic from any radio port.
*/
void setKeyerDisabled() {
  keyerState = 0;
  writeOutput(micState | keyerState);
}

void writeOutput(byte output) {

  digitalWrite(MAX_RESET, HIGH);  // Enable blocks for programming

  /*
  CS: Drive CS low to alert the device for programming. When CS is low, data at DIN is
  clocked into the 8-bit shift register on SCLK’s rising edge. Drive CS
  from low to high to latch the data to the registers and activate the
  appropriate relays.
  */
  digitalWrite(MAX_CS, LOW);
  delayMicroseconds (outputDelay);
  
  //Write the byte data out one bit at a time
  for (int i = 0; i < 8; i++) {
    
    uint8_t bit = bitRead(output,i);
    if (bit == 1) {
      digitalWrite(MAX_SDA, HIGH);
      
    } else {
      digitalWrite(MAX_SDA, LOW);
      
    }

    // latch assignment
    digitalWrite(MAX_SCLK, HIGH); //clock pulse is generated, data is accepted 
    delayMicroseconds(outputDelay);   


    digitalWrite(MAX_SCLK, LOW); //clock pulse is withdrawn 
    delayMicroseconds(outputDelay);   
         
  }
  digitalWrite(MAX_SDA, LOW);

  // /Deactivate CS = latch data to outputs
  digitalWrite(MAX_CS, HIGH);

}
