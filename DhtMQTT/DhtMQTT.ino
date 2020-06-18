#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
 
//a cada 20 segundos sera enviada informação para o MQTT
#define intervalo       20000
 
#define DHTPIN 2      
#define DHTTYPE DHT11
 
DHT dht(DHTPIN, DHTTYPE);
 
#define DEBUG

 
//dados do wifi
const char* ssid = "entra que e virus";             
const char* password =  "pa$sw0rd"; 
 
//Dados do cloudmqtt
const char* mqttServer = "soldier.cloudmqtt.com";   
const char* mqttUser = "kfuzhbia";              
const char* mqttPassword = "GRns79hpLpYk";      
const int mqttPort = 17915;                     
const char* mqttTopicSub ="temp";            
 
int ultimoEnvioMQTT = 0;
 
WiFiClient espClient;
PubSubClient client(espClient);
 
void setup() {
 
  
 
  dht.begin();
 
}
 

 
//função pra reconectar ao servido MQTT
void reconect() {
  //Enquanto estiver desconectado
  while (!client.connected()) {
    #ifdef DEBUG
    Serial.print("Tentando conectar ao servidor MQTT");
    #endif
     
    bool conectado = strlen(mqttUser) > 0 ?
                     client.connect("ESP8266Client", mqttUser, mqttPassword) :
                     client.connect("ESP8266Client");
 
    if(conectado) {
      #ifdef DEBUG
      Serial.println("Conectado!");
      #endif
      //subscreve no tópico
      client.subscribe(mqttTopicSub, 1); //nivel de qualidade: QoS 1
    } else {
      #ifdef DEBUG
      Serial.println("Falha durante a conexão.Code: ");
      Serial.println( String(client.state()).c_str());
      Serial.println("Tentando novamente em 10 s");
      #endif
      //Aguarda 10 segundos 
      delay(10000);
    }
  }
}
 
void loop() {
  if (!client.connected()) {
    reconect();
  }
 
  //envia a cada X segundos
  if ((millis() - ultimoEnvioMQTT) > intervalo)
  {
      enviaDHT();
      ultimoEnvioMQTT = millis();
  }
  
  client.loop();
}
 
//função para leitura do DHT11
void enviaDHT(){
 
  char MsgUmidadeMQTT[10];
  char MsgTemperaturaMQTT[10];
  
  float umidade = dht.readHumidity();
  float temperatura = dht.readTemperature();


    sprintf(MsgUmidadeMQTT,"%f",umidade);
    client.publish("casa/umidade", MsgUmidadeMQTT);
    sprintf(MsgTemperaturaMQTT,"%f",temperatura);
    client.publish("casa/temperatura", MsgTemperaturaMQTT);
  }
