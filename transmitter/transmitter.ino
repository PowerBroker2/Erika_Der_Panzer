#include <SoftwareSerial.h>
SoftwareSerial BTserial(2, 3); // RX | TX

const int highBuffer = 550;
const int lowBuffer = 510;

const byte maxSpeed = 180;
const byte minSpeed = 0;
const byte maxTrav = 180;
const byte MIDTrav = 90;
const byte minTrav = 0;
const byte maxOffset = 90;
const byte minOffset = 0;
const byte maxTurret = 180;
const byte MIDTurret = 120;
const byte minTurret = 90;
const byte StartOfFrame = 200;
const byte X_Axis_Pin = 0;
const byte Y_Axis_Pin = 1;
const byte depression_Pin = 2;
const byte traverse_Pin = 3;
const byte trigger_Pin = 5;
const byte singing_Pin = 10;

byte current_Depression = MIDTurret;
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
unsigned long recoilTime = 2000;

int driveDirection = 1; //forward

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

void setup()
{
  Serial.begin(115200);
  BTserial.begin(9600);

  pinMode(trigger_Pin, INPUT_PULLUP);
  pinMode(singing_Pin, INPUT_PULLUP);
}



void loop()
{
  int x = analogRead(X_Axis_Pin);
  int y = analogRead(Y_Axis_Pin);
  int depr = analogRead(depression_Pin);
  int trav = analogRead(traverse_Pin);
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
    if (currentTime - timeBench >= recoilTime)
    {
      timeBench = currentTime;
      Tonk.Fire = checkFallingEdge();
      test = 0;
    }
    else
    {
      Tonk.Fire = 0;
    }
  }
  else
  {
    Tonk.Fire = checkFallingEdge();
  }

  if (singingCheckFallingEdge())
  {
    if(currentSingStatus)
    {
      currentSingStatus = 0;
    }
    else
    {
      currentSingStatus = 1;
    }
  }
  Tonk.Sing = currentSingStatus;
  
  add_Depression(depr);
  Tonk.Depression = current_Depression;

  //////////////////////////////ADD turret traverse and traverse direction
  if (y >= highBuffer)
  {
    offset = map(y, highBuffer, 1023, minOffset, maxOffset);
    driveDirection = 0;
  }
  else if (y <= lowBuffer)
  {
    offset = map(y, lowBuffer, 0, minOffset, maxOffset);
    driveDirection = 1;
  }
  else
  {
    offset = 0;
  }

  Throttle(x, offset, &Tonk.RSpeed, &Tonk.LSpeed);

  if (trav >= highBuffer)
  {
    Tonk.Traverse = map(trav, highBuffer, 1023, MIDTrav, minTrav);
  }
  else if (trav <= lowBuffer)
  {
    Tonk.Traverse = map(trav, lowBuffer, 0, MIDTrav, maxTrav);
  }
  else
  {
    Tonk.Traverse = MIDTrav;
  }

  BTserial.write(StartOfFrame);
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
  if (x >= highBuffer)
  {
    *right_Speed = 90;
    *left_Speed = map(x, highBuffer, 1023, 90, maxSpeed);
  }
  else if (x <= lowBuffer)
  {
    *left_Speed = 90;
    *right_Speed = map(x, highBuffer, 0, 90, minSpeed);
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

  if (*right_Speed > maxSpeed)
  {
    *right_Speed = maxSpeed;
  }
  if (*left_Speed > maxSpeed)
  {
    *left_Speed = maxSpeed;
  }
  if (*right_Speed < minSpeed)
  {
    *right_Speed = minSpeed;
  }
  if (*left_Speed < minSpeed)
  {
    *left_Speed = minSpeed;
  }

  return;
}



void add_Depression(int depr)
{

  if (depr >= highBuffer)
  {
    current_Depression = current_Depression - map(depr, highBuffer, 1023, 0, 1);
  }
  else if (depr <= lowBuffer)
  {
    current_Depression = current_Depression + map(depr, lowBuffer, 0, 0, 1);
  }

  //Serial.println(depression);

}



uint8_t checkFallingEdge()
{
  uint8_t fire = 0;

  //prime
  previousTriggerState = currentTriggerState;
  //poll button
  if (digitalRead(trigger_Pin))
  {
    currentTriggerState = 1;
  }
  else
  {
    currentTriggerState = 0;
  }

  //check for falling edge
  if ((currentTriggerState == 0) && (previousTriggerState == 1))
  {
    pressDetected = 1;
  }
  else
  {
    pressDetected = 0;
  }

  //debounce after press
  if (pressDetected)
  {
    if (digitalRead(trigger_Pin))
    {
      debounce = (debounce << 1) | 1;
    }
    else
    {
      debounce = (debounce << 1) & 0xFE;
    }

    if (!debounce)
    {
      fire = 1;
    }
  }

  return fire;
}



uint8_t singingCheckFallingEdge()
{
  uint8_t singing = 0;

  //prime
  singingPreviousTriggerState = singingCurrentTriggerState;
  //poll button
  if (digitalRead(singing_Pin))
  {
    singingCurrentTriggerState = 1;
  }
  else
  {
    singingCurrentTriggerState = 0;
  }

  //check for falling edge
  if ((singingCurrentTriggerState == 0) && (singingPreviousTriggerState == 1))
  {
    singingPressDetected = 1;
  }
  else
  {
    singingPressDetected = 0;
  }

  //debounce after press
  if (singingPressDetected)
  {
    if (digitalRead(singing_Pin))
    {
      singingDebounce = (singingDebounce << 1) | 1;
    }
    else
    {
      singingDebounce = (singingDebounce << 1) & 0xFE;
    }

    if (!singingDebounce)
    {
      singing = 1;
    }
  }

  return singing;
}
