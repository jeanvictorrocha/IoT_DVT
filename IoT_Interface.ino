//Função responsavel pela atualização da tela oled
void Interface(void *arg){
  bool StatusAtual = getStatusAddress("0x3c");
  bool InitOk = false;

  while(true){
    if(StatusAtual != getStatusAddress("0x3c")){
      StatusAtual = !StatusAtual;
      oLed_0x3C.init();
      oLed_0x3C.flipScreenVertically();
      InitOk = true;
    }//else{
      //Serial.println("Interface - FALSE");
    //}
    //Executa apenas se o Objeto existir
    if(InitOk){
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
    }
    delay(1000);
  }
}

//Função responsavel por monitorar o botão Boot.
void buttonPressed(void *arg){
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