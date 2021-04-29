#include <Servo.h>
#include "MPU9250.h"
#include "SerialTransfer.h"
#include "DFPlayerMini_Fast.h"
#include "FireTimer.h"




const int STAB_SERVO_MIN = 1000;
const int STAB_SERVO_MAX = 2000;

const int STAB_ANGLE_MIN = -90;
const int STAB_ANGLE_MAX = 90;

const byte RECOIL_MIN = 7;
const byte RECOIL_MAX = 158;

const byte GUN_PIN    = 6;
const byte TRAV_PIN   = 5;
const byte R_PIN      = 3;
const byte L_PIN      = 4;
const byte RECOIL_PIN = 7;

const byte SONGS[] = { 1, 9, 10, 11, 12, 13, 14, 15 };




Servo Gun_Servo;
Servo Trav_Servo;
Servo R_Servo;
Servo L_Servo;
Servo recoil;

MPU9250 mpu;
SerialTransfer myTransfer;
DFPlayerMini_Fast myMP3;
FireTimer recoilSound;
FireTimer recoilMotion;
FireTimer drivingSound;




struct control
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

float yaw;
float pitch;
float roll;

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
  Serial.begin(9600);

  Gun_Servo.attach(GUN_PIN);
  Trav_Servo.attach(TRAV_PIN);
  R_Servo.attach(R_PIN);
  L_Servo.attach(L_PIN);
  recoil.attach(RECOIL_PIN);

  Gun_Servo.write(90);
  Trav_Servo.write(90);
  R_Servo.write(90);
  L_Servo.write(90);
  recoil.write(RECOIL_MIN);

  Wire.begin();

  delay(2000);

  mpu.setup(0x68);
  myTransfer.begin(Serial, false);
  myMP3.begin(Serial, false);
  recoilSound.begin(1);
  recoilMotion.begin(1000);
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

    R_Servo.write(Tonk.RSpeed);
    L_Servo.write(Tonk.LSpeed);
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

  if (mpu.update())
  {
    yaw   = mpu.getYaw();
    pitch = mpu.getPitch();
    roll  = mpu.getRoll();
  }

  stabDepress = map(constrain(pitch + Tonk.Depression, STAB_ANGLE_MIN, STAB_ANGLE_MAX), STAB_ANGLE_MIN, STAB_ANGLE_MAX, STAB_SERVO_MIN, STAB_SERVO_MAX);
  Gun_Servo.writeMicroseconds(stabDepress);
}
