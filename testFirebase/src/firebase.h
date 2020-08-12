#include <FirebaseESP32.h>
#include "FirebaseJson.h"

extern bool firebaseConnected;
extern String devMacId;

typedef struct
{
    bool updateFlame;
    bool updateFan;
    bool updatePump;
    bool updateAttempts;
    bool updateAlarmLevel;
    bool updateAlarmFlame;
    bool updateAlarmFlood;
    bool updateAlarmPressure;
    bool updateAlarmFan;
    bool updateAlarmGas;
    bool updateAlarmTemp;
    bool updateAlarmCent;
    bool updateAlarmPump;
    bool updateErrorStart;
    bool updateSetpoint;
    bool updateTemp;
    bool updateStatusSystem;
    bool updateStatusHeater;      
    bool updatePumpOn;
}firebase_st;

extern firebase_st firebaseStruct;

extern unsigned long contTempCloud;

void initFirebase();
void processFirebase();
void initData();
void printResult(FirebaseData &data);
void processUpdate(String Key, String value);

