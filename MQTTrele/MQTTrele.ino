#include <FS.h> 
#include <ArduinoJson.h>
#include <PubSubClient.h>                
#include <ESP8266WiFi.h>   
#include <ESP8266WebServer.h>     
#include <DNSServer.h>
#include <WiFiManager.h>        
#include <EEPROM.h>

#define DEBUG  


//Aqui nós colocamos os dados da nossa instancia mqtt 


#define servidor_mqtt             "soldier.cloudmqtt.com"  
#define servidor_mqtt_porta       "17915"  
#define servidor_mqtt_usuario     "kfuzhbia"  
#define servidor_mqtt_senha       "GRns79hpLpYk"  
#define mqtt_topico_sub           "esp8266/pincmd"    


#define pino                      2                   
#define alocada           4                   

//instancia wifi client

WiFiClient esp01cli;                                 
PubSubClient client(esp01cli);                       

bool verifSalvar  =             false;              //para salvar os dados

//Função para imprimir na porta serial
void printSerial(bool linha, String mensagem){
  #ifdef DEBUG
    if(linha){
      Serial.println(mensagem);
    }else{
      Serial.print(mensagem);
    }
  #endif
}

//Função de retorno para notificar sobre a necessidade de salvar as configurações
void verifSalvarCallback() {
  printSerial(true, "Salvar configurações");
  verifSalvar = true;
}

//Caso necessario, reconecta ao mqtt
void reconect() {
  
  while (!client.connected()) {
    printSerial(false, "Reconectando");
    
    bool conectado = strlen(servidor_mqtt_usuario) > 0 ?
                     client.connect("ESP8266Client", servidor_mqtt_usuario, servidor_mqtt_senha) :
                     client.connect("ESP8266Client");

    if(conectado) {
      printSerial(true, "Conectado!");
      
      //Subscreve para monitorar os comandos recebidos
      client.subscribe(mqtt_topico_sub, 1); 
    } else {
      printSerial(false, "Falha ");
      printSerial(true, " tentando reconectar");
      //tempo para reconectar
      delay(5000);
    }
  }
}


//Gravar status do pino na EEPROM
void gravarStatusPino(uint8_t statusPino){
  EEPROM.begin(alocada);
  EEPROM.write(0, statusPino);
  EEPROM.end();
}

//Função que será chamada ao receber mensagem do servidor MQTT
void retorno(char* topico, byte* mensagem, unsigned int tamanho) {
  //Convertendo a mensagem recebida para string
  mensagem[tamanho] = '\0';
  String strMensagem = String((char*)mensagem);
  strMensagem.toLowerCase();
  
  printSerial(false, "Mensagem recebida, Topico: ");
  printSerial(false, topico);
  printSerial(false, ". Tamanho: ");
  printSerial(false, String(tamanho).c_str());
  printSerial(false, ". Mensagem: ");
  printSerial(true, strMensagem);

  //Executando o comando recebido pelo broker MQTT, (Liga ou Desliga)
  printSerial(false, "Status do pino antes de processar o comando: ");
  printSerial(true, String(digitalRead(pino)).c_str());
  
  if(strMensagem == "liga"){
    printSerial(true, "Ligando");
    digitalWrite(pino, HIGH);
    gravarStatusPino(HIGH);
  }else if(strMensagem == "desliga"){
    printSerial(true, "Desligando");
    digitalWrite(pino, LOW);
    gravarStatusPino(LOW);
  }else{
    printSerial(true, "Trocando o estado do pino...");
    digitalWrite(pino, !digitalRead(pino));
    gravarStatusPino(digitalRead(pino));
  }
  
  printSerial(false, "Status do pino depois de processar o comando: ");
  printSerial(true, String(digitalRead(pino)).c_str());
}

//função setup é executada apenas quando o dispositivo é ligado
void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
  #endif
  printSerial(true, "...");

  //declarando o pino como saida
  pinMode(pino, OUTPUT);

  //Iniciando o SPIFSS (SPI Flash File System)
  printSerial(true, "Iniciando o SPIFSS (SPI Flash File System)");
  if (SPIFFS.begin()) {
    printSerial(true, "Sistema de arquivos SPIFSS montado!");
    if (SPIFFS.exists("/config.json")) {
      //Arquivo de configuração existe e será lido.
      printSerial(true, "Abrindo o arquivo de configuracao...");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        printSerial(true, "Arquivo de configuracao aberto.");
        size_t size = configFile.size();
        
        //Alocando um buffer para armazenar o conteúdo do arquivo.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          
            //Copiando as variáveis salvas previamente no aquivo json para a memória do ESP.
            printSerial(true, "arquivo json analisado.");
            strcpy(servidor_mqtt, json["servidor_mqtt"]);
            strcpy(servidor_mqtt_porta, json["servidor_mqtt_porta"]);
            strcpy(servidor_mqtt_usuario, json["servidor_mqtt_usuario"]);
            strcpy(servidor_mqtt_senha, json["servidor_mqtt_senha"]);
            strcpy(mqtt_topico_sub, json["mqtt_topico_sub"]);

        } else {
          printSerial(true, "Falha ao ler as configuracoes do arquivo json.");
        }
      }
    }
  } else {
    printSerial(true, "Falha ao montar o sistema de arquivos SPIFSS.");
  }

  //Parâmetros extras para configuração
  //Depois de conectar, parameter.getValue() vai pegar o valor configurado.
  //Os campos do WiFiManagerParameter são: id do parâmetro, nome, valor padrão, comprimento
  WiFiManagerParameter custom_mqtt_server("server", "Servidor MQTT", servidor_mqtt, 40);
  WiFiManagerParameter custom_mqtt_port("port", "Porta", servidor_mqtt_porta, 6);
  WiFiManagerParameter custom_mqtt_user("user", "Usuario", servidor_mqtt_usuario, 20);
  WiFiManagerParameter custom_mqtt_pass("pass", "Senha", servidor_mqtt_senha, 20);
  WiFiManagerParameter custom_mqtt_topic_sub("topic_sub", "Topico para subscrever", mqtt_topico_sub, 30);

  //Inicialização do WiFiManager
  WiFiManager wifiManager;

  //Definindo a função que informará a necessidade de salvar as configurações
  wifiManager.setSaveConfigCallback(verifSalvarCallback);
  
  //Adicionando os parâmetros para conectar ao servidor MQTT
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_mqtt_topic_sub);

  //Busca o ID e senha da rede wifi e tenta conectar.
  //Caso não consiga conectar ou não exista ID e senha,
  //cria um access point com o nome "AutoConnectAP" e a senha "senha123"
  //E entra em loop aguardando a configuração de uma rede WiFi válida.
  if (!wifiManager.autoConnect("AutoConnectAP", "senha123")) {
    printSerial(true, "Falha ao conectar. Excedeu o tempo limite para conexao.");
    delay(3000);
    //Reinicia o ESP e tenta novamente
    ESP.reset();
    delay(5000);
  }

  //Conectou com sucesso no wifi
  printSerial(true, "Conectado!! :)");

  //Lendo os parâmetros atualizados
  strcpy(servidor_mqtt, custom_mqtt_server.getValue());
  strcpy(servidor_mqtt_porta, custom_mqtt_port.getValue());
  strcpy(servidor_mqtt_usuario, custom_mqtt_user.getValue());
  strcpy(servidor_mqtt_senha, custom_mqtt_pass.getValue());
  strcpy(mqtt_topico_sub, custom_mqtt_topic_sub.getValue());

  //Salvando os parâmetros informados na tela web do WiFiManager
  if (verifSalvar) {
    printSerial(true, "Salvando as configuracoes");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["servidor_mqtt"] = servidor_mqtt;
    json["servidor_mqtt_porta"] = servidor_mqtt_porta;
    json["servidor_mqtt_usuario"] = servidor_mqtt_usuario;
    json["servidor_mqtt_senha"] = servidor_mqtt_senha;
    json["mqtt_topico_sub"] = mqtt_topico_sub;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      printSerial(true, "Houve uma falha ao abrir o arquivo de configuracao para incluir/alterar as configuracoes.");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }

  printSerial(false, "IP: ");
  printSerial(true, WiFi.localIP().toString());

  //Informando ao client do PubSub a url do servidor e a porta.
  int portaInt = atoi(servidor_mqtt_porta);
  client.setServer(servidor_mqtt, portaInt);
  client.setCallback(retorno);
  
}

//Função de repetição
void loop() {
  if (!client.connected()) {
    reconect();
  }
  client.loop();
}
