package com.infinity360.remote;

import java.net.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.ArrayList;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

final class ConsoleDiscovery {
    private static final int PORT=32162;
    private static String mac(String secret,String text)throws Exception{
        Mac mac=Mac.getInstance("HmacSHA256");
        mac.init(new SecretKeySpec(secret.getBytes(StandardCharsets.US_ASCII),"HmacSHA256"));
        StringBuilder result=new StringBuilder();
        for(byte value:mac.doFinal(text.getBytes(StandardCharsets.US_ASCII))){
            result.append(Character.forDigit((value>>4)&15,16));result.append(Character.forDigit(value&15,16));
        }
        return result.toString();
    }
    static ArrayList<String> find(String local,String secret)throws Exception{
        if(!RemoteProtocol.privateIp(local)||!RemoteProtocol.validKey(secret))throw new IllegalArgumentException("IP/chave inválidos");
        InetAddress source=InetAddress.getByName(local);
        NetworkInterface network=NetworkInterface.getByInetAddress(source);
        if(network==null||!network.isUp()||network.isLoopback())throw new java.io.IOException("Interface indisponível");
        InetAddress broadcast=null;
        for(InterfaceAddress iface:network.getInterfaceAddresses())if(source.equals(iface.getAddress())){broadcast=iface.getBroadcast();break;}
        if(broadcast==null||!(broadcast instanceof Inet4Address))throw new java.io.IOException("Broadcast IPv4 indisponível");
        String nonce=RemoteProtocol.newKey();
        byte[] request=("INFINITY360-DISCOVER/1 "+nonce).getBytes(StandardCharsets.US_ASCII);
        ArrayList<String> found=new ArrayList<>();
        try(DatagramSocket socket=new DatagramSocket(new InetSocketAddress(source,0))){
            socket.setBroadcast(true);socket.setSoTimeout(250);
            long start=android.os.SystemClock.elapsedRealtime(),deadline=start+2400;
            socket.send(new DatagramPacket(request,request.length,broadcast,PORT));
            boolean repeated=false;
            while(android.os.SystemClock.elapsedRealtime()<deadline&&found.size()<8){
                if(!repeated&&android.os.SystemClock.elapsedRealtime()-start>=1000){
                    socket.send(new DatagramPacket(request,request.length,broadcast,PORT));repeated=true;
                }
                byte[] buffer=new byte[256];DatagramPacket packet=new DatagramPacket(buffer,buffer.length);
                try{socket.receive(packet);}catch(SocketTimeoutException e){continue;}
                String ip=packet.getAddress().getHostAddress();
                if(packet.getPort()!=PORT||!RemoteProtocol.privateIp(ip)||ip.equals(local))continue;
                String msg="INFINITY360/1 "+nonce+" "+ip;
                String expected=msg+" "+mac(secret,msg);
                String answer=new String(packet.getData(),packet.getOffset(),packet.getLength(),StandardCharsets.US_ASCII);
                if(MessageDigest.isEqual(expected.getBytes(StandardCharsets.US_ASCII),answer.getBytes(StandardCharsets.US_ASCII))&&!found.contains(ip))found.add(ip);
            }
        }
        return found;
    }
}
