#include <Arduino.h>
#include "main.h"
#include "firebase.h"

#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <esp_wifi.h>

////******************************************************************************
////  VARIÁVEIS
////------------------------------------------------------------------------------
unsigned long currTime = 0;
unsigned long lastTime = 0;
bool wifiConnected = false;

void coreTaskZero( void * pvParameters );
void coreTaskOne( void * pvParameters );

float tempCloud = 20.0;
float temperature = 20.0;

////******************************************************************************
////  Setup - Inicialização
////------------------------------------------------------------------------------
void setup() 
{
  Serial.begin(9600);  

  devMacId = "246F28";
  Serial.print("Device MAC:"); Serial.println(devMacId);
  if(devMacId == "")
  {
    Serial.println("erro para obter mac");
    devMacId = "Dev00";
  }
  
  Serial.println("*** Reset ***");
  
  //cria uma tarefa que será executada na função coreTaskZero, com prioridade 1 e execução no núcleo 0
  xTaskCreatePinnedToCore(
                    coreTaskZero,   /* função que implementa a tarefa */
                    "coreTaskZero", /* nome da tarefa */
                    10000,      /* número de palavras a serem alocadas para uso com a pilha da tarefa */
                    NULL,       /* parâmetro de entrada para a tarefa (pode ser NULL) */
                    1,          /* prioridade da tarefa (0 a N) */
                    NULL,       /* referência para a tarefa (pode ser NULL) */
                    0);         /* Núcleo que executará a tarefa */
                    
  delay(500); //tempo para a tarefa iniciar
  
  //cria uma tarefa que será executada na função coreTaskOne, com prioridade 2 e execução no núcleo 1
  xTaskCreatePinnedToCore(
                    coreTaskOne,   /* função que implementa a tarefa */
                    "coreTaskOne", /* nome da tarefa */
                    10000,      /* número de palavras a serem alocadas para uso com a pilha da tarefa */
                    NULL,       /* parâmetro de entrada para a tarefa (pode ser NULL) */
                    2,          /* prioridade da tarefa (0 a N) */
                    NULL,       /* referência para a tarefa (pode ser NULL) */
                    1);         /* Núcleo que executará a tarefa */

    delay(500); //tempo para a tarefa iniciar  
}

////******************************************************************************
////  Loop - vazio
////------------------------------------------------------------------------------
void loop() 
{
}

////******************************************************************************
////  GERENCIAMENTO DA FUNÇÃO PRINCIPAL
////  Ler e controlar temperatura, ligar e desligar aquecedor, display, I2C, timers
////------------------------------------------------------------------------------
void coreTaskZero( void * pvParameters )
{
  while(true)
  { 
      currTime = millis();
      if((currTime - lastTime) > (unsigned long)10000) //esperou o tempo de conversão 
      {		
          tempCloud = temperature + 0.1;
          firebaseStruct.updateTemp = true;
          lastTime = currTime; 
      }
      vTaskDelay(6);      
  } 
}

////******************************************************************************
////  WIFI, WEBSERVER LOCAL E FIREBASE
////------------------------------------------------------------------------------
void coreTaskOne( void * pvParameters )
{
    unsigned long startedAt = millis();

    WiFi.mode(WIFI_AP_STA);
    wifi_config_t current_conf;
    esp_wifi_get_config(WIFI_IF_STA, &current_conf);
    String _ssid = String(reinterpret_cast<const char*>(current_conf.sta.ssid));
    Serial.print("ssid: ");
    Serial.println(_ssid);

    if(_ssid != "")
    {
      String _pass = String(reinterpret_cast<const char*>(current_conf.sta.password));

      WiFi.begin();
      bool tryconnect = true;
      while(millis() - startedAt < 5000 && tryconnect)
      {
        delay(500);
        if (WiFi.status()==WL_CONNECTED) 
        {
          tryconnect=false;
          wifiConnected = true;
          Serial.println("wifi conectado");
        }
      }
    }

    while(true)
    {
        if(wifiConnected && !firebaseConnected)
        {
            initFirebase();
            firebaseConnected = true;
        }
        if(wifiConnected && firebaseConnected)
          processFirebase();
        
        delay(10);       
    } 
}