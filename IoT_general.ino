
//Função responsavel pelo calculo da data atual, com base no NTPClient
String getDate() {
  unsigned long rawTime = ntpClient.getEpochTime() / 86400L;  //valor em dias
  unsigned long days = 0, year = 1970;
  uint8_t month;
  static const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};

  while((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)//Tratamento ano bisexto
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365);
  days=0;
  for (month=0; month<12; month++) {
    uint8_t monthLength;
    if (month==1) { // Fevereiro
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    } else {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength) break;
    rawTime -= monthLength;
  }
  String monthStr = ++month < 10 ? "0" + String(month) : String(month); // jan is month 1  
  String dayStr = ++rawTime < 10 ? "0" + String(rawTime) : String(rawTime); // day of month  
  return dayStr+"/"+monthStr+"/"+String(year);
}

//Retorna status do endereço
bool getStatusAddress(String keyAddress) {
  //Serial.print("[getStatusAddress] Start : ");
  //Serial.println(keyAddress);
  bool return_value = false;
  int LenAddress_Enable = sizeof(Address_Enable)/sizeof(Address_Enable[0]);

  for ( uint8_t i = 0; i < LenAddress_Enable; i++ )
    if ( Address_Enable[i].HEXA == keyAddress){
      return_value = Address_Enable[i].ENABLE;
      break;
    }
  //Serial.println("[getStatusAddress] finish ");
  return return_value;
}