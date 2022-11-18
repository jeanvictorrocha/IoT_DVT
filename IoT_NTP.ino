
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