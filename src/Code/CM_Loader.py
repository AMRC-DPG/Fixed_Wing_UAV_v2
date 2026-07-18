import socket
import time

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 5001 
#dataset = "Dataset/datalog.txt"
dataset = "Dataset/CommandTesting.txt"

with open(dataset) as f:
    data = f.readlines()

#for x in data:
    #print(x[:x.rindex("}")], "\n")

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    print("Connected!")
    for x in data: 
        message = bytes(x[:x.rindex(":")].encode("utf-8"))
        #message = x[:x.rindex("}")].encode("utf-8")
        s.sendall(message)
        print("Sent: ", message)
        """
        data = s.recv(1024)
        if data: print(data)
        """
        time.sleep(0.2)