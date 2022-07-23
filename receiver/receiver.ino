#include <Servo.h>
#include "SerialTransfer.h"
#include "DFPlayerMini_Fast.h"
#include "FireTimer.h"
#include "MotorController.h"




const int STAB_SERVO_MIN = 1000;
const int STAB_SERVO_MAX = 2000;

const int STAB_ANGLE_MIN = -90;
const int STAB_ANGLE_MAX = 90;

const byte RECOIL_MIN = 7;
const byte RECOIL_MAX = 158;

const byte GUN_PIN    = 5;
const byte TRAV_PIN   = 4;
const byte R_PIN      = 2;
const byte L_PIN      = 3;
const byte RECOIL_PIN = 6;

const byte SONGS[] = { 1, 9, 10, 11, 12, 13, 14, 15 };




Servo Gun_Servo;
Servo Trav_Servo;
Servo recoil;

SerialTransfer myTransfer;
DFPlayerMini_Fast myMP3;
FireTimer recoilSound;
FireTimer recoilMotion;
FireTimer drivingSound;
DCMotorController motorR = DCMotorController(R_PIN, 17, 16); // pwmPin, in1, in2
DCMotorController motorL = DCMotorController(L_PIN, 15, 14); // pwmPin, in1, in2




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




byte songNum = 0;

int stabDepress;
int trackDuration;

int prevVolume;
bool prevSing;
bool prevdriving;
bool prevFire;

bool driving;
bool firing;
bool recoiling;




void setup()
{
  Serial.begin(115200);
  Serial1.begin(9600);

  Gun_Servo.attach(GUN_PIN);
  Trav_Servo.attach(TRAV_PIN);
  recoil.attach(RECOIL_PIN);

  Gun_Servo.write(90);
  Trav_Servo.write(90);
  recoil.write(RECOIL_MIN);

  myTransfer.begin(Serial1, true, Serial);
  myMP3.begin(Serial, false);
  recoilSound.begin(1);
  recoilMotion.begin(1000);

  Serial.print("RSpeed ");
  Serial.print("LSpeed ");
  Serial.print("Depression ");
  Serial.print("Traverse ");
  Serial.print("Fire ");
  Serial.print("Volume ");
  Serial.println("Sing");
}




void loop()
{
  if (myTransfer.available())
  {
    prevdriving = driving;
    prevVolume  = Tonk.Volume;
    prevSing    = Tonk.Sing;
    prevFire    = Tonk.Fire;

    myTransfer.rxObj(Tonk);

    Serial.print(Tonk.RSpeed); Serial.print(' ');
    Serial.print(Tonk.LSpeed); Serial.print(' ');
    Serial.print(Tonk.Depression); Serial.print(' ');
    Serial.print(Tonk.Traverse); Serial.print(' ');
    Serial.print(Tonk.Fire); Serial.print(' ');
    Serial.print(Tonk.Volume); Serial.print(' ');
    Serial.println(Tonk.Sing);

    motorR.write(Tonk.RSpeed);
    motorL.write(Tonk.LSpeed);
    Trav_Servo.write(Tonk.Traverse);

    if (Tonk.RSpeed || Tonk.LSpeed)
      driving = true;
    else
      driving = false;

    if (prevVolume != Tonk.Volume)
      myMP3.volume(Tonk.Volume);

    if (prevSing != Tonk.Sing) // Sound state change
    {
      if (Tonk.Sing) // Start playing music
      {
        // Reset gun just in case
        recoil.write(RECOIL_MIN);

        // Play next song
        myMP3.loop(SONGS[songNum]);

        songNum++;

        // Reset to keep song number in bounds
        if (songNum >= (sizeof(SONGS) / sizeof(SONGS[0])))
          songNum = 0;
      }
      else // Stop playing music
      {
        if (driving)
          myMP3.play(4); // driving
        else
          myMP3.play(5); // Idling
      }
    }
    else if (Tonk.Sing && !prevFire && Tonk.Fire) // Play next song
    {
      // Play next song
      myMP3.loop(SONGS[songNum]);

      songNum++;

      // Reset to keep song number in bounds
      if (songNum >= (sizeof(SONGS) / sizeof(SONGS[0])))
        songNum = 0;
    }
    else if (!Tonk.Sing) // Normal tank sounds
    {
      if (!prevFire && Tonk.Fire) // Fire
      {
        firing    = true;
        recoiling = true;

        recoilMotion.start();
        
        if (driving)
        {
          myMP3.play(3); // Shoot while driving
          recoilSound.begin(3000);
        }
        else
        {
          myMP3.play(2); // Shoot idling
          recoilSound.begin(4000);
        }

        recoil.write(RECOIL_MAX);
      }
      else if (!firing && (prevdriving != driving)) // Normal driving state change
      {
        if (driving) // Start moving
          myMP3.play(6); // revupandgo
        else // Stop moving
          myMP3.play(7); // slowingnotgoing
      }
    }
  }

  if (firing && recoilSound.fire())
    firing = false;

  if (recoiling && recoilMotion.fire())
  {
    recoil.write(RECOIL_MIN);
    recoiling = false;
  }

  stabDepress = map(constrain(Tonk.Depression, STAB_ANGLE_MIN, STAB_ANGLE_MAX), STAB_ANGLE_MIN, STAB_ANGLE_MAX, STAB_SERVO_MIN, STAB_SERVO_MAX);
  Gun_Servo.writeMicroseconds(stabDepress);
}
