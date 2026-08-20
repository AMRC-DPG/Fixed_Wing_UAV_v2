import socket

HOST = ''
PORT = 5001

QD = 0x1
QS = 0x2
QC = 0x3
QV = 0x4
QT = 0x5
QSS = 0x6
QAS = 0x7
QAH = 0x8
QAA = 0x9
QAD = 0xA
QSD = 0xB
QLED = 0xC
QG = 0xD
QMS = 0xE
QF = 0xF
QAR = 0x10

ID_OFFSET = 24

def makeFanCmd(value):
     if value < 0: value = 0
     elif value > 100: value = 100
     else: pass
    #1100 = 0%, 1300 = 100%
     fanSetting = (value*2)+1100 #Map 0-100% range to within 1100 to 1300
     return "F:"+ str(fanSetting)

def makeEStopCmd():
     return b"STOP"

def makeServoCmd(value, time):
     degrees = 0.0 #0-90 degrees
     ms = 500 #time in milliseconds

     if value > 90.0: degrees = 90.0
     elif value < 0.0: degrees = 0.0
     else:pass

     return "S:" + str(degrees) + ":" + str(ms)

def mapLiveData(obj):
    print(obj)
    x = ALH.getValue(self, "liveValues")
    ALH.setValue(x, "fanRpm", obj.get("fan").get("rpm"))
    ALH.setValue(x, "fanPwm", obj.get("fan").get("pwm"))
    ALH.setValue(x, "servoPos", obj.get("servo").get("pos"))
    ALH.setValue(x, "servoRpm", obj.get("servo").get("rpm"))
    ALH.setValue(x, "servoCur", obj.get("servo").get("cur"))
    ALH.setValue(x, "servoVol", obj.get("servo").get("vol"))
    ALH.setValue(x, "servoTmp", obj.get("servo").get("tmp"))
    ALH.setValue(x, "servoStat", obj.get("servo").get("stat"))

def mapCfgData(obj):
    print(obj)
    x = ALH.getValue(self, "configValues")
    ALH.setValue(x, "fanMin", obj.get("fan").get("min"))
    ALH.setValue(x, "fanMax", obj.get("fan").get("max"))
    ALH.setValue(x, "servoModel", obj.get("servo").get("model"))
    ALH.setValue(x, "servoFw", obj.get("servo").get("fw"))
    ALH.setValue(x, "servoStiff", obj.get("servo").get("stiff"))
    ALH.setValue(x, "servoHstiff", obj.get("servo").get("hstiff"))
    ALH.setValue(x, "servoAcc", obj.get("servo").get("acc"))
    ALH.setValue(x, "servoDec", obj.get("servo").get("dec"))
    ALH.setValue(x, "servoMaxspd", obj.get("servo").get("maxSpd"))
    ALH.setValue(x, "servoLed", obj.get("servo").get("led"))
    ALH.setValue(x, "servoGyre", obj.get("servo").get("gyre"))

try:
    s= socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((HOST, PORT))
    s.listen(1)
    conn, addr = s.accept()

    try:
        print("Connected by ", addr)
        f=conn.makefile()
        while True:
            data = f.readline()[:-1]
            if not data: break
            if data == None: break
            #print(data)
            
            msgHeader = (data >> ID_OFFSET)

            if

            if ALH.getValue(self, "ready"):
                commandType = ALH.getValue(self, "type")
                value = ALH.getValue(self, "value")
                time = ALH.getValue(self, "time")
                cmdMessage = ""
                if commandType == "ESTOP":
                    cmdMessage = makeEStopCmd()
                elif commandType == "FanRPM":
                    cmdMessage = makeFanCmd(value)
                elif commandType == "ServoCommand":
                     cmdMessage = makeServoCmd(value, time)
                else: pass

                ALH.setValue(self, "ready", False)
                
                if cmdMessage: #i.e. if we have a message to send
                     conn.sendall(cmdMessage.encode())
                else: pass
            else: pass
     

    except Exception as e:
          print(type(e))
          print(e)
          print("Error occurred on:", data)
    finally:
           pass
except:
    pass
finally:
    s.close()
