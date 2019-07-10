#include <SoftwareSerial.h>




const unsigned long RECOIL_DELAY = 2000;
const unsigned long DEP_DELAY    = 10;

const int STICK_DEAD_MAX    = 540;
const int STICK_DEAD_MIN    = 500;

const byte SPEED_MAX        = 180;
const byte SPEED_MIN        = 0;
const byte TRAV_MAX         = 180;
const byte TRAV_MID         = 90;
const byte TRAV_MIN         = 0;
const byte STICK_OFFSET_MAX = 90;
const byte STICK_OFFSET_MIN = 0;
const byte DEP_MAX          = 105;
const byte DEP_MID          = 45;
const byte DEP_MIN          = 0;
const byte DEP_DELTA        = 1;
const byte START_OF_FRAME   = 200;
const byte X_AXIS_PIN       = 0;
const byte Y_AXIS_PIN       = 1;
const byte DEP_PIN          = 2;
const byte TRAV_PIN         = 3;
const byte TRIG_PIN         = 5;
const byte SOUND_PIN        = 10;




SoftwareSerial BTserial(2, 3); // RX | TX

struct control
{
  uint8_t RSpeed;
  uint8_t LSpeed;
  int8_t Depression;
  uint8_t Traverse;
  uint8_t Fire;
  uint8_t Volume;
  uint8_t Sing;
} Tonk;




byte current_Depression = DEP_MID;
byte currentTriggerState = 1;
byte previousTriggerState = 1;
byte debounce = 0;
byte pressDetected = 0;

byte test = 1;

byte singingCurrentTriggerState = 1;
byte singingPreviousTriggerState = 1;
byte singingDebounce = 0;
byte singingPressDetected = 0;
byte currentSingStatus = 0;

unsigned long currentTime = millis();
unsigned long timeBench = currentTime;

unsigned long dep_currentTime = millis();
unsigned long dep_timeBench = dep_currentTime;

int driveDirection = 1; // 1 - forward




void setup()
{
  Serial.begin(115200);
  BTserial.begin(9600);

  pinMode(TRIG_PIN, INPUT_PULLUP);
  pinMode(SOUND_PIN, INPUT_PULLUP);
}




void loop()
{
  int x = analogRead(X_AXIS_PIN);
  int y = analogRead(Y_AXIS_PIN);
  int depr = analogRead(DEP_PIN);
  int trav = analogRead(TRAV_PIN);
  uint8_t offset = 0;

  Tonk.Volume = map(analogRead(4), 0, 1023, 0, 30);

  if (Tonk.Fire == 1)
  {
    test = 1;
    currentTime = millis();
    timeBench = currentTime;
  }
  if (test)
  {
    currentTime = millis();
    if (currentTime - timeBench >= RECOIL_DELAY)
    {
      timeBench = currentTime;
      Tonk.Fire = checkFallingEdge();
      test = 0;
    }
    else
      Tonk.Fire = 0;
  }
  else
    Tonk.Fire = checkFallingEdge();

  if (singingCheckFallingEdge())
  {
    if(currentSingStatus)
      currentSingStatus = 0;
    else
      currentSingStatus = 1;
  }
  Tonk.Sing = currentSingStatus;

  dep_currentTime = millis();
  if (dep_currentTime - dep_timeBench > DEP_DELAY)
  {
    dep_timeBench = dep_currentTime;
    add_Depression(depr);
    Tonk.Depression = current_Depression;
  }

  if (y >= STICK_DEAD_MAX)
  {
    offset = map(y, STICK_DEAD_MAX, 1023, STICK_OFFSET_MIN, STICK_OFFSET_MAX);
    driveDirection = 0;
  }
  else if (y <= STICK_DEAD_MIN)
  {
    offset = map(y, STICK_DEAD_MIN, 0, STICK_OFFSET_MIN, STICK_OFFSET_MAX);
    driveDirection = 1;
  }
  else
    offset = 0;

  Throttle(x, offset, &Tonk.RSpeed, &Tonk.LSpeed);

  if (trav >= STICK_DEAD_MAX)
    Tonk.Traverse = map(trav, STICK_DEAD_MAX, 1023, TRAV_MID, TRAV_MIN);
  else if (trav <= STICK_DEAD_MIN)
    Tonk.Traverse = map(trav, STICK_DEAD_MIN, 0, TRAV_MID, TRAV_MAX);
  else
    Tonk.Traverse = TRAV_MID;

  BTserial.write(START_OF_FRAME);
  BTserial.write(Tonk.Fire);
  BTserial.write(Tonk.LSpeed);
  BTserial.write(Tonk.RSpeed);
  BTserial.write(Tonk.Depression);
  BTserial.write(Tonk.Traverse);
  BTserial.write(Tonk.Volume);
  BTserial.write(Tonk.Sing);

  delay(100);
}



void Throttle(int x, uint8_t offset, uint8_t * right_Speed, uint8_t * left_Speed)
{
  if (x >= STICK_DEAD_MAX)
  {
    *right_Speed = 90;
    *left_Speed = map(x, STICK_DEAD_MAX, 1023, 90, SPEED_MIN);
  }
  else if (x <= STICK_DEAD_MIN)
  {
    *left_Speed = 90;
    *right_Speed = map(x, STICK_DEAD_MAX, 0, 90, SPEED_MAX);
  }
  else
  {
    *right_Speed = 90;
    *left_Speed = 90;
  }

  if ((offset > 0) && (driveDirection == 0))
  {
    *right_Speed = *right_Speed - offset;
    *left_Speed = *left_Speed + offset;
  }
  else if ((offset > 0) && (driveDirection == 1))
  {
    *right_Speed = *right_Speed + offset;
    *left_Speed = *left_Speed - offset;
  }
  else
  {
    //do nothing
  }

  if (*right_Speed > SPEED_MAX)
    *right_Speed = SPEED_MAX;
  if (*left_Speed > SPEED_MAX)
    *left_Speed = SPEED_MAX;
  if (*right_Speed < SPEED_MIN)
    *right_Speed = SPEED_MIN;
  if (*left_Speed < SPEED_MIN)
    *left_Speed = SPEED_MIN;
}




void add_Depression(int depr)
{
  if ((depr >= STICK_DEAD_MAX) && ((current_Depression - DEP_DELTA) >= DEP_MIN) && (current_Depression > DEP_DELTA))
  {
    current_Depression -= DEP_DELTA;
  }
  else if ((depr <= STICK_DEAD_MIN) && ((current_Depression + DEP_DELTA) <= DEP_MAX))
  {
    current_Depression += DEP_DELTA;
  }
}




uint8_t checkFallingEdge()
{
  uint8_t fire = 0;

  //prime
  previousTriggerState = currentTriggerState;
  
  //poll button
  if (digitalRead(TRIG_PIN))
    currentTriggerState = 1;
  else
    currentTriggerState = 0;

  //check for falling edge
  if ((currentTriggerState == 0) && (previousTriggerState == 1))
    pressDetected = 1;
  else
    pressDetected = 0;

  //debounce after press
  if (pressDetected)
  {
    if (digitalRead(TRIG_PIN))
      debounce = (debounce << 1) | 1;
    else
      debounce = (debounce << 1) & 0xFE;

    if (!debounce)
      fire = 1;
  }

  return fire;
}




uint8_t singingCheckFallingEdge()
{
  uint8_t singing = 0;

  //prime
  singingPreviousTriggerState = singingCurrentTriggerState;
  
  //poll button
  if (digitalRead(SOUND_PIN))
    singingCurrentTriggerState = 1;
  else
    singingCurrentTriggerState = 0;

  //check for falling edge
  if ((singingCurrentTriggerState == 0) && (singingPreviousTriggerState == 1))
    singingPressDetected = 1;
  else
    singingPressDetected = 0;

  //debounce after press
  if (singingPressDetected)
  {
    if (digitalRead(SOUND_PIN))
      singingDebounce = (singingDebounce << 1) | 1;
    else
      singingDebounce = (singingDebounce << 1) & 0xFE;

    if (!singingDebounce)
      singing = 1;
  }

  return singing;
}



