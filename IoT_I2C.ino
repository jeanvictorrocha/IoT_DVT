//Checagem de uso de barramento I2C
void CheckI2c(void *arg) {
  byte error;
  int nDevices = 0;
  int LenAddress_Enable = sizeof(Address_Enable)/sizeof(Address_Enable[0]);

  while (true) {
    //Serial.print("[CheckI2c] Scanning ");
    //Serial.print(LenAddress_Enable);
    //Serial.println(" address ...");
    for(byte i = 0; i < LenAddress_Enable; i++ ) {
      Address_Enable[i].ENABLE = false;
      Wire.beginTransmission(Address_Enable[i].ADDRESS);
      error = Wire.endTransmission();
      if (error == 0) {
        Address_Enable[i].HEXA = "0x" + String(Address_Enable[i].ADDRESS, HEX);
        Address_Enable[i].ENABLE = true;
      }
      else if (error==4) {
        Address_Enable[i].HEXA = "0x" + String(Address_Enable[i].ADDRESS, HEX);
        Address_Enable[i].ENABLE = false;
        Serial.print("Unknow error at address " + Address_Enable[i].HEXA);
      }
      if(Address_Enable[i].ENABLE){
        Serial.println("[CheckI2c] " + Address_Enable[i].HEXA + " ENABLE");
      }else{
        Serial.println("[CheckI2c] " + Address_Enable[i].HEXA + " DISABLE");
      }
    }
    delay(5000);
  }
}
