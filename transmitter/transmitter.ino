#include "OneButton.h"
#include "SerialTransfer.h"
#include "FireTimer.h"




const int STICK_DEAD_MAX = 540;
const int STICK_DEAD_MIN = 490;

const int TRAV_MAX = 180;
const int TRAV_MID = 90;
const int TRAV_MIN = 0;

const int RECOIL_DELAY  = 2000;
const int DEPRESS_DELAY = 85;

const int DEP_MAX = 200;
const int DEP_MIN = -200;

const int STICK_OFFSET_MAX = 255;
const int STICK_OFFSET_MIN = 0;

const int SPEED_MAX = 255;
const int SPEED_MID = 0;
const int SPEED_MIN = -255;

const int VOL_PIN = A5;
const int DIF_PIN = A4;

const int X_AXIS_PIN = A0;
const int Y_AXIS_PIN = A1;

const int DEP_PIN  = A3;
const int TRAV_PIN = A2;

const int TRIG_PIN  = 4;
const int SOUND_PIN = 5;




OneButton trigBtn = OneButton(
  TRIG_PIN,    // Input pin for the button
  true,        // Button is active LOW
  true         // Enable internal pull-up resistor
);

OneButton soundBtn = OneButton(
  SOUND_PIN,   // Input pin for the button
  true,        // Button is active LOW
  true         // Enable internal pull-up resistor
);

SerialTransfer myTransfer;

FireTimer recoilDelay;
FireTimer depressDelay;

struct __attribute__((__packed__)) control
{
  int   RSpeed;
  int   LSpeed;
  float Depression;
  int   Traverse;
  bool  Fire;
  int   Volume;
  bool  Sing;
} Tonk;




static void handleTrig()
{
  if (!Tonk.Fire)
  {
    Tonk.Fire = true;
    recoilDelay.start();
  }
}




static void handleSound()
{
  Tonk.Sing = !Tonk.Sing;
}




void setup()
{
  Serial.begin(115200);
  Serial1.begin(9600);
  
  trigBtn.attachClick(handleTrig);
  soundBtn.attachClick(handleSound);
  
  myTransfer.begin(Serial1, false);
  
  recoilDelay.begin(RECOIL_DELAY);
  depressDelay.begin(DEPRESS_DELAY);
}




void loop()
{
  Buttons();
  Sound();
  Turret();
  Throttle();

  Serial.print(Tonk.RSpeed); Serial.print(" "); 
  Serial.print(Tonk.LSpeed); Serial.print(" "); 
  Serial.print(Tonk.Depression); Serial.print(" "); 
  Serial.print(Tonk.Traverse); Serial.print(" "); 
  Serial.print(Tonk.Fire); Serial.print(" "); 
  Serial.print(Tonk.Volume); Serial.print(" "); 
  Serial.print(Tonk.Sing); Serial.print(" ");
  Serial.println();
  
  myTransfer.sendDatum(Tonk);
  delay(100);
}




void Buttons()
{
  trigBtn.tick();
  soundBtn.tick();
}




void Sound()
{
  Tonk.Volume = map(analogRead(VOL_PIN), 0, 1023, 0, 30);
}




void Turret()
{
  if (recoilDelay.fire() && Tonk.Fire)
    Tonk.Fire = false;
  
  int trav = analogRead(TRAV_PIN);
  
  if (trav >= STICK_DEAD_MAX)
    Tonk.Traverse = constrain(map(trav, STICK_DEAD_MAX, 1023, TRAV_MID, TRAV_MIN), TRAV_MIN, TRAV_MID);
  else if (trav <= STICK_DEAD_MIN)
    Tonk.Traverse = constrain(map(trav, STICK_DEAD_MIN, 0, TRAV_MID, TRAV_MAX), TRAV_MID, TRAV_MAX);
  else
    Tonk.Traverse = TRAV_MID;
  
  if (depressDelay.fire())
  {
    int depr = analogRead(DEP_PIN);
    
    if (depr >= STICK_DEAD_MAX)
      Tonk.Depression += map(depr, STICK_DEAD_MAX, 1023, 0, 10);
    else if (depr <= STICK_DEAD_MIN)
      Tonk.Depression += map(depr, STICK_DEAD_MIN, 0, -10, 0);
    
    Tonk.Depression = constrain(Tonk.Depression, DEP_MIN, DEP_MAX);
  }
}




void Throttle()
{
  static bool driveDirection = true;

  int x = analogRead(X_AXIS_PIN);
  int y = analogRead(Y_AXIS_PIN);
  
  uint8_t offset = 0;

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
  
  if (x >= STICK_DEAD_MAX)
  {
    Tonk.RSpeed = SPEED_MID;
    Tonk.LSpeed = map(x, STICK_DEAD_MAX, 1023, SPEED_MID, SPEED_MIN);
  }
  else if (x <= STICK_DEAD_MIN)
  {
    Tonk.LSpeed = SPEED_MID;
    Tonk.RSpeed = map(x, STICK_DEAD_MAX, 0, SPEED_MID, SPEED_MAX);
  }
  else
  {
    Tonk.RSpeed = SPEED_MID;
    Tonk.LSpeed = SPEED_MID;
  }

  if ((offset > 0) && !driveDirection)
  {
    Tonk.RSpeed -= offset;
    Tonk.LSpeed += offset;
  }
  else if ((offset > 0) && driveDirection)
  {
    Tonk.RSpeed += offset;
    Tonk.LSpeed -= offset;
  }

  int diff = map(analogRead(DIF_PIN), 0, 1023, -90, 90);

  Tonk.RSpeed += diff;
  Tonk.LSpeed -= diff;

  Tonk.RSpeed = constrain(Tonk.RSpeed, SPEED_MIN, SPEED_MAX);
  Tonk.LSpeed = constrain(Tonk.LSpeed, SPEED_MIN, SPEED_MAX);
}
