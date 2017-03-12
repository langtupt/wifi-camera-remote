/////////////////////////////////////////////////////////////////////
//This code is used to control a Canon DSLR camera from the Blynk app via wifi.
//The shutter is connected to digital output 25.
//The focus is connected to digital output 27.
//Virtual pin V0 is used to control the start and stop of the record (timelapse)
//Virtual pin V1 is used as a feedback to the phone. It is high when digital output 25 is high
//Virtual pin V2 is used as a feedback to the phone. It is high when digital output 27 is high
​//Virtual pin V3 is used to send texts to the terminal on the phone
//Virtual pin V4 contains the time (in ms) during which the shutter contact has to be closed to trigger a picture
//Virtual pin V5 contains the time (in s) between each picture
//Virtual pin V6 contains the duration (in s) of the record
///////////////////////////////////////////////////////////////////////

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <SPI.h>
#include <WiFi.h>
#include <BlynkSimpleWifi.h>
#include <SimpleTimer.h>


//variables
WidgetTerminal terminal(V3);

char auth[] = "authToken";//Enter auth token here


// WiFi credentials:
char* ssid[] = {"ssid1", "ssid2"};//Enter ssid number 1, 2, etc...
char* pass[] = {"pass1", "pass2"};//Enter pass number 1, 2, etc...
int nbKnownAP = 2;//number of elements in ssid[] and pass[]
char ssidChosen[20] = "\0";//increase the number of characters if longest ssid is longer than 20 characters
char passChosen[20] = "\0";//increase the number of characters if longest pass is longer than 20 characters

IPAddress ip (45,55,96,146); //ip address of local server

int shutterT = 1000;//time the shutter contact has to be closed to take a picture (in ms)
int waitingT = 20;//time between each picture (in s)
int duration = 300;//duration of the record (in s). After duration, pin V0 goes to 0 (stop)
int startTime = 0;//time in sec when timer started
int shutterTimerId = 0;//timer ID

bool lastV0 = false;//checks if V0 changes
bool lastV2 = false;//checks if V2 changes
bool lastV1 = false;//checks if V1 changes
bool startRecord = false;//true when the camera starts recording a timelapse

SimpleTimer timer;//Timer object

WidgetLED led2(2); //virtual pin 2
WidgetLED led1(1); //virtual pin 1

//V0 is true when the record starts and false when the record stops
BLYNK_WRITE(V0)
{
  if(param.asInt()>0)
    startRecord = true;
  else
    startRecord = false;
}

//the time the shutter should be opened is sent from the app
BLYNK_WRITE(V4)
{
  shutterT = param.asInt();
}

//the waiting time between shutters is sent from the app
BLYNK_WRITE(V5)
{
  waitingT = param.asInt();
  if(timer.isEnabled(shutterTimerId))
  {
      timer.deleteTimer(shutterTimerId);
      shutterTimerId = timer.setInterval(1000*waitingT, openShutter);
  }
}

//the duration of the recording is sent from the app
BLYNK_WRITE(V6)
{
  duration = param.asInt();
}

//closes the shutter (opens the contact)
void closeShutter()
{
  digitalWrite(25, LOW);
  terminal.println(" closed");
  terminal.flush();
}

//opens the shutter (closes the contact)
void openShutter()
{
  digitalWrite(25, HIGH);
  timer.setTimeout(shutterT, closeShutter);
  terminal.print(millis()/1000-startTime);
  terminal.flush();
}

void setup()
{
  int numSsid = 0;//Number of ssid found
  int i = 0;
  int j = 0;
  
  Serial.begin(9600);

  Serial.println("** Scan Networks **");
  numSsid = WiFi.scanNetworks();
  if (numSsid == -1)//In case there is no wifi connection
  {
    Serial.println("Couldn't get a wifi connection");
    while (true)
    {delay(1000);}
  }

  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (i = 0; i < numSsid; i++)
  {
    Serial.print(i);
    Serial.print(") ");
    Serial.print(WiFi.SSID(i));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(i));
    Serial.println(" dBm");
    for(j=0; j<nbKnownAP; j++)
    {  
      if(strcmp(WiFi.SSID(i), ssid[j])==0)//in case 1 of the networks found is a known network
      {
        Serial.print(ssid[j]);
        Serial.println(" is a known network!");
        strcpy(ssidChosen, ssid[j]);
        strcpy(passChosen, pass[j]);
      }
    }
  }
  
  if(strlen(ssidChosen) == 0)//in case no known network was found
  {
    Serial.println("None of the known networks has been found");
    while (true)
    {delay(1000);}
  }

  Blynk.begin(auth, ssidChosen, passChosen, ip, 8442);

  while(Blynk.connect() == false);

  pinMode(27, OUTPUT);
  pinMode(25, OUTPUT);

  digitalWrite(27, LOW);
  digitalWrite(25, LOW);

  Blynk.syncAll();
}


void loop()
{
  Blynk.run();
  timer.run();


  if(digitalRead(27) > 0 && !lastV2)
  {
    led2.on();
    lastV2 = true;
  }
  else if(digitalRead(27) == 0 && lastV2)
  {
    led2.off();
    lastV2 = false;
  }

  if(digitalRead(25) > 0 && !lastV1)
  {
    led1.on();
    lastV1 = true;
  }
  else if(digitalRead(25) == 0 && lastV1)
  {
    led1.off();
    lastV1 = false;
  }

  if(startRecord)
  {
    if(!lastV0)//when timer starts
    {
      lastV0 = true;
      startTime = millis()/1000;
      shutterTimerId = timer.setInterval(1000*waitingT, openShutter);
    }
    if(millis()/1000-startTime > duration)//after duration
    {
      Blynk.virtualWrite(V0, 0);
      startRecord = false;
      lastV0 = false;
      timer.deleteTimer(shutterTimerId);
    }
  }
  else
  {
    if(lastV0)//when timer stops
    {
      lastV0 = false;
      timer.deleteTimer(shutterTimerId);
    }
  }
}

