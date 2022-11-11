/********************************************************
 * Jean Victor Rocha
 * IoT DVT
 *******************************************************/
String FirmwareVer = {"0.2"};

// Bibliotecas ----------------------------------------------------------------------------------------------
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "data\cert.h"//Certificado Github
#include <NTPClient.h> //Hora NTP - https://github.com/arduino-libraries/NTPClient
#include <Adafruit_ADS1X15.h>//Biblioteca ADS - 0x48-L;0x49-H;0x4A-SDA;0x4B-SCL
#include <SSD1306Wire.h>//Biblioteca OLED - 0x3c

//Definições ---------------------------------------------------------------------------------------------------
#define LEAP_YEAR(Y)     ( (Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) ) )
#define URL_fw_Version "https://raw.githubusercontent.com/jeanvictorrocha/IoT_DVT/main/version.txt"
#define URL_fw_Bin     "https://raw.githubusercontent.com/jeanvictorrocha/IoT_DVT/main/build/esp32.esp32.esp32/IoT_DVT.ino.bin"

//Definição de mudulos e objetos ------------------------------------------------------------------------------
Adafruit_ADS1115 ADS1015L;  /* Use this for the 12-bit version */
Adafruit_ADS1015 ADS1115H;  /* Use this for the 16-bit version */

SSD1306Wire oLed_0x3C(0x3c,SDA, SCL);

WiFiUDP udp;//Socket UDP que a lib utiliza para recuperar dados sobre o horário
NTPClient ntpClient(udp,"1.br.pool.ntp.org",(-3)*3600,60000);//Objeto responsável por recuperar dados sobre horário //-3 = SP-Brasil

//Variaveis ----------------------------------------------------------------------------------------------------
static uint8_t taskCoreZero = 0;
static uint8_t taskCoreOne  = 1;

struct Address
{ int ADDRESS;
  String HEXA;
  bool ENABLE;
  String DESC; 
};
//Address Address_Enable[127];
Address Address_Enable[2] = { {60,"0x3c",false,"Display oLed"},
                              {72,"0x48",false,"ADS1015 - Sensor de corrente nao invasivo SCT-013-100"}
                            };
/*0x48 - ADS1015
  0x49 - ADS1X15
  0x4A - ADS1X15
  0x4B - ADS1X15
  0x3C - OLED Display 0.96"
  0x70 - TCA9548A*/

String DataAtual = "";
String HoraAtual = "";

//char* dayOfWeekNames[] = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sabado"};//Nomes dos dias da semana

const byte      LED_PIN = 2;

unsigned long repeatedCallMillis = 0;
unsigned long repeatedCallMillis_2 = 0;

//const char * ssid = "CORP-TLG";
//const char * password = "y.q<5sQWKj#E";
const char * ssid = "GRU_Office";
const char * password = "K4ilztqh03@#";
//const char * ssid = "J2-VIVO-4G";
//const char * password = "3346878710";

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
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(button_boot.PIN, INPUT);
  attachInterrupt(button_boot.PIN, isr, RISING);

  Serial.println("IOT_DANNY - iniciando --------------------------------");
  Serial.print("[Setup] Versão do firmware: ");
  Serial.println(FirmwareVer);
  int LenAddress_Enable = sizeof(Address_Enable)/sizeof(Address_Enable[0]);

  //Rastreio dos endereços I2C
  Serial.println("Inicializando I2C Scanner...");
  Wire.begin();
  CheckI2c();
  for (byte i = 0; i < LenAddress_Enable; i++) {
    if (Address_Enable[i].ENABLE) {
      Serial.print("[Setup] Address: ");
      Serial.print(Address_Enable[i].ADDRESS);
      Serial.print(" HEX: ");
      Serial.print(Address_Enable[i].HEXA);
      Serial.print(" DESC: ");
      Serial.println(Address_Enable[i].DESC);      
    }
  } 

  //Inicializa o display Oled
  if(getStatusAddress("0x3c")){
    //Definição de atualização de Display oled
    xTaskCreatePinnedToCore(
                          Interface,
                          "Interface",
                          10000,
                          NULL,
                          1,
                          NULL,
                          taskCoreZero);
  }
  //Inicialização do ADS
  if(getStatusAddress("0x48")){
    if (!ADS1015L.begin(0x48)) {
      Serial.println("[Setup] Falha na inicialização do endereço 0x48.");
    }
  }
  if(getStatusAddress("0x49")){
    if (!ADS1015L.begin(0x49)) {
      Serial.println("[Setup] Falha na inicialização do endereço 0x49.");
    }
  }

  connect_wifi();
  setupNTP();//Conexão com o serviço de NTP
  delay(2000);
  
  //#################### CORE_0 ################################
  xTaskCreatePinnedToCore(
                    AtualizaDataHora,  //função que implementa a tarefa 
                    "AtualizaDataHora",//nome da tarefa 
                    5000,              //número de palavras a serem alocadas para uso com a pilha da tarefa 
                    NULL,              //parâmetro de entrada para a tarefa (pode ser NULL) 
                    1,                 //prioridade da tarefa (0 a N) 
                    NULL,              //referência para a tarefa (pode ser NULL) 
                    taskCoreZero);     //Núcleo que executará a tarefa 
  xTaskCreatePinnedToCore(
                    CheckWIFIConnected,
                    "CheckWIFIConnected",
                    5000,
                    NULL,
                    1,
                    NULL,
                    taskCoreZero);
  xTaskCreatePinnedToCore(
                    LoopFirmware,
                    "LoopFirmware",
                    10000,
                    NULL,
                    1,
                    NULL,
                    taskCoreZero);
  xTaskCreatePinnedToCore(
                    buttonPressed,
                    "buttonPressed",
                    10000,
                    NULL,
                    1,
                    NULL,
                    taskCoreZero);

  //#################### CORE_1 ################################

  Serial.println("[Setup] FIM");
}

//Checagem de uso de barramento I2C
void CheckI2c() {
  byte error;
  int nDevices = 0;
  int LenAddress_Enable = sizeof(Address_Enable)/sizeof(Address_Enable[0]);
  
  Serial.print("[CheckI2c] Scanning ");
  Serial.print(LenAddress_Enable);
  Serial.println(" address ...");
  for(byte i = 0; i < LenAddress_Enable; i++ ) {
    Address_Enable[i].ENABLE = false;
    Wire.beginTransmission(Address_Enable[i].ADDRESS);
    error = Wire.endTransmission();
    if (error == 0) {
      Address_Enable[i].HEXA = "0x" + String(Address_Enable[i].ADDRESS, HEX);
      Address_Enable[i].ENABLE = true;
    }
    else if (error==4) {
      Address_Enable[i].HEXA = "0x" + String(Address_Enable[i].ADDRESS, HEX);
      Address_Enable[i].ENABLE = false;
      Serial.print("Unknow error at address 0x");
    }
  }
  Serial.println("[CheckI2c] finish Scan ");
}

//Funções diversas
bool getStatusAddress(String keyAddress) {
  Serial.print("[getStatusAddress] Start : ");
  Serial.println(keyAddress);
  bool return_value = false;
  int LenAddress_Enable = sizeof(Address_Enable)/sizeof(Address_Enable[0]);

  for ( uint8_t i = 0; i < LenAddress_Enable; i++ )
    if ( Address_Enable[i].HEXA == keyAddress){
      return_value = Address_Enable[i].ENABLE;
      break;
    }
  Serial.println("[getStatusAddress] finish ");
  return return_value;
}

//Função responsavel pela conexão ao WIFI
void connect_wifi() {
  Serial.print("[connect_wifi] Conectando Wifi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("[connect_wifi] WiFi conectado - IP address: ");
  Serial.println(WiFi.localIP());
}

//Função responsavel pela conexão com o Client NTP
void setupNTP() {
    ntpClient.begin();
    Serial.println("[setupNTP] Aguardando primeiro update NTP");
    while(!ntpClient.update()) {
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

  if (client) {
    client -> setCACert(rootCACertificate);
    HTTPClient https;// Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 

    if (https.begin( * client, fwurl)) { // HTTPS      
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(100);
      if (httpCode == HTTP_CODE_OK) {// if version received
        payload = https.getString(); // save received version
      } else {
        Serial.print("Erro no download do arquivo de versão:");
        Serial.println(httpCode);
      }
      https.end();
    }
    delete client;
  }
      
  if (httpCode == HTTP_CODE_OK) {// if version received
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      Serial.printf("Não existe atualizações disponiveis, firmware atual: %s\n", FirmwareVer);
      return 0;
    } 
    else {
      Serial.println(payload);
      Serial.println("Novo firmware detectado");
      return 1;
    }
  } 
  return 0;  
}

//Busca de atualizações de Firmware via HTTPUpdate
void firmwareUpdate(void) {
  WiFiClientSecure client;// Cria instância de Cliente seguro
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

//############################################################
//#################### CORE_0 ################################
//############################################################
//Função responsavel pela atualização da tela oled
void Interface( void * pvParameters ){
  oLed_0x3C.init();
  oLed_0x3C.flipScreenVertically();
  while(true){
    oLed_0x3C.clear();
    oLed_0x3C.setFont(ArialMT_Plain_10);
    oLed_0x3C.setTextAlignment(TEXT_ALIGN_CENTER);
    oLed_0x3C.drawString(64,2,"Danny");

    if(HoraAtual != ""){
      oLed_0x3C.setFont(ArialMT_Plain_10);
      oLed_0x3C.setTextAlignment(TEXT_ALIGN_RIGHT);
      oLed_0x3C.drawString(126,2,HoraAtual.substring(0,5));
    }
    oLed_0x3C.display();
    delay(1000);
  }
}

//Função responsavel pela atualização dos dados de Data e Hora
void AtualizaDataHora( void * pvParameters ){
  while(true){
    HoraAtual = ntpClient.getFormattedTime();
    DataAtual = getDate();
    delay(1000);
  }
}

void CheckWIFIConnected(){
  while(true){
    if(WiFi.status() == WL_CONNECTED) {
      Serial.println("Wifi connected");
    }
    else {
      connect_wifi();
    }
    delay(1000);
  }
}

//Função responsavel por monitorar o botão Boot.
void buttonPressed( void * pvParameters ){
  while(true){
    if (button_boot.pressed) {
      Serial.println("[buttonPressed] Forçando busca de atualizações de Firmware..");
      if (FirmwareVersionCheck()) {
        firmwareUpdate();
      }
      button_boot.pressed = false;
    }
    delay(500);
  }
}

//Buscar atualização de forma autonoma
void LoopFirmware(){
  const long interval = 60000;//60 segundos
  while(true){
    if (FirmwareVersionCheck()) {
      firmwareUpdate();
    }
    delay(interval);
  }
}

//############################################################
//#################### CORE_1 ################################
//############################################################
void loop() {
  //Serial.println("[Loop] "+DataAtual +" "+HoraAtual);
  //delay(2000);
}