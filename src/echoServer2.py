import socket
import time

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 5001  # Port to listen on (non-privileged ports are > 1023)

#Incoming Message info
INCOMING_ID_OFFSET = 24
INCOMING_BIT_MASK = 0xFFFFFF
QD = 0X1
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

#Outgoing Message Info
OUTGOING_ID_OFFSET = 16
CMD_FANSPEED = 1
CMD_SERVO = 2
CMD_ESTOP = 3
CMD_AS_CONFIG = 4
CMD_AH_CONFIG = 5
CMD_AA_CONFIG = 6
CMD_AD_CONFIG = 7


def createMsg(id, msg):
    return (id << OUTGOING_ID_OFFSET) | msg


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    conn, addr = s.accept()
    starttime = time.time()
    log = open("testing.txt", "w")
    try:
        with conn:
            print(f"Connected by {addr}")
            data = ""
            f = conn.makefile()
            counter = 0
            fanRunning = False

            while True:
                """
                datatime = time.time() - starttime
                logText = data + ":" + str(datatime) + "\n"
                print(logText)
                log.write(logText)
                data = conn.recv(1024)
                print(datatime)
                """
                
                asCommand = createMsg(CMD_AS_CONFIG, 2)
                print("Sent:", asCommand)
                conn.send(str(asCommand).encode())
                
                data = f.readline()[:-1]
                #print(data)
                if(data):
                    number = int(data)
                    #print("Received: ", number)
                    msgType = number >> INCOMING_ID_OFFSET
                    msgData = number & INCOMING_BIT_MASK
                    if msgType == QAS:
                       print("AS Value: ", msgData)
                



    except KeyboardInterrupt:
        #conn.sendall(emergencyStopCommand)
        log.close()
