/********************************************************
 * Jean Victor Rocha
 * IoT DVT
 */

// Bibliotecas ------------------------------------------
#include <WiFi.h>
#include <HTTPUpdate.h>
#include <SPIFFS.h>

// Constantes -------------------------------------------
// Wi-Fi
const char*   SSID      = "CORP-TLG";
const char*   PASSWORD  = "y.q<5sQWKj#E";

//Blink
// ESP32 não possui pino padrão de LED
const byte      LED_PIN                 = 2;
// ESP32 utilizam estado normal
const byte      LED_ON                  = HIGH;
const byte      LED_OFF                 = LOW;

// Setup ------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("\nIniciando");

  // Inicializa SPIFFS
  if (SPIFFS.begin()) {
    Serial.println("SPIFFS Ok");
  } else {
    Serial.println("SPIFFS Falha");
  }

  // Verifica / exibe arquivo
  if (SPIFFS.exists("/Teste.txt")) {
    File f = SPIFFS.open("/Teste.txt", "r");
    if (f) {
      Serial.println("Lendo arquivo:");
      Serial.println(f.readString());
      f.close();
    }
  } else {
    Serial.println("Arquivo não encontrado");
  }

  // Conecta WiFi
  WiFi.begin(SSID, PASSWORD);
  Serial.println("\nConectando WiFi " + String(SSID));
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("WiFi connected - IP address: ");
  Serial.println(WiFi.localIP());

  // LED indicador de progresso
  httpUpdate.setLedPin(2, HIGH);

  // Callback - Progresso
  Update.onProgress([](size_t progresso, size_t total) {
    Serial.print(progresso * 100 / total);
    Serial.print(" ");
  });

  // Não reiniciar automaticamente
  httpUpdate.rebootOnUpdate(false);

  // Cria instância de Cliente seguro
  WiFiClientSecure client;
  // Instrui Cliente a ignorar assinatura do Servidor na conexao segura
  client.setInsecure();

  // Atualização SPIFFS ---------------------------------
  Serial.println("\n*** Atualização da SPIFFS ***");
  for (byte b = 5; b > 0; b--) {
    Serial.print(b);
    Serial.println("... ");
    delay(300);
  }
  // Encerra SPIFFS
  SPIFFS.end();

  t_httpUpdate_return resultado = httpUpdate.updateSpiffs(client, "//192.168.100.246/publico/TI/IOT/spiffs.bin");

  // Verifica resultado
  switch (resultado) {
    case HTTP_UPDATE_FAILED: {
      String s = httpUpdate.getLastErrorString();
      Serial.println("\nFalha: " + s);
    } break;
    case HTTP_UPDATE_NO_UPDATES: {
      Serial.println("\nNenhuma atualização disponível");
    } break;
    case HTTP_UPDATE_OK: {
      Serial.println("\nSucesso");
    } break;
  }

  // Inicializa SPIFFS
  if (SPIFFS.begin()) {
    Serial.println("SPIFFS Ok");
  } else {
    Serial.println("SPIFFS Falha");
  }

  // Verifica / exibe arquivo
  if (SPIFFS.exists("/Teste.txt")) {
    File f = SPIFFS.open("/Teste.txt", "r");
    if (f) {
      Serial.println("Lendo arquivo:");
      Serial.println(f.readString());
      f.close();
    }
  } else {
    Serial.println("Arquivo não encontrado");
  }

  // Atualização Sketch ---------------------------------
  Serial.println("\n*** Atualização do sketch ***");
  for (byte b = 5; b > 0; b--) {
    Serial.print(b);
    Serial.println("... ");
    delay(300);
  }

  // Efetua atualização do Sketch
  resultado = httpUpdate.update(client, "//192.168.100.246/publico/TI/IOT/IoT_DVT.ino.bin");

  // Verifica resultado
  switch (resultado) {
    case HTTP_UPDATE_FAILED: {
      String s = httpUpdate.getLastErrorString();
      Serial.println("\nFalha: " + s);
    } break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("\nNenhuma atualização disponível");
      break;
    case HTTP_UPDATE_OK: {
      Serial.println("\nSucesso, reiniciando...");
      ESP.restart();
    } break;
  }

  //Blink
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);
  Serial.println("\n\nIniciando blink");
}

// Loop --------------------------------------------
void loop() {
  //Blink
  digitalWrite(LED_PIN, LED_ON);
  delay(300);
  digitalWrite(LED_PIN, LED_OFF);
  delay(300);
}
