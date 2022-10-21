/********************************************************
 * Jean Victor Rocha
 * IoT DVT
 *******************************************************/
String FirmwareVer = {"0.2"};

// Bibliotecas ------------------------------------------
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "data\cert.h"//Certificado Github
#include <NTPClient.h> //Hora NTP - https://github.com/arduino-libraries/NTPClient

#define LEAP_YEAR(Y)     ( (Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) ) )
#define URL_fw_Version "https://raw.githubusercontent.com/jeanvictorrocha/IoT_DVT/main/version.txt"
#define URL_fw_Bin     "https://raw.githubusercontent.com/jeanvictorrocha/IoT_DVT/main/build/esp32.esp32.esp32/IoT_DVT.ino.bin"

//Variaveis publicas ------------------------------------------
String DataAtual = "";
String HoraAtual = "";

//Constantes ------------------------------------------
const byte      LED_PIN = 2;
static uint8_t taskCoreZero = 0;
static uint8_t taskCoreOne  = 1;

unsigned long repeatedCallMillis = 0;
unsigned long repeatedCallMillis_2 = 0;

//const char * ssid = "CORP-TLG";
//const char * password = "y.q<5sQWKj#E";
//const char * ssid = "GRU_Office";
//const char * password = "K4ilztqh03@#";
const char * ssid = "J2-VIVO-4G";
const char * password = "3346878710";

WiFiUDP udp;//Socket UDP que a lib utiliza para recuperar dados sobre o horário

NTPClient ntpClient(udp,"1.br.pool.ntp.org",(-3)*3600,60000);//Objeto responsável por recuperar dados sobre horário //-3 = SP-Brasil

char* dayOfWeekNames[] = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sabado"};//Nomes dos dias da semana

//Button Boot
struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  bool pressed;
};
Button button_boot = {
  0,
  0,
  false
};

//############################################################
//#################### IRAM_ATTR #############################
//############################################################
//Função para Checagem de botão pressionado
void IRAM_ATTR isr() {
  button_boot.numberKeyPresses += 1;
  button_boot.pressed = true;
}

//############################################################
//#################### SETUP CORE_1 ##########################
//############################################################
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(button_boot.PIN, INPUT);
  attachInterrupt(button_boot.PIN, isr, RISING);
  
  Serial.begin(115200);

  Serial.print("Versão ativa do firmware: ");
  Serial.println(FirmwareVer);

  Serial.println("Inicializando");
  connect_wifi();

  setupNTP();//Conexão com o serviço de NTP

  delay(2000);
  Serial.println("Inicialização finalizada");

  xTaskCreatePinnedToCore(
                    AtualizaDataHora,  /* função que implementa a tarefa */
                    "AtualizaDataHora",/* nome da tarefa */
                    5000,              /* número de palavras a serem alocadas para uso com a pilha da tarefa */
                    NULL,              /* parâmetro de entrada para a tarefa (pode ser NULL) */
                    1,                 /* prioridade da tarefa (0 a N) */
                    NULL,              /* referência para a tarefa (pode ser NULL) */
                    taskCoreZero);     /* Núcleo que executará a tarefa */

  xTaskCreatePinnedToCore(
                    buttonPressed,  /* função que implementa a tarefa */
                    "buttonPressed",/* nome da tarefa */
                    10000,              /* número de palavras a serem alocadas para uso com a pilha da tarefa */
                    NULL,              /* parâmetro de entrada para a tarefa (pode ser NULL) */
                    0,                 /* prioridade da tarefa (0 a N) */
                    NULL,              /* referência para a tarefa (pode ser NULL) */
                    taskCoreZero);     /* Núcleo que executará a tarefa */

  xTaskCreatePinnedToCore(
                    repeatedCall,  /* função que implementa a tarefa */
                    "repeatedCall",/* nome da tarefa */
                    10000,              /* número de palavras a serem alocadas para uso com a pilha da tarefa */
                    NULL,              /* parâmetro de entrada para a tarefa (pode ser NULL) */
                    0,                 /* prioridade da tarefa (0 a N) */
                    NULL,              /* referência para a tarefa (pode ser NULL) */
                    taskCoreZero);     /* Núcleo que executará a tarefa */
}

//Função responsavel pela conexão ao WIFI
void connect_wifi() {
  Serial.print("Conectando Wifi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi conectado - IP address: ");
  Serial.println(WiFi.localIP());
}

//Função responsavel pela conexão com o Client NTP
void setupNTP()
{
    ntpClient.begin();
    
    Serial.println("Aguardando primeiro update NTP");
    while(!ntpClient.update())
    {
        Serial.print(".");
        ntpClient.forceUpdate();
        delay(500);
    }

    Serial.println();
}

//Função responsavel pelo calculo da data atual, com base no NTPClient
String getDate() {
  unsigned long rawTime = ntpClient.getEpochTime() / 86400L;  //valor em dias
  unsigned long days = 0, year = 1970;
  uint8_t month;
  static const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};

  while((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)//Tratamento ano bisexto
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365);
  days=0;
  for (month=0; month<12; month++) {
    uint8_t monthLength;
    if (month==1) { // Fevereiro
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    } else {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength) break;
    rawTime -= monthLength;
  }
  String monthStr = ++month < 10 ? "0" + String(month) : String(month); // jan is month 1  
  String dayStr = ++rawTime < 10 ? "0" + String(rawTime) : String(rawTime); // day of month  
  return dayStr+"/"+monthStr+"/"+String(year);
}

//Busca de atualizações de Firmware via HTTPUpdate
void firmwareUpdate(void) {
  // Cria instância de Cliente seguro
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);

  httpUpdate.setLedPin(LED_PIN, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}

//Checagem de versão disponivel do Firmware via HTTPUpdate
int FirmwareVersionCheck(void) {
  String payload;
  int httpCode;
  String fwurl = "";
  fwurl += URL_fw_Version;
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure * client = new WiFiClientSecure;

  if (client) 
  {
    client -> setCACert(rootCACertificate);

    HTTPClient https;// Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 

    if (https.begin( * client, fwurl)) 
    { // HTTPS      
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(100);
      if (httpCode == HTTP_CODE_OK) // if version received
      {
        payload = https.getString(); // save received version
      } else {
        Serial.print("Erro no download do arquivo de versão:");
        Serial.println(httpCode);
      }
      https.end();
    }
    delete client;
  }
      
  if (httpCode == HTTP_CODE_OK) // if version received
  {
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      Serial.printf("\nNão existe atualizações disponiveis, firmware atual: %s\n", FirmwareVer);
      return 0;
    } 
    else 
    {
      Serial.println(payload);
      Serial.println("Novo firmware detectado");
      return 1;
    }
  } 
  return 0;  
}

//############################################################
//#################### CORE_0 ################################
//############################################################
//Função responsavel pela atualização dos dados de Data e Hora
void AtualizaDataHora( void * pvParameters ){
  while(true){
    HoraAtual = ntpClient.getFormattedTime();
    DataAtual = getDate();
    delay(500);
  }
}

//Função responsavel por monitorar o botão Boot.
void buttonPressed( void * pvParameters ){
  while(true){
    if (button_boot.pressed) { //to connect wifi via Android esp touch app 
      Serial.println("Botão pressionado - buscando atualizações de Firmware..");
      if (FirmwareVersionCheck()) {
        firmwareUpdate();
      }
      button_boot.pressed = false;
    }
    delay(500);
  }
}

//Buscar atualização de forma autonoma
void repeatedCall( void * pvParameters ){
  const long interval = 60000;//60 segundos
  const long mini_interval = 1000;//1 segundo
  static int num=0;
  unsigned long currentMillis = millis();
  
  while(true){
    currentMillis = millis();
    if ((currentMillis - repeatedCallMillis) >= interval) {
      repeatedCallMillis = currentMillis;
      if (FirmwareVersionCheck()) {
        firmwareUpdate();
      }
    }
    if ((currentMillis - repeatedCallMillis_2) >= mini_interval) {
      repeatedCallMillis_2 = currentMillis;
      Serial.print("idle loop...");
      Serial.print(num++);
      Serial.print(" Active fw version:");
      Serial.println(FirmwareVer);
      if(WiFi.status() == WL_CONNECTED) 
      {
        Serial.println("wifi connected");
      }
      else
      {
        connect_wifi();
      }
    }
    delay(500);
  }
}

//############################################################
//#################### CORE_1 ################################
//############################################################
void loop() {
  Serial.println(DataAtual +" "+HoraAtual);
  delay(2000);
}




























