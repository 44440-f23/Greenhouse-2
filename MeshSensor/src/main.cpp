//************************************************************
// this is a simple example that uses the easyMesh library
//
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 3. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 4. prints anything it receives to Serial.print
//
//
//************************************************************
#include <Arduino.h>
#include <painlessMesh.h>
#include "Wire.h"

// Temperature and Humidity Sensor
//DFRobot SEN0546
#define address 0x40
char dtaUart[15];
char dtaLen = 0;
uint8_t Data[100] = {0};
uint8_t buff[100] = {0};
uint8_t readReg(uint8_t reg, const void* pBuf, size_t size);

uint8_t buf[4] = {0};
uint16_t data;
uint16_t data1;
float temperature;
float humidity;
float ftemp;

/*
// Analog Soil Moisture Sensor
// DFRobot SEN0308
const int AirValue = 0;
const int WaterValue = 0;

int intervals = (AirValue - WaterValue) / 3;
int soilMoistureValue = 0;
*/

// some gpio pin that is connected to an LED...
// on my rig, this is 5, change to the right number of your LED.
#ifdef LED_BUILTIN
#define LED LED_BUILTIN
#else
#define LED 2
#endif

#define BLINK_PERIOD 3000 // milliseconds until cycle repeat
#define BLINK_DURATION 100  // milliseconds LED is on for

#define MESH_SSID "WhateverYouLike"
#define MESH_PASSWORD "SomethingSneaky"
#define MESH_PORT 5555

#define MSG_DELAY_SEC 1

//TODO - parse this in the JSON
int basestationID = 0;

const size_t bufferSize = 1024;

// Prototypes
void sendMessage(); 
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback(); 
void sendMessage();
uint32_t parseSimpleJson(const char* jsonString);

void gatherData();

//TODO - understand this code
Task taskSendMessage( TASK_SECOND * MSG_DELAY_SEC, TASK_FOREVER, &sendMessage );

//This is to schedule our personal jobs
Scheduler userScheduler;

//The actual mesh
painlessMesh mesh;

//TODO - write a comment here
bool calc_delay = false;
SimpleList<uint32_t> nodes;

// Task to blink the number of nodes using the default task params
Task blinkNoNodes;
bool onFlag = false;

void setup() 
{
  //Setup Serial Console
  Serial.begin(9600);

  Wire.begin();
  
  //Setup Pinmode(s)
  pinMode(LED, OUTPUT);

  //Set before init() so that error messages work
  //I dont really understand the ints here
  mesh.setDebugMsgTypes(ERROR | DEBUG);  

  //Initialize mesh; init(Network, Pass, a scheduler for tasks, the port number)
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);

  //Set up listeners for the mesh
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  //Add a task our schedule
  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();

  //TODO - understand this code
  randomSeed(analogRead(A0));
}


void loop() 
{
  //Various maintenence tasks
  mesh.update();

  delay(500);

/*
  //DFRobot SEN0308
  //Serial.println(analogRead(A0));
  //delay(100);
  soilMoistureValue = analogRead(A0);
  
  Serial.print("Soil Moisture:");
  Serial.println(soilMoistureValue);

  if(soilMoistureValue > WaterValue && soilMoistureValue < (WaterValue + intervals))
  {
    Serial.println("Very Wet");
  }
  else if(soilMoistureValue > (WaterValue + intervals) && soilMoistureValue < (AirValue - intervals))
  {
    Serial.println("Wet");
  }
  else if(soilMoistureValue < AirValue && soilMoistureValue > (AirValue - intervals))
  {
    Serial.println("Dry");
  }
  delay(100);
*/

}

void gatherData()
{
  //DFRobot SEN0546
  readReg(0x00, buf, 4);
  data = buf[0] << 8 | buf[1];
  data1 = buf[2] << 8 | buf[3];
  
  //Celcius
  temperature = ((float)data * 165 / 65535.0) - 40.0;
  humidity = ((float)data1 / 65535.0) * 100;

  //DFRobot DFR0026
  int val;
  val = analogRead(0);
  Serial.print("Light:");
  Serial.println(val, DEC);
  delay(100);
}

void sendMessage() 
{
  DynamicJsonDocument doc(1024);
  doc["id"] = "Greenhouse 2";
  doc["temp"];

  String msg = "{id:Greenhouse 2, temp:}";

  if(basestationID == 0)
  {
    Serial.println("BS has no id");
  }
  else
  {
    mesh.sendSingle(basestationID, msg);
    Serial.println("Sent: " + msg);
  }
  
  //Scheduled every time between MSG_DELAY_SEC to 5 seconds
  //taskSendMessage.setInterval( random(TASK_SECOND * MSG_DELAY_SEC, TASK_SECOND * 5));
  taskSendMessage.setInterval(5000);
}



void receivedCallback(uint32_t from, String & msg) 
{
  Serial.printf("Received: from %u msg=%s\n", from, msg.c_str());

  try
  {
    basestationID = parseSimpleJson(msg.c_str());
  }
  catch(const std::exception& e)
  {
    Serial.printf("ERROR in json parse");
  }
}

void newConnectionCallback(uint32_t nodeId)
{
  Serial.println("***********************************************************");
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
  Serial.printf("New Connection, %s\n", mesh.subConnectionJson(true).c_str());
  Serial.println("***********************************************************");
}

void changedConnectionCallback() 
{
  Serial.println("//////////////////////////////");
  Serial.println("Changed connections");
  Serial.println("//////////////////////////////");
  calc_delay = true;
}

uint32_t parseSimpleJson(const char* jsonString)
{
  //create the parsable json object
  StaticJsonDocument<bufferSize> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, jsonString);

  // catch the errors if there are any
  if (error) 
  {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    uint32_t i = 0;
    return i;
  }

  uint32_t baseID = jsonDoc["basestation"];
  return baseID;
}

uint8_t readReg(uint8_t reg, const void* pBuf, size_t size)
{
  if (pBuf == NULL)
  {
    Serial.println("pBuf ERROR!! : null pointer");
  }
  
  uint8_t * _pBuf = (uint8_t *)pBuf;
  Wire.beginTransmission(address);
  Wire.write(&reg, 1);

  if (Wire.endTransmission() != 0)
  {
    return 0;
  }

  delay(20);
  Wire.requestFrom(address, (uint8_t) size);

  for (uint16_t i = 0; i < size; i++)
  {
    _pBuf[i] = Wire.read();
  }

  return size;
}
