#include <Servo.h>

Servo Gun_Servo;
Servo Trav_Servo;
Servo R_Servo;
Servo L_Servo;
Servo recoil;

enum state
{
  GIT_DATA,
  DRIVE,
  SING,
  FIRE
};

state Current_State = GIT_DATA;
state Previous_State = GIT_DATA;

struct control
{
  uint8_t RSpeed;
  uint8_t LSpeed;
  int8_t Depression;
  uint8_t Traverse;
  uint8_t Fire;
  uint8_t FireDone;
  uint8_t Volume;
  uint8_t PrevVolume;
  uint8_t Sing;
  uint8_t PrevSing;
  uint8_t Driving;
  uint8_t PrevDriving;
} Tonk;

const byte PacketSize   = 8;
const byte StartOfFrame = 200;

const byte recoilMin = 7;
const byte recoilMax = 158;

unsigned long currentTime = millis();
unsigned long timeBench   = currentTime;
unsigned long recoilTime  = 200;

unsigned long PLAYcurrentTime = millis();
unsigned long PLAYtimeBench   = PLAYcurrentTime;
unsigned long duration        = 1000;

byte randNumber = 1;
byte trip       = 0;

byte BOF          = 0x7E;
byte ver          = 0xFF;
byte number       = 0x6;
byte commandValue = 0x8;
byte feedback     = 0x0;
byte paramMSB     = 0x0;
byte paramLSB     = 0x3;
byte checksumMSB  = 0x0;
byte checksumLSB  = 0x0;
byte _EOF         = 0xEF;

int checksum = 0;

void setup()
{
  Serial.begin(9600);

  Tonk.Volume      = 90;
  Tonk.Volume      = 90;
  Tonk.Depression  = 90;
  Tonk.Traverse    = 90;
  Tonk.Fire        = 0;
  Tonk.FireDone    = 1;
  Tonk.Volume      = 10;
  Tonk.PrevVolume  = 10;
  Tonk.Sing        = 0;
  Tonk.PrevSing    = 0;
  Tonk.Driving     = 0;
  Tonk.PrevDriving = 0;

  Gun_Servo.attach(5);
  Trav_Servo.attach(6);
  R_Servo.attach(7);
  L_Servo.attach(8);
  recoil.attach(9);

  Gun_Servo.write(90);
  Trav_Servo.write(90);
  R_Servo.write(90);
  L_Servo.write(90);
  recoil.write(recoilMin);

  volume(Tonk.Volume);
  findChecksum();
  sendData();

  play(8);
  findChecksum();
  sendData();

  delay(3500);

  loop(5);
  findChecksum();
  sendData();
}

void loop()
{
  switch (Current_State)
  {
    case GIT_DATA:///////////////////////////////////////////////////////GIT_DATA
      if (Serial.available() >= PacketSize)
      {
        if (checkSOF())
        {
          readData();
          writeToServos();
          adjustVolume();

          if ((Tonk.PrevSing == 1) && (Tonk.Sing == 0))
          {
            if (Tonk.Driving)
            {
              play(4);    //goinglongtime
              findChecksum();
              sendData();
            }
            else
            {
              play(5);    //idlelongtime
              findChecksum();
              sendData();
            }
          }

          if ((Tonk.PrevSing == 0) && (Tonk.Sing == 1))
          {
            Current_State = SING;
            Previous_State = GIT_DATA;
          }
          else if (Tonk.Driving != Tonk.PrevDriving)
          {
            Current_State = DRIVE;
            Previous_State = GIT_DATA;
          }
          else if (Tonk.Fire)
          {
            Current_State = FIRE;
            Previous_State = GIT_DATA;
          }
          else
          {
            //do nothing
          }
        }
      }
      break;

    case DRIVE:///////////////////////////////////////////////////////DRIVE
      if ((Tonk.Sing == 0) && (Tonk.FireDone == 1))
      {
        if ((Tonk.PrevDriving == 0) && (Tonk.Driving == 1))
        {
          play(6);        //revupandgo
          findChecksum();
          sendData();
        }
        else
        {
          play(7);        //slowingnotgoing
          findChecksum();
          sendData();
        }
      }

      if (Tonk.Fire)
      {
        Current_State  = FIRE;
        Previous_State = DRIVE;
      }
      else
      {
        Current_State  = GIT_DATA;
        Previous_State = DRIVE;
      }
      break;

    case SING:///////////////////////////////////////////////////////SING
      playSong();

      if (Tonk.Driving != Tonk.PrevDriving)
      {
        Current_State  = DRIVE;
        Previous_State = SING;
      }
      else if (Tonk.Fire)
      {
        Current_State  = FIRE;
        Previous_State = SING;
      }
      else
      {
        Current_State  = GIT_DATA;
        Previous_State = SING;
      }
      break;

    case FIRE:///////////////////////////////////////////////////////FIRE
      if (Tonk.Sing == 0)
      {
        currentTime = millis();
        timeBench   = currentTime;
        recoil.write(recoilMax);
        if (Tonk.Driving)
        {
          play(3);
          findChecksum();
          sendData();
          duration = 3000;
        }
        else
        {
          play(2);
          findChecksum();
          sendData();
          duration = 4000;
        }
        Tonk.FireDone = 0;
      }

      Current_State = GIT_DATA;
      Previous_State = FIRE;
      break;

    default:///////////////////////////////////////////////////////default
      while (1); //HALT
      break;
  }

  if (Tonk.FireDone == 0)
  {
    currentTime = millis();
    if ((currentTime - timeBench) >= duration)
    {
      timeBench = currentTime;

      if ((Tonk.Driving == 1) && (Tonk.Sing == 0))
      {
        play(4);
        findChecksum();
        sendData();
      }
      else
      {
        play(5);
        findChecksum();
        sendData();
      }
      Tonk.FireDone = 1;
    }
    else if((currentTime - timeBench) >= recoilTime)
    {
      recoil.write(recoilMin);
    }
    else
    {
      //do nothing
    }
  }
}

void findChecksum()
{
  checksum = (~(ver + number + commandValue + feedback + paramMSB + paramLSB)) + 1;

  checksumMSB = checksum >> 8;
  checksumLSB = checksum & 0xFF;

  return;
}

void volume(byte x)
{
  commandValue = 6;
  paramMSB = 0;
  paramLSB = x;

  return;
}

void loop(byte x)
{
  commandValue = 8;
  paramMSB = 0;
  paramLSB = x;

  return;
}

void play(byte x)
{
  commandValue = 3;
  paramMSB = 0;
  paramLSB = x;

  return;
}

void sendData()
{
  Serial.write(BOF);
  Serial.write(ver);
  Serial.write(number);
  Serial.write(commandValue);
  Serial.write(feedback);
  Serial.write(paramMSB);
  Serial.write(paramLSB);
  Serial.write(checksumMSB);
  Serial.write(checksumLSB);
  Serial.write(_EOF);

  return;
}

byte checkSOF()
{
  byte result = 0;

  if (Serial.read() == StartOfFrame)
  {
    result = 1;
  }
  else
  {
    result = 0;
  }

  return result;
}

void readData()
{
  Tonk.PrevSing    = Tonk.Sing;
  Tonk.PrevDriving = Tonk.Driving;
  Tonk.PrevVolume  = Tonk.Volume;
  Tonk.Fire        = Serial.read();
  Tonk.RSpeed      = Serial.read();
  Tonk.LSpeed      = Serial.read();
  Tonk.Depression  = Serial.read();
  Tonk.Traverse    = Serial.read();
  Tonk.Volume      = Serial.read();
  Tonk.Sing        = Serial.read();

  if ((Tonk.RSpeed != 90) || (Tonk.LSpeed != 90))
  {
    Tonk.Driving = 1;
  }
  else
  {
    Tonk.Driving = 0;
  }

  return;
}

void writeToServos()
{
  R_Servo.write(Tonk.RSpeed);
  L_Servo.write(Tonk.LSpeed);
  Gun_Servo.write(Tonk.Depression);
  Trav_Servo.write(Tonk.Traverse);

  return;
}

void playSong()
{
  if (randNumber == 1)
  {
    loop(1);
  }
  else if (randNumber == 2)
  {
    loop(9);
  }
  else if (randNumber == 3)
  {
    loop(10);
  }
  else if (randNumber == 4)
  {
    loop(11);
  }
  else if (randNumber == 5)
  {
    loop(12);
  }
  else if (randNumber == 6)
  {
    loop(13);
  }
  else if (randNumber == 7)
  {
    loop(14);
  }
  else if (randNumber == 8)
  {
    loop(15);
  }
  else
  {
    //do nothing
  }

  randNumber = randNumber + 1;

  if (randNumber == 9)
  {
    randNumber = 1;
  }

  findChecksum();
  sendData();

  return;
}

//return to ready state
void firin_Mah_Laza()
{
  currentTime = millis();
  if ((currentTime - timeBench) >= recoilTime)
  {
    recoil.write(recoilMin);
    timeBench = currentTime;
    Tonk.FireDone = 1;
  }

  return;
}

void adjustVolume()
{
  if (Tonk.PrevVolume != Tonk.Volume)
  {
    volume(Tonk.Volume);
    findChecksum();
    sendData();
  }

  return;
}
