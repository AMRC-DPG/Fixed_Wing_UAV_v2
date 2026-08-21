#include <Arduino.h>

#define ID_OFFSET 24
//Limit to 16 bits data identifier, we should be good for a while
#define QD 0X1
#define QS 0x2
#define QC 0x3
#define QV 0x4
#define QT 0x5
#define QSS 0x6
#define QAS 0x7
#define QAH 0x8
#define QAA 0x9
#define QAD 0xA
#define QSD 0xB
#define QLED 0xC
#define QG 0xD
#define QMS 0xE
#define QF 0xF
#define QAR 0x10 //This isn't actually used

class systemState
{
  private:
    int AS, AH, AA, AD, FanMicro, servoPos, servoSpeed;
    bool estop;
  public:
    systemState()
    {
      this->AS = 0;
      this->AH = 0;
      this->AA = 0;
      this->AD = 0;
      this->FanMicro = 0;
      this->servoPos = 0;
      this->servoSpeed = 0;
      this->estop = false;
    }
    ~systemState()
    {
    }
    void setAS(int x)
    {
      this->AS = x;
    }
    int getAS()
    {
      return this->AS;
    }
    void setAH(int x)
    {
      this->AH = x;
    }
    int getAH()
    {
      return this->AH;
    }
    void setAA(int x)
    {
      this->AA = x;
    }
    int getAA()
    {
      return this->AA;
    }
    void setAD(int x)
    {
      this->AD = x;
    }
    int getAD()
    {
      return this->AD;
    }
    void setFanMicro(int x)
    {
      this->FanMicro = x;
    }
    int getFanMicro()
    {
      return this->FanMicro;
    }
    void setServoPos(int x)
    {
      this->servoPos = x;
    }
    int getServoPos()
    {
      return this->servoPos;
    }
    void setServoSpeed(int x)
    {
      this->servoSpeed = x;
    }
    int getServoSpeed()
    {
      return this->servoSpeed;
    }
    void setEStop(bool x)
    {
      this->estop = x;
    };
    bool getEStop()
    {
      return this->estop;
    };
};

systemState dummy;

void setup()
{
  //Pushed to end of setup so that we only have connection when ready
  randomSeed(255);
  Serial.begin(115200);
  while(!Serial){;} //Wait until Serial is connected before leaving this function
}

void queryTelemetry (const char* query, int typeMsg, int bufferOffset)
{
  #define MAX_BUFFER 48 // Increased slightly for longer String replies
  char inputBuffer[MAX_BUFFER];
  byte bufferIndex = 0;
  int incomingByte = 0;
  long dataMsg = 0;
  int xdata = 0;

  switch(typeMsg)
  {
    case QD:
      xdata = random(0xFF);
    break;
    case QS:
      xdata = random(0xFF);
    break;
    case QC:
      xdata = random(0xFF);
    break;
    case QV:
      xdata = random(0xFF);
    break;
    case QT:
      xdata = random(0xFF);
    break;
    case QSS:
      xdata = random(0xFF);
    break;
    case QAS:
      xdata = dummy.getAS();
      break;
    case QAH:
      xdata = dummy.getAH();
      break;
    case QAA:
      xdata = dummy.getAA();
      break;
    case QAD:
      xdata = dummy.getAD();
      break;
    case QSD:
      xdata = random(0xFF);
      break;
    case QLED:
      xdata = random(0xFF);
      break;
    case QG:
      xdata = random(0xFF);
      break;
    case QMS:
      xdata = random(0xFF);
      break;
    case QF:
      xdata = random(0xFF);
      break;
    case QAR:
      xdata = random(0xFF);
      break;
  }
  
  dataMsg = (typeMsg << ID_OFFSET) | xdata;
  
  Serial.println(dataMsg);
}

void loop()
{
  // TWO-TIER SEQUENCER (Fires every 15ms)
  static unsigned long lastQuery = 0;
  static int iteration = 0;

  if (millis() - lastQuery > 15)
  {
    lastQuery = millis();

    // --- THE FAST LOOP (Live Telemetry - 10Hz) ---
    switch(iteration)
    {
      case 0: queryTelemetry("#1QD\r",QD, 4); break;
      case 1: queryTelemetry("#1QS\r",QS, 4); break;
      case 2: queryTelemetry("#1QC\r",QC, 4); break;
      case 3: queryTelemetry("#1QV\r",QV, 4); break;
      case 4: queryTelemetry("#1QT\r",QT, 4); break;
      case 5: queryTelemetry("#1Q\r",QSS, 3); break;
      case 6:
      // --- THE SLOW LOOP (Static Configs - 1Hz) ---
        queryTelemetry("#1QAS\r", QAS, 5);
        queryTelemetry("#1QAH\r", QAH, 5);
        queryTelemetry("#1QAA\r", QAA, 5);
        queryTelemetry("#1QAD\r", QAD, 5);
        queryTelemetry("#1QSD\r", QSD, 5);
        queryTelemetry("#1QLED\r", QLED, 6);
        queryTelemetry("#1QG\r", QG, 4);
        queryTelemetry("#1QAR\r", QAR, 5);
        queryTelemetry("#1QMS\r", QMS, 5);
        queryTelemetry("#1QF\r", QF, 4);
      break;
    }
    iteration > 6 ? iteration = 0 : iteration++;
  }
}

void emptySerialBuffer()
{
  while(Serial.available()>0)
  {
    char t = Serial.read();
  }
}

void serialEvent() //This will be called once per loop and the overhead is present based upon 
{
  char data[8];
  while(Serial.available())
  {
    //long command = Serial.parseInt();
    int bytesRead = Serial.readBytes(data, 8);
    emptySerialBuffer();

    long command = 0;
    int c = 0;


    for(int i = 0; i < (sizeof(data)/sizeof(char)); i++)
    {
      c = data[i];
      if(c >= '0' && c <= '9')        // is c a digit?
      {command = command * 10 + c - '0';}
    }
    data[8] = {0};

    unsigned int type = ((command & 0xFFFF0000) >> 0x10); //pulls the first 16 bits which can be used as identifier 
    unsigned int value = ((command & 0xFFFF)); //Pulls the last 16 bits for values up to 65535

    switch(type)
    {
      case 1: //case 1 handles fans
      {
        dummy.setFanMicro(value);
        break;
      }
      case 2: //case 2 handles servo
      {
        unsigned int targetTime = (value&&0xFF00) >> 0x10; //Get the first half bytw between 9-16
        dummy.setServoSpeed(targetTime);
        unsigned int lssPosition = (value&&0xFF)* 10; //Get the first 8 bits which covers 0 to 90
        dummy.setServoPos(lssPosition);
        break;
      }
      case 3: //case 3 handles ESTOP
      {
        (dummy.getEStop() == false)?dummy.setEStop(true):dummy.setEStop(false);
        break;
      }
      case 4: //case 4 handles AS config
      {
        Serial.println(4);
        if (value >= -4 && value <= 4)
        {
          dummy.setAS(value);
        }
        break;
      }
      case 5: //case 5 handles AH Config
      {
        if(value >= -10 && value <= 10)
        {
          dummy.setAH(value);
        }
        break;
      }
      case 6: //case 6 handles AA config
      {
        if (value >= 10 && value <= 10000)
        {
          dummy.setAA(value);
        }
        break;
      }

      case 7: //case 7 handles AD config
      {
        if (value >= 10 && value <= 10000)
        {
          dummy.setAD(value);
        }
        break;
      }
      default:
        Serial.println(command);
        break;
    }
  }
}
