
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