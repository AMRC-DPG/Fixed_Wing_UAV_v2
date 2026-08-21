import socket
import time

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 5001  # Port to listen on (non-privileged ports are > 1023)

fanSetting = 1200
fanCommand = "F:"+ str(fanSetting) #1100 = 0%, 1300 = 100%

degrees = 0.0 #0-90 degrees
ms = 500 #time in milliseconds

servoCommand = "S:" + str(degrees) + ":" + str(ms)
emergencyStopCommand = b"STOP"

verticalFlag = False

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
                data = f.readline()[:-1]
                datatime = time.time() - starttime
                logText = data + ":" + str(datatime) + "\n"
                print(logText)
                log.write(logText)

                counter += 1
                
                
                if counter == 25:
                    if not verticalFlag:
                        degrees = 0
                        verticalFlag = True
                    else:
                        degrees = 90
                        verticalFlag = False
                    servoCommand = "S:" + str(degrees) + ":" + str(ms)
                    conn.sendall(servoCommand.encode())
                    print("ServoCommand: ", servoCommand)
                    print("Vertical Flag set to: ", verticalFlag)

                
                if counter >= 50:
                    counter = 0
                    if not fanRunning:
                        fanSetting = 1200
                        fanCommand = "F:"+ str(fanSetting) #1100 = 0%, 1300 = 100%
                        conn.sendall(fanCommand.encode())
                        fanRunning = True
                        print("Fan Command: ", fanCommand)
                    elif fanRunning:
                        fanSetting = 1100
                        fanCommand = "F:"+ str(fanSetting) #1100 = 0%, 1300 = 100%
                        conn.sendall(fanOff.encode())
                        fanRunning = False
                        print("Fan Command: ", fanCommand)
                    else:
                        continue
                        



    except KeyboardInterrupt:
        conn.sendall(emergencyStopCommand)
        log.close()
