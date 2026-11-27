package com.infinity360.remote;
import java.net.HttpURLConnection;
import java.net.Proxy;
import java.net.URL;
import java.security.SecureRandom;

final class RemoteProtocol {
    static final int PORT = 32161;
    static boolean privateIp(String s) {
        if (s == null) return false;
        String[] parts=s.split("\\.",-1);
        if(parts.length!=4)return false;
        int[] oct=new int[4];
        for(int i=0;i<4;i++){
            if(!parts[i].matches("0|[1-9][0-9]{0,2}"))return false;
            oct[i]=Integer.parseInt(parts[i]);if(oct[i]>255)return false;
        }
        return oct[3]!=0&&oct[3]!=255&&(oct[0]==10||(oct[0]==172&&oct[1]>=16&&oct[1]<=31)||(oct[0]==192&&oct[1]==168));
    }
    static boolean validKey(String s){return s!=null&&s.matches("[a-f0-9]{32}");}
    static String newKey(){
        byte[] random=new byte[16];new SecureRandom().nextBytes(random);
        StringBuilder result=new StringBuilder(32);
        for(byte b:random){result.append(Character.forDigit((b>>4)&15,16));result.append(Character.forDigit(b&15,16));}
        return result.toString();
    }
    static String catalog(String ip,String key,boolean tv,int offset,String query)throws Exception{
        byte[] bytes=query.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        if(bytes.length>64||offset<0||offset>999999)throw new IllegalArgumentException("Pesquisa longa demais");
        StringBuilder encoded=new StringBuilder();
        for(byte value:bytes){encoded.append(Character.forDigit((value>>4)&15,16));encoded.append(Character.forDigit(value&15,16));}
        return exchange(ip,key,(tv?"tv/":"games/")+offset+"/"+encoded,true);
    }
    static String art(String ip,String key,String id,boolean tv)throws Exception{
        if(!id.matches("[1-9][0-9]{0,19}"))throw new IllegalArgumentException("ID inválido");
        return exchange(ip,key,(tv?"logo/":"cover/")+id,true);
    }
    static String snapshot(String ip,String key)throws Exception{
        return exchange(ip,key,"snapshot",true);
    }
    static int request(String ip,String key,String command)throws Exception{
        return Integer.parseInt(exchange(ip,key,command,false));
    }
    private static String exchange(String ip,String key,String command,boolean body)throws Exception{
        if(!privateIp(ip))throw new IllegalArgumentException("IPv4 privado obrigatório");
        boolean probe=command==null;
        if(!probe&&(!validKey(key)||!command.matches("up|down|left|right|accept|back|mute|quality|quieter|louder|channels|guide|favorite|refresh|pageup|pagedown|status|snapshot|(games|tv)/[0-9]{1,6}/[a-f0-9]{0,128}|(tune|logo|cover)/[1-9][0-9]{0,19}")))throw new IllegalArgumentException("Comando ou chave inválida");
        HttpURLConnection c=(HttpURLConnection)new URL("http://"+ip+":"+PORT+(probe?"/":"/cmd/"+command)).openConnection(Proxy.NO_PROXY);
        try{
            c.setInstanceFollowRedirects(false);c.setConnectTimeout(2500);c.setReadTimeout(3500);c.setUseCaches(false);c.setRequestProperty("Connection","close");
            if(!probe){c.setRequestMethod("POST");c.setRequestProperty("X-Infinity-Key",key);c.setDoOutput(true);c.setFixedLengthStreamingMode(0);c.getOutputStream().close();}
            int code=c.getResponseCode();
            if(!body)return Integer.toString(code);
            if(code!=200)throw new java.io.IOException("HTTP "+code);
            if(c.getContentType()==null||!c.getContentType().startsWith("application/json"))throw new java.io.IOException("Resposta incompatível");
            try(java.io.InputStream in=c.getInputStream();java.io.ByteArrayOutputStream bytes=new java.io.ByteArrayOutputStream()){
                byte[] buffer=new byte[1024];int n;
                while((n=in.read(buffer))!=-1){if(bytes.size()+n>8192)throw new java.io.IOException("Resposta grande demais");bytes.write(buffer,0,n);}
                return new String(bytes.toByteArray(),java.nio.charset.StandardCharsets.UTF_8);
            }
        }finally{c.disconnect();}
    }
}
