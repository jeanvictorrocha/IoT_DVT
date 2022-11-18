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
#include <PCF8575.h>//PCF8575

//Prototipos de funções externas
extern void CheckI2c();
extern void CheckWIFIConnected();
extern void connect_wifi();
extern void setupNTP();
extern void AtualizaDataHora();
extern void Interface();
extern void buttonPressed();
extern int FirmwareVersionCheck();
extern void firmwareUpdate();
extern void LoopFirmware();
extern String getDate();
extern bool getStatusAddress();

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
Address Address_Enable[3] = {{33,"0x21",false,"PCF8575"},
                              {60,"0x3c",false,"Display oLed"},
                              {72,"0x48",false,"ADS1015 - Sensor de corrente nao invasivo SCT-013-100"}
                            };
/*0x20 - 0x27 = PCF8575
  0x48 - ADS1015
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
void IRAM_ATTR isr(void) {
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
  Wire.begin();
  
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
                    CheckI2c,
                    "CheckI2c",
                    10000,
                    NULL,
                    1,
                    NULL,
                    taskCoreZero);
  xTaskCreatePinnedToCore(
                    Interface,
                    "Interface",
                    10000,
                    NULL,
                    1,
                    NULL,
                    taskCoreZero);
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

void loop() {

}