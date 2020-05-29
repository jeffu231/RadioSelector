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

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Data pin configurations based on 74SL139
#define micEnablePin 2
#define micAPin 3
#define micBPin 4

#define keyEnablePin 5
#define keyAPin 6
#define keyBPin 7

//Input button pins
#define micButtonPin 8
#define keyerButtonPin 9

//Define the radios here.
// hasMic, hasKeyer, name 
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

int lastMicButtonState = HIGH;
int lastKeyerButtonState = HIGH;
bool micButtonEnabled = false;
bool keyerButtonEnabled = false;
byte numRadios;

/**
    Arduino Setup block
*/
void setup() {

  Serial.begin(9600);

  //determine how many radios are defined
  numRadios = (sizeof(radios)/sizeof(*radios));
  
  initializeDisplay();

  if (numRadios > 4) {
    Serial.println("Too many radios");
    display.println("Too Many");
    display.println("Radios");
    display.display();
    for (;;); // Don't proceed, loop forever
  }

  pinMode(micEnablePin, OUTPUT);
  pinMode(micAPin, OUTPUT);
  pinMode(micBPin, OUTPUT);

  pinMode(keyEnablePin, OUTPUT);
  pinMode(keyAPin, OUTPUT);
  pinMode(keyBPin, OUTPUT);

  pinMode(micButtonPin, INPUT);
  pinMode(keyerButtonPin, INPUT);

  int micButtonState = digitalRead(micButtonPin);
  if (micButtonState == LOW) {
    restoreInputState();
  } else {
    Serial.println("Clearing state!");
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
  Serial.println("Getting state!");
  int stateStored;

  EEPROM.get(stateStoredAddress, stateStored);
  if (stateStored == 1) {
    Serial.println("We have saved state!");
    EEPROM.get(stateStoreAddress, inputState);
  } else {
    Serial.println("No saved state!");
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

  setBank1State(micButtonEnabled);
  setBank2State(keyerButtonEnabled);
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
  switch (inputState.activeMicRadio) {
    case 0:
      setBank1Node0On();
      break;
    case 1:
      setBank1Node1On();
      break;
    case 2:
      setBank1Node2On();
      break;
    case 3:
      setBank1Node3On();
      break;
  }
}

/**
    Connects the Keyer to the active Radio by toggling the IO state for it.
*/
void setActiveKeyerRadio() {
  switch (inputState.activeKeyerRadio) {
    case 0:
      setBank2Node0On();
      break;
    case 1:
      setBank2Node1On();
      break;
    case 2:
      setBank2Node2On();
      break;
    case 3:
      setBank2Node3On();
      break;
  }
}

/*
    Initializes the display
*/
void initializeDisplay(){

  // Initiate the LCD:
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  Serial.println(F("SSD1306 allocation success"));

    // Clear the buffer
  display.clearDisplay();
  display.setFont(&FreeMono9pt7b);
  display.setTextSize(1); //font 1-8
  display.setTextColor(WHITE);
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
//TODO Move this to other files to seperate it out operationally

/**
    Connects the Mic to Radio port 1
*/
void setBank1Node0On() {
  digitalWrite(micBPin, LOW);
  digitalWrite(micAPin, LOW);
  setBank1State(true);
}

/**
    Connects the Mic to Radio port 2
*/
void setBank1Node1On() {
  digitalWrite(micBPin, LOW);
  digitalWrite(micAPin, HIGH);
  setBank1State(true);
}

/**
    Connects the Mic to Radio port 3
*/
void setBank1Node2On() {
  digitalWrite(micBPin, HIGH);
  digitalWrite(micAPin, LOW);
  setBank1State(true);
}

/**
    Connects the Mic to Radio port 3
*/
void setBank1Node3On() {
  digitalWrite(micBPin, HIGH);
  digitalWrite(micAPin, HIGH);
  setBank1State(true);
}

/**
    Enable or disable the entire Mic from any radio port.
*/
void setBank1State(bool state) {
  digitalWrite(micEnablePin, state ? LOW : HIGH);
}

/**
    Connects the Keyer to Radio port 1
*/
void setBank2Node0On() {
  digitalWrite(keyBPin, LOW);
  digitalWrite(keyAPin, LOW);
  setBank2State(true);
}

/**
    Connects the Keyer to Radio port 2
*/
void setBank2Node1On() {
  digitalWrite(keyBPin, LOW);
  digitalWrite(keyAPin, HIGH);
  setBank2State(true);
}

/**
    Connects the Keyer to Radio port 3
*/
void setBank2Node2On() {
  digitalWrite(keyBPin, HIGH);
  digitalWrite(keyAPin, LOW);
  setBank2State(true);
}

/**
    Connects the Keyer to Radio port 4
*/
void setBank2Node3On() {
  digitalWrite(keyBPin, HIGH);
  digitalWrite(keyAPin, HIGH);
  setBank2State(true);
}

/**
    Enable or disable the entire Keyer from any radio port.
*/
void setBank2State(bool state) {
  digitalWrite(keyEnablePin, state ? LOW : HIGH);
}
