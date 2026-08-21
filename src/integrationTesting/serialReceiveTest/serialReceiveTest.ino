void setup()
{
  // put your setup code here, to run once:
  randomSeed(255);
  Serial.begin(115200);
  while(!Serial){;} //Wait until Serial is connected before leaving this function
}

void loop()
{
  // put your main code here, to run repeatedly:
  int value = random(0xFF);
  //Serial.println(value);
  //delay(200);
}

void emptySerialBuffer()
{
  while(Serial.available()>0)
  {
    char t = Serial.read();
  }
}

void serialEvent()
{
  char data[8];
  while(Serial.available())
  {
    //long command = Serial.parseInt();
    int bytesRead = Serial.readBytes(data, 8);
    emptySerialBuffer();
    long value = 0;
    int c;

    for(int i = 0; i < (sizeof(data)/sizeof(char)); i++)
    {
      c = data[i];
      if(c >= '0' && c <= '9')        // is c a digit?
      {value = value * 10 + c - '0';}
    }
    unsigned int type = ((value & 0xFFFF0000) >> 0x10);
    Serial.println(type);
  }
}