void CheckWIFIConnected(void *arg){
  while(true){
    if(WiFi.status() == WL_CONNECTED) {
      Serial.println("Wifi connected");
    }
    else {
      connect_wifi();
    }
    delay(2000);
  }
}