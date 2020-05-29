//define all our structs and functions that use them to make the compiler happy.

struct InputState {
  int activeMicRadio = 0;
  int activeKeyerRadio = 0;
};

struct Radio {
  bool hasMic;
  bool hasKeyer;
  char nm[7]; 
};

Radio *getActiveMicRadio();

Radio *getActiveKeyerRadio();

void storeInputState();

void clearStoredInputState();

void restoreInputState();

void setButtonsEnable();

void moveToNextMicRadio();

void moveToNextKeyerRadio();

void setActiveMicRadio();

void setActiveKeyerRadio();

void initializeDisplay();

void updateDisplay();

void displayResetMessage();

Radio *getActiveKeyerRadio();

Radio *getActiveMicRadio();

void setBank1Node0On();

void setBank1Node1On();

void setBank1Node2On();

void setBank1Node3On();

void setBank2Node0On();

void setBank2Node1On();

void setBank2Node2On();

void setBank2Node3On();

void setBank1State(bool state);

void setBank2State(bool state);

