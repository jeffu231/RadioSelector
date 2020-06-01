//define all our structs and functions that use them to make the compiler happy.

struct InputState {
  int activeMicRadio = 0;
  int activeKeyerRadio = 0;
};

struct Radio {
  char nm[7]; //Radio name
  bool hasMic;
  bool hasKeyer;
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

void setKeyerOn(int index);

void setKeyerDisabled();

void setMicOn(int index);

void setMicDisabled();

void writeOutput(byte output);
