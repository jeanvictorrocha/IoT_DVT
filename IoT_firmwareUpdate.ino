//Buscar atualização de forma autonoma
void LoopFirmware(void *arg){
  const long interval = 60000;//60 segundos
  while(true){
    if (FirmwareVersionCheck()) {
      firmwareUpdate();
    }
    delay(interval);
  }
}

//Checagem de versão disponivel do Firmware via HTTPUpdate
int FirmwareVersionCheck() {
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
void firmwareUpdate() {
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