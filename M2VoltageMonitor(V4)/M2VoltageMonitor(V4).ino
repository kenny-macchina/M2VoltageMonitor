//Macchina LLC
//Kenny Truong
//5-31-19

#include <Arduino_Due_SD_HSMCI.h>
#include "Arduino.h"
#include <M2_12VIO.h>

FileStore FS;
M2_12VIO M2IO;

float temp=0;

float readBatteryVoltage()//returns the battery voltage, known good
{
  float voltage=M2IO.Supply_Volts();
  voltage=voltage/1000;
  voltage=.1795*voltage*voltage-2.2321*voltage+14.596;//calibration curve determined with DSO, assumed good

  //additional correction for M2 V4
  voltage=-.0168*voltage*voltage+1.003*voltage+1.3199;//calibration curve determined with DMM, assumed good (M2 V4 only!)

  return voltage;
}

void logDataPoint()//writes current time and voltage to SD card as csv file "data.txt", known good
{
  String str=String(millis())+",";
  char write_buffer[sizeof(str)];
  str.toCharArray(write_buffer, sizeof(str));
  FS.Open("0:", "data.txt", true);
  FS.GoToEnd();
  FS.Write(write_buffer);
  FS.Close();

  str=String(readBatteryVoltage())+"\r\n";
  write_buffer[sizeof(str)];
  str.toCharArray(write_buffer, sizeof(str));
  FS.Open("0:", "data.txt", true);
  FS.GoToEnd();
  FS.Write(write_buffer);
  FS.Close();
}

void setup()
{
  pinMode(Button1, INPUT_PULLUP);//initialize button 1 for input
  
  for(int i=14; i<=18; i++)//initialize LEDs, 14 is red 18 is green
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  }

  SD.Init();
  FS.Init();

  FS.CreateNew("0:","data.txt");//create new file called "data"
  FS.CreateNew("0:","stats.txt");

  M2IO.Init_12VIO();
}

void loop()
{
  float pointsInit=0;
  float sumInit=0;
  float avgInit=0;

  float pointsFin=0;
  float sumFin=0;
  float avgFin=0;
  
  float minDuringStart=1000;

  digitalWrite(14, LOW);//turn red LED on

  while(digitalRead(Button1)==HIGH)//read battery voltage before button is pressed
  {
    sumInit+=readBatteryVoltage();
    pointsInit++;
    logDataPoint();
    delay(50);
  }

  avgInit=sumInit/pointsInit;//compute average voltage prior to engine start

  digitalWrite(14, HIGH);//turn off the red LED and wait for the user to stop pressing the button
  while(digitalRead(Button1)==LOW)
  {
    logDataPoint();//continue recording data while button is being held down, not strictly necessary but maintains graph continuity
    delay(25);
  }
  
  for(int i=14; i<=18; i++)//button pressed, turn on all the LEDs
  {
    digitalWrite(i, LOW);
  }
  
  while(digitalRead(Button1)==HIGH)//watch for minimum voltage while engine is starting
  {
    temp=readBatteryVoltage();

    if(temp<minDuringStart)//update the minimum voltage
    {
      minDuringStart=temp;
    }
    logDataPoint();
  }

  unsigned long startTime=millis();//record the current time
  unsigned long currentTime=startTime;
  unsigned long deltaTime=currentTime-startTime;

  while(deltaTime<=5000)//record idle battery voltage for 5 seconds, turn off LEDs to indicate when time has elapsed
  {
    sumFin+=readBatteryVoltage();
    pointsFin++;

    currentTime=millis();
    deltaTime=currentTime-startTime;

    int ledCount=0;

    ledCount=deltaTime/1000;

    for(int i=14; i<14+ledCount; i++)
    {
      digitalWrite(i, HIGH);
    }

    logDataPoint();
    
    delay(50);
  }

  avgFin=sumFin/pointsFin;//compute average voltage after engine start

  String str="AvgI: ";//write the resulting statistics to the "stats.txt" file on the SD card
  char write_buffer[sizeof(str)];
  str.toCharArray(write_buffer, sizeof(str));
  FS.Open("0:", "stats.txt", true);
  FS.GoToEnd();
  FS.Write(write_buffer);
  FS.Close();

  str=String(avgInit)+"\r\n";
  write_buffer[sizeof(str)];
  str.toCharArray(write_buffer, sizeof(str));
  FS.Open("0:", "stats.txt", true);
  FS.GoToEnd();
  FS.Write(write_buffer);
  FS.Close();

  str="Min: ";
  write_buffer[sizeof(str)];
  str.toCharArray(write_buffer, sizeof(str));
  FS.Open("0:", "stats.txt", true);
  FS.GoToEnd();
  FS.Write(write_buffer);
  FS.Close();

  str=String(minDuringStart)+"\r\n";
  write_buffer[sizeof(str)];
  str.toCharArray(write_buffer, sizeof(str));
  FS.Open("0:", "stats.txt", true);
  FS.GoToEnd();
  FS.Write(write_buffer);
  FS.Close();

  str="AvgF: ";
  write_buffer[sizeof(str)];
  str.toCharArray(write_buffer, sizeof(str));
  FS.Open("0:", "stats.txt", true);
  FS.GoToEnd();
  FS.Write(write_buffer);
  FS.Close();

  str=String(avgFin)+"\r\n";
  write_buffer[sizeof(str)];
  str.toCharArray(write_buffer, sizeof(str));
  FS.Open("0:", "stats.txt", true);
  FS.GoToEnd();
  FS.Write(write_buffer);
  FS.Close();

  for(int i=14; i<=18; i++)//confirm all LEDs are off
  {
    digitalWrite(i, HIGH);
  }

  while(1)
  {
    bool restartTest=true;
    for(int i=0; i<300; i++)//escape while loop if button is held down for 3 seconds
    {
      if(digitalRead(Button1)==HIGH)
      {
        restartTest=false;
        break;
      }
      delay(10);
    }
    if(restartTest)
      break;
  }

  for(int i=0; i<10; i++)//flash red LED for 10 seconds and then restart test, appending new results at end of respective files
  {
    digitalWrite(14, LOW);
    delay(500);
    digitalWrite(14, HIGH);
    delay(500);
  }
}
