#include <Arduino.h>
#include <WiFi.h>
#include "firebase.h"
#include "main.h"

FirebaseData firebaseData1;
FirebaseData firebaseData2;

FirebaseJson jsonbkp;
firebase_st firebaseStruct;

#define FIREBASE_HOST "FIREBASE_HOST" 
#define FIREBASE_AUTH "FIREBASE_AUTH"

unsigned long sendDataPrevMillis = 0;
bool firebaseConnected = false;

String devMacId;
String path = "";
bool updateJson = false;
bool errorToSend = false;
unsigned long contTempCloud = 0;


////******************************************************************************
////  Process stream callback
////------------------------------------------------------------------------------
void streamCallback(StreamData data)
{
    Serial.print("free heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("Stream Data1 available...");
    Serial.println("STREAM PATH: " + data.streamPath());
    Serial.println("EVENT PATH: " + data.dataPath());
    Serial.println("DATA TYPE: " + data.dataType());
    Serial.println("EVENT TYPE: " + data.eventType());

    Serial.print("free heap: ");
    Serial.println(ESP.getFreeHeap());
}

////******************************************************************************
////  Process stream callback timeout
////------------------------------------------------------------------------------
void streamTimeoutCallback(bool timeout)
{
    if (timeout)
    {
        Serial.println("Stream timeout, resume streaming...");
    }
}


////******************************************************************************
////  Init firebase
////------------------------------------------------------------------------------
void initFirebase() 
{
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    firebaseConnected = true; 
    Firebase.reconnectWiFi(true);
    path = "/" + devMacId + "/config";
    Serial.print("path: ");
    Serial.println(path);

    initData();

    if (!Firebase.beginStream(firebaseData1, path))
    {   
        Serial.println("------------------------------------");
        Serial.println("Can't begin stream connection...");
        Serial.println("REASON: " + firebaseData1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
    }

    Firebase.setStreamCallback(firebaseData1, streamCallback, streamTimeoutCallback);    
}

////******************************************************************************
////  Send data at init
////------------------------------------------------------------------------------
void initData()
{
  FirebaseJson json;
  
  json.set("config/time_fan", (unsigned short)10);
  json.set("config/time_pump", (unsigned short)10);
  json.set("config/time_flame", (double)2);
  json.set("config/attempts", (unsigned short)4);
  json.set("config/setpoint", (double)25.0);
  json.set("temperature", (double)tempCloud);
  json.set("status_system", false);
  json.set("status_heater", false);   
  json.set("config/getStatus", false);  
  json.set("config/onoff", false);
  json.set("config/testValve", 0);
  json.set("config/testPump", 0);
  json.set("config/testSparker", 0);
  json.set("config/testFan", 0);

  if (Firebase.setJSON(firebaseData2, "/"+devMacId, json))
  {
      Serial.println("Iniciando o envio de dados...");
  }
  else
  {
      Serial.println("Falha no envio inicial de dados");
      Serial.println("REASON: " + firebaseData2.errorReason());
  }
}

////******************************************************************************
////  Update data on firebase
////------------------------------------------------------------------------------
void processFirebase() 
{
  FirebaseJson jsonUpdt;
  if (millis() - sendDataPrevMillis > 1000)
  {
    sendDataPrevMillis = millis();
    if(errorToSend)
      jsonUpdt = jsonbkp;

    if(firebaseStruct.updateTemp)
    {
        firebaseStruct.updateTemp = false;
        jsonUpdt.add("temperature", tempCloud);
        contTempCloud = 0;
        updateJson = true;
    }

    if(updateJson)
    {
      updateJson = false;
      Serial.print("free heap: ");
      Serial.println(ESP.getFreeHeap());
      if (Firebase.updateNodeSilent(firebaseData2, "/"+devMacId, jsonUpdt)) //decidir se utiliza setJSON ou updateNode
      {
          Serial.println("Update node");
          if(errorToSend)
          {
            errorToSend = false;
            jsonbkp.clear();
            Serial.println("send error");
          }
      }
      else
      {
          Serial.println("Falha - Update node");
          Serial.println("REASON: " + firebaseData2.errorReason());
          errorToSend = true;
          jsonbkp = jsonUpdt;
      }
      Serial.print("free heap: ");
      Serial.println(ESP.getFreeHeap());

      if (Firebase.setTimestamp(firebaseData2, "/" + devMacId + "/timestamp"))
      {
        Serial.println("Set timestamp");
      }
      else
      {
        Serial.println("Falha - Set timestamp");
        Serial.println("REASON: " + firebaseData2.errorReason());
      }
      Serial.print("free heap: ");
      Serial.println(ESP.getFreeHeap());
    }
  }
}

////******************************************************************************
////  printResult
////------------------------------------------------------------------------------
void printResult(FirebaseData &data)
{

  if (data.dataType() == "int")
    Serial.println(data.intData());
  else if (data.dataType() == "float")
    Serial.println(data.floatData(), 5);
  else if (data.dataType() == "double")
    printf("%.9lf\n", data.doubleData());
  else if (data.dataType() == "boolean")
    Serial.println(data.boolData() == 1 ? "true" : "false");
  else if (data.dataType() == "string")
    Serial.println(data.stringData());
  else if (data.dataType() == "json")
  {
    Serial.println();
    FirebaseJson &json = data.jsonObject();
    //Print all object data
    //Serial.println("Pretty printed JSON data:");
    String jsonStr;
    json.toString(jsonStr, true);
    // Serial.println(jsonStr);
    // Serial.println();
    Serial.println("Iterate JSON data:");
    Serial.println();
    size_t len = json.iteratorBegin();
    String key, value = "";
    int type = 0;
    for (size_t i = 0; i < len; i++)
    {
      json.iteratorGet(i, type, key, value);
      Serial.print(i);
      Serial.print(", ");
      Serial.print("Type: ");
      Serial.print(type == FirebaseJson::JSON_OBJECT ? "object" : "array");
      if (type == FirebaseJson::JSON_OBJECT)
      {
        Serial.print(", Key: ");
        Serial.print(key);
      }
      Serial.print(", Value: ");
      Serial.println(value);
    }
    json.iteratorEnd();
  }
  else if (data.dataType() == "array")
  {
    Serial.println();
    //get array data from FirebaseData using FirebaseJsonArray object
    FirebaseJsonArray &arr = data.jsonArray();
    //Print all array values
    Serial.println("Pretty printed Array:");
    String arrStr;
    arr.toString(arrStr, true);
    Serial.println(arrStr);
    Serial.println();
    Serial.println("Iterate array values:");
    Serial.println();
    for (size_t i = 0; i < arr.size(); i++)
    {
      Serial.print(i);
      Serial.print(", Value: ");

      FirebaseJsonData &jsonData = data.jsonData();
      //Get the result data from FirebaseJsonArray object
      arr.get(jsonData, i);
      if (jsonData.typeNum == FirebaseJson::JSON_BOOL)
        Serial.println(jsonData.boolValue ? "true" : "false");
      else if (jsonData.typeNum == FirebaseJson::JSON_INT)
        Serial.println(jsonData.intValue);
      else if (jsonData.typeNum == FirebaseJson::JSON_DOUBLE)
        printf("%.9lf\n", jsonData.doubleValue);
      else if (jsonData.typeNum == FirebaseJson::JSON_STRING ||
               jsonData.typeNum == FirebaseJson::JSON_NULL ||
               jsonData.typeNum == FirebaseJson::JSON_OBJECT ||
               jsonData.typeNum == FirebaseJson::JSON_ARRAY)
        Serial.println(jsonData.stringValue);
    }
  }
}
