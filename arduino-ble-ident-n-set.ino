// Arduino BLE module identification and setup sketch
// Copyright, Arik Yavilevich

#ifdef __AVR__ // supports modules connected over software serial
#include <SoftwareSerial.h>
#endif

/// Consts

// BLE module default pins
#ifdef ESP32
#define RX_PIN 16
#define TX_PIN 17
#define STATE_PIN 5
#else
#define RX_PIN 8 //BLE TX -> ARDUINO PIN 8
#define TX_PIN 9 //BLE RX -> ARDUINO PIN 9
#define STATE_PIN 7
#endif
#define STATE_PIN_MISSING -1

// misc
#define SERIAL_BAUD 115200
#define SERIAL_TIMEOUT 60000 // 1 min
#define BLE_BAUD_DEFAULT 9600 // for AT mode and for data mode (CC41, HM-10 and MLT-BT05)
#define BLE_TIMEOUT 250 // 100ms was ok for CC41 and HM-10 but not for MLT_BT05's AT+HELP command
#define INITIAL_DELAY 200 // in ms

enum ModuleType { HM10, CC41, MLT_BT05, Unknown };
enum Operation {Quit, SetName, SetPass, SetStateBehavior, SetPower, SetBindingType, SetBaud, DisplayMainSettings, RestoreDefaults, Reboot, ReIdentifyDevice, DetermineConnectionState, SetRole, SendCommand};
enum ConnectionState {NoStatePin, Blinking, Connected, Disconnected};

// Support hardware serials and predefined serials
#ifdef ESP32
HardwareSerial Serial2(2);
#define SPECIAL_SERIAL Serial2
#endif

/// State
Stream * ble = NULL; // common interface of hardware/software serial
int rxPin, txPin, statePin;
long ble_baud = BLE_BAUD_DEFAULT; //can be changed in menu
ModuleType moduleType;

/// Special functions
// define here due to custom return type
ModuleType identifyDevice();
ConnectionState getConnectionState();
void doCommandAndEchoResult(const char * command, const __FlashStringHelper * meaning = NULL);
Operation getMenuSelection();

// Serial line end modes
// CR \r
// LF \n
// CR&LF \r\n

void setup()
{
  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(SERIAL_TIMEOUT);
  delay(INITIAL_DELAY);

  Serial.println(F("Arduino BLE module identification and setup sketch."));
  Serial.println(F("Interact with this interface using serial in CR&LF mode."));
  // Serial.println(F(""));
  do
  {
    do
    {
      openBLE();
    } while (determineInitialConnectionState() == false);
  } while ((moduleType=identifyDevice()) == Unknown);
  displayMainSettings();
  do
  {
    Operation op = getMenuSelection();
    switch (op)
    {
    case SetName:
      setName();
      break;
    case SetPass:
      setPass();
      break;
    case SetStateBehavior:
      setStateBehavior();
      break;
    case SetPower:
      setPower();
      break;
    case SetBindingType:
      setBindingType();
      break;
    case DisplayMainSettings:
      displayMainSettings();
      break;
    case RestoreDefaults:
      restoreDefaults();
      break;
    case Reboot:
      reboot();
      break;
    case ReIdentifyDevice:
      moduleType = identifyDevice();
      break;
    case DetermineConnectionState:
      printConnectionState();
      break;
    case SetRole:
      setRole();
      break;
    case SendCommand:
      sendCommand();
      break;
    case SetBaud: 
      setBaud();
      break;
    default: // timeout and no option was selected
      Serial.println(F("Quitting. Sketch ended."));
      return;
    }
  } while (true);
}

/// BL functions

void openBLE()
{
  rxPin=readInt(F("Enter the number of the RX pin on the Arduino, TX on the module"), RX_PIN);
  txPin = readInt(F("Enter the number of the TX pin on the Arduino, RX on the module"), TX_PIN);
  statePin = readInt(F("Enter the number of the State pin on the Arduino, State on the module (enter -1 if not present or not connected)"), STATE_PIN);

  ble_baud = readLong(F("Enter the ble baud rate, enter -1 for default"), BLE_BAUD_DEFAULT);

  if(ble_baud == -1)
    ble_baud = BLE_BAUD_DEFAULT;

  Serial.print(F("Opening serial connection to BLE module at pins: "));
  Serial.print(rxPin);
  Serial.print(F(", "));
  Serial.print(txPin);
  Serial.print(F(", "));
  Serial.print(statePin);
  Serial.println();

  // open and create object
#ifndef SPECIAL_SERIAL
  if (ble)
    delete ble;
  SoftwareSerial * ss = new SoftwareSerial(rxPin, txPin);
  ss->begin(ble_baud);
  ble = ss;
#else
  ble = &SPECIAL_SERIAL;
  SPECIAL_SERIAL.begin(ble_baud, SERIAL_8N1, rxPin, txPin);
#endif
  ble->setTimeout(BLE_TIMEOUT);
  if(statePin!=STATE_PIN_MISSING)
    pinMode(statePin, INPUT);
}

ConnectionState getConnectionState()
{
  if (statePin == STATE_PIN_MISSING)
    return NoStatePin;

  // read state
  int state = digitalRead(statePin);
  // check if state is "blinking"
  unsigned long p1 = pulseIn(statePin, HIGH, 2000000);
  unsigned long p2 = pulseIn(statePin, LOW, 2000000);
  if (p1 == p2 && p1 == 0) // if no pulse detected
  {
    if (state == HIGH)
      return Connected;
    else
      return Disconnected;
  }

  return Blinking;
}

// perform initial connection state detection including sending user messages/instructions
// returns true is can detect that the device is in data mode
// returns false if can't dtermine or if in command mode
bool determineInitialConnectionState()
{
  if (statePin != STATE_PIN_MISSING)
    Serial.println(F("Checking module state..."));

  switch (getConnectionState())
  {
  case Connected:
    Serial.println(F("The signal on the state pin is HIGH. This means the device is connected and is in data mode. Sketch can't proceed."));
    return false;
  case Disconnected:
    Serial.println(F("The signal on the state pin is LOW. This means the device is not connected and is in command mode."));
    break;
  case Blinking:
    Serial.println(F("The signal on the state pin is \"blinking\". This usually means the device is not connected and is in command mode."));
    Serial.println(F("Consider reconfiguring the device to pass the state as-is. So it is simple to detect the state."));
    break;
  case NoStatePin:
    Serial.println(F("For this sketch, make sure the module is not connected to another BLE device."));
    Serial.println(F("This will make sure the device is in command mode."));
    Serial.println(F("A led on the module should be off or blinking."));
    break;
  default:
    Serial.println(F("Unknown connection state."));
    break;
  }

  return true;
}

void printConnectionState()
{
  Serial.println(F("Checking module state..."));
  switch (getConnectionState())
  {
  case Connected:
    Serial.println(F("The signal on the state pin is HIGH. This means the device is connected and is in data mode. Can not send commands."));
    break;
  case Disconnected:
    Serial.println(F("The signal on the state pin is LOW. This means the device is not connected and is in command mode."));
    break;
  case Blinking:
    Serial.println(F("The signal on the state pin is \"blinking\". This usually means the device is not connected and is in command mode."));
    Serial.println(F("Consider reconfiguring the device to pass the state as-is. So it is simple to detect the state."));
    break;
  case NoStatePin:
    Serial.println(F("No state pin was defined, can't detect connection state programmatically."));
    break;
  default:
    Serial.println(F("Unknown connection state."));
    break;
  }
}

ModuleType identifyDevice()
{
  Serial.println(F("Detecting module type"));
  // check for HM-10
  ble->print("AT");
  String s = ble->readString();
  if (s.length() == 2 && s.compareTo("OK")==0)
  {
    Serial.println(F("HM-10 detected!"));
    return HM10;
  }
  else if (s.length() == 0)
  {
    // check for CC41
    ble->println("");
    s = ble->readString();
    if (s.length() == 4 && s.compareTo("OK\r\n") == 0)
    {
      Serial.println(F("CC41 detected!")); // An HM-10 clone
      return CC41;
    }
    else if (s.length() == 0)
    {
      // check for MLT_BT05
      ble->println("AT");
      s = ble->readString();
      if (s.length() == 4 && s.compareTo("OK\r\n") == 0)
      {
        Serial.println(F("MLT-BT05 detected!")); // An HM-10 clone
        return MLT_BT05;
      }
      else if (s.length() == 0)
      {
        Serial.println(F("No response received from module."));
        Serial.println(F("Verify that the module is powered. Is the led blinking?"));
        Serial.println(F("Check that pins are correctly connected and the right values were entered."));
        Serial.println(F("Are you using a logic voltage converter for a module that already has such logic on board?"));
        Serial.println(F("Are you using a module that expects 3.3v logic? You might need to do logic convertion on Arduio's TX pin (module's RX pin)."));
      }
      else
      {
        Serial.print(F("Unexpected result of length="));
        Serial.println(s.length());
        Serial.println(s);
      }
    }
    else
    {
      Serial.print(F("Unexpected result of length="));
      Serial.println(s.length());
      Serial.println(s);
    }
  }
  else
  {
    Serial.print(F("Unexpected result of length="));
    Serial.println(s.length());
    Serial.println(s);
  }
  return Unknown;
}

void displayMainSettings()
{
  if (moduleType == HM10)
  {
    doCommandAndEchoResult(("AT+HELP?"));
    doCommandAndEchoResult(("AT+VERS?"));
    doCommandAndEchoResult(("AT+NAME?"));
    doCommandAndEchoResult(("AT+PASS?"));
    doCommandAndEchoResult(("AT+ADDR?"));
    doCommandAndEchoResult(("AT+ROLE?"), F("Peripheral=0, Central=1"));
    doCommandAndEchoResult(("AT+POWE?"), F("0 = -23dbm, 1 = -6dbm, 2 = 0dbm, 3 = 6dbm"));
    doCommandAndEchoResult(("AT+TYPE?"), F("0 = Not need PIN Code, 1 = Auth not need PIN, 2 = Auth with PIN, 3 = Auth and bond"));
    doCommandAndEchoResult(("AT+MODE?"), F("Transmission Mode=0, PIO collection Mode=1, Remote Control Mode=2"));
    doCommandAndEchoResult(("AT+PIO1?"), F("Behavior of state pin, Blink on disconnect=0, Off on disconnect=1"));
  }
  else if (moduleType == CC41)
  {
    doCommandAndEchoResult(("AT+HELP"));
    doCommandAndEchoResult(("AT+VERSION"));
    doCommandAndEchoResult(("AT+NAME"));
    doCommandAndEchoResult(("AT+PASS"));
    doCommandAndEchoResult(("AT+ADDR"));
    doCommandAndEchoResult(("AT+ROLE"));
    doCommandAndEchoResult(("AT+POWE"), F("0 = -23dbm, 1 = -6dbm, 2 = 0dbm, 3 = 6dbm"));
    doCommandAndEchoResult(("AT+TYPE"), F("0 = No binding, 3 = Do binding (not documented)"));
  }
  else if (moduleType == MLT_BT05)
  {
    doCommandAndEchoResult(("AT+HELP"));
    doCommandAndEchoResult(("AT+VERSION"));
    doCommandAndEchoResult(("AT+NAME"));
    doCommandAndEchoResult(("AT+PIN"));
    doCommandAndEchoResult(("AT+LADDR"));
    doCommandAndEchoResult(("AT+ROLE"), F("0 = Slave, 1 = Master"));
    doCommandAndEchoResult(("AT+POWE"), F("0 = -23dbm, 1 = -6dbm, 2 = 0dbm, 3 = 6dbm"));
    doCommandAndEchoResult(("AT+TYPE"), F("0 = No password, 1 = Password pairing, 2 = Password pairing and binding, 3 = Not documented"));
  }
}

Operation getMenuSelection()
{
  Serial.println(F("0) Quit"));
  Serial.println(F("1) Set module name"));
  Serial.println(F("2) Set module password"));
  if(moduleType==HM10)
    Serial.println(F("3) Set module state pin behavior"));
  Serial.println(F("4) Set module power"));
  Serial.println(F("5) Set module binding type"));
  Serial.println(F("6) Set module baud"));
  Serial.println(F("7) Display main settings"));
  Serial.println(F("8) Restore default settings"));
  Serial.println(F("9) Reboot/reset/restart"));
  Serial.println(F("10) Re-identify module"));
  Serial.println(F("11) Detect connection state"));
  Serial.println(F("12) Set role"));
  Serial.println(F("13) Send custom command"));
  int op = readInt(F("Enter menu selection"), 0);
  return (Operation)(op);
}

void setName()
{
  String val = readString(F("Enter new name"), F("HMSoft"));
  String command(F("AT+NAME"));
  command += val;
  doCommandAndEchoResult(command.c_str());
}

void setPass()
{
  String val = readString(F("Enter new pass/pin"), F("000000"));
  String command(moduleType ==MLT_BT05? F("AT+PIN") : F("AT+PASS"));
  command += val;
  doCommandAndEchoResult(command.c_str());
}

void setStateBehavior()
{
  String val = readString(F("Enter new state pin behavior 0 or 1"), F("1"));
  String command(F("AT+PIO1"));
  command += val;
  doCommandAndEchoResult(command.c_str());
  doCommandAndEchoResult("AT+RESET", F("to apply the AT+PIO1 command"));
}

void setPower()
{
  int dbm = readInt(F("Enter new module power (0 = -23dbm, 1 = -6dbm, 2 = 0dbm, 3 = 6dbm)"), 2); // 2 is the default
  String command(F("AT+POWE"));
  command += dbm;
  doCommandAndEchoResult(command.c_str());
}

void setRole()
{
  int val = readInt(F("Enter new role (0 = Slave, 1 = Master)"), 0); // 0 is the default
  String command(F("AT+ROLE"));
  command += val;
  doCommandAndEchoResult(command.c_str());
}

void setBaud()
{
  int baud = readInt(F("Enter new baud\n1=1200\n2=2400\n3=4800\n4=9600\n5=19200\n6=38400\n7=57600\n8=115200\n9=230400"), 4); // 9600 is the default
  if(baud < 1 || baud>9)
  {
    Serial.print("Invalid baud rate selected: ");
    Serial.println(baud);
    Serial.println("baud must be between 1-9");
    return;
  }
  String command(F("AT+BAUD"));
  command += baud;
  Serial.println(command.c_str());
  doCommandAndEchoResult(command.c_str());
}

void sendCommand()
{
  String command = readString(F("Enter a command"),F("AT")); // AT is the default
  doCommandAndEchoResult(command.c_str());
}

void setBindingType()
{
  int dbm = readInt(F("Enter new module binding type (options depend on module, see printout of main settings)"), 0); // 0 is the default
  String command(F("AT+TYPE"));
  command += dbm;
  doCommandAndEchoResult(command.c_str());
}

void restoreDefaults()
{
  doCommandAndEchoResult(moduleType == HM10?"AT+RENEW":"AT+DEFAULT");
}

void reboot()
{
  doCommandAndEchoResult("AT+RESET");
}

/// Interface helper functions

int getLengthWithoutTerminator(String & s)
{
  int l = s.length();
  if (l == 0)
    return 0;
  if (s.charAt(l - 1) == '\r')
    return l - 1;
  return l;
}

int readInt(const __FlashStringHelper * message, int defaultValue)
{
  Serial.print(message);
  Serial.print(" [");
  Serial.print(defaultValue);
  Serial.println("] :");

  String s = Serial.readStringUntil('\n');
  if (getLengthWithoutTerminator(s) == 0)
    return defaultValue;
  return s.toInt();
}

long readLong(const __FlashStringHelper * message, long defaultValue)
{
  Serial.print(message);
  Serial.print(" [");
  Serial.print(defaultValue);
  Serial.println("] :");

  String s = Serial.readStringUntil('\n');
  if (getLengthWithoutTerminator(s) == 0)
    return defaultValue;
  return s.toInt();
}

String readString(const __FlashStringHelper * message, const __FlashStringHelper * defaultValue)
{
  Serial.print(message);
  Serial.print(" [");
  Serial.print(defaultValue);
  Serial.println("] :");

  String s = Serial.readStringUntil('\n');
  int l = getLengthWithoutTerminator(s);
  if (l == 0)
    return defaultValue;
  s.remove(l); // remove terminator
  return s;
}

void doCommandAndEchoResult(const char * command, const __FlashStringHelper * meaning)
{
  // announce command
  Serial.print(F("Sending command: "));
  Serial.print(command);
  if (meaning != NULL)
  {
    Serial.print(F(" ("));
    Serial.print(meaning);
    Serial.print(F(")"));
  }
  Serial.println();

  // send command
  if (moduleType == HM10)
    ble->print(command);
  else
    ble->println(command);

  // read and return response
  // don't use "readString", it can't handle long and slow responses (such as AT+HELP) well
  byte b;
  while (ble->readBytes(&b, 1) == 1) // use "readBytes" and not "read" due to the timeout support
    Serial.write(b);

  // normalize line end
  if (moduleType == HM10)
    Serial.println();
}

void loop()
{
}
