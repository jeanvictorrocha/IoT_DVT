
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

void CheckWIFIConnected(void *arg){
  while(true){
    if(WiFi.status() == WL_CONNECTED) {
      //Serial.println("Wifi connected");
    }
    else {
      connect_wifi();
    }
    delay(2000);
  }
}