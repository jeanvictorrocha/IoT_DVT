//Função responsavel pela atualização dos dados de Data e Hora
void AtualizaDataHora(void *arg){
  while(true){
    HoraAtual = ntpClient.getFormattedTime();
    DataAtual = getDate();
    delay(1000);
  }
}