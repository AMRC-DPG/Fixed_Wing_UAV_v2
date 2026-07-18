import java.io.IOException; 
import java.net.DatagramPacket; 
import java.net.DatagramSocket; 
public class smallserver { 
  
    public static void main(String[] args) throws IOException { 
          
        DatagramSocket ds = new DatagramSocket(5252); 
        byte buf[] = new byte[2]; 
        byte send[] = { 13, 18 }; 
        DatagramPacket dp = new DatagramPacket(buf, 2); 
  
        ds.receive(dp); 
  
        DatagramPacket senddp = new DatagramPacket(send, 2, dp.getAddress(), dp.getPort()); 
        ds.send(senddp); 
    } 
  
}