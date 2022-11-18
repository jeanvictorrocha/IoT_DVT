
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

//Função responsavel pela atualização dos dados de Data e Hora
void AtualizaDataHora(void *arg){
  while(true){
    HoraAtual = ntpClient.getFormattedTime();
    DataAtual = getDate();
    delay(1000);
  }
}