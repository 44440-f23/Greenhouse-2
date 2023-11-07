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
#include "OneWire.h"

int DS18S20_Pin = 25;

OneWire ds(DS18S20_Pin);

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
float getTemp();
float SoilTemp;

String gatherData();


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

//Parsing tools
uint32_t parseSimpleJson(const char* jsonString);
const size_t bufferSize = 1024;

//Mesh Vars
void sendMessage(); 
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback(); 
painlessMesh mesh;
SimpleList<uint32_t> nodes;
Scheduler userScheduler;
Task taskSendMessage( TASK_SECOND * MSG_DELAY_SEC, TASK_FOREVER, &sendMessage );

void setup() 
{
  //Setup Serial Console
  Serial.begin(9600);

  Wire.begin();

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
}

String gatherData()
{
  //DFRobot SEN0546
  readReg(0x00, buf, 4);
  data = buf[0] << 8 | buf[1];
  data1 = buf[2] << 8 | buf[3];
  
  //Celcius
  temperature = ((float)data * 165 / 65535.0) - 40.0;
  humidity = ((float)data1 / 65535.0) * 100;

  //DFRobot DFR0026
  int light = analogRead(A0);

  //DFRobot DS18B20 - Digital Temp Sensor
  float SoilTemp = getTemp();

  String json = "{\"id\":2,\"temp\":" + String(temperature) + ",\"humidity\":" + String(humidity) + ",\"soilT\":" + String(SoilTemp) + ",\"soilM\":0,\"lightS\":" + String(light) + "}";
  return json;

}

void sendMessage() 
{
  String msg = gatherData();

  if(basestationID == 0)
  {
    Serial.println("BS has no id");
  }
  else
  {
    mesh.sendSingle(basestationID, msg);
    Serial.println("Sent: " + msg);
  }

  taskSendMessage.setInterval(5000);
}



void receivedCallback(uint32_t from, String & msg) 
{
  Serial.printf("Received Callback: from %u msg=%s\n", from, msg.c_str());

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
  Serial.println();
  Serial.printf("New Connection Callback, nodeId = %u\n", nodeId);
  Serial.println();
}

void changedConnectionCallback() 
{
  Serial.println();
  Serial.println("Changed connections");
  Serial.println();
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

float getTemp()
{
  //sensors.requestTemperatures();
  //return sensors.getTempCByIndex(0);

byte data[12];
byte addr[8];

if (!ds.search(addr))
{
  ds.reset_search();
  return -1000;
}

if (OneWire::crc8(addr, 7) != addr[7])
{
  Serial.println("CRC is not valid!");
  return -1000;
}

if (addr[0] != 0x10 && addr[0] != 0x28)
{
  Serial.print("Device is not recognized");
  return -1000;
}

ds.reset();
ds.select(addr);
ds.write(0x44, 1);

byte present = ds.reset();
ds.select(addr);
ds.write(0xBE);

for (int i = 0; i < 9; i++)
{
  data[i] = ds.read();
}

ds.reset_search();

byte MSB = data[1];
byte LSB = data[0];

float tempRead = ((MSB << 8) | LSB);
float TemperatureSum = tempRead / 16;

return TemperatureSum;

}
