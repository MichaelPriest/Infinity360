package com.infinity360.remote;

import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Client for the console's bounded passive FTP service, not a general FTP proxy. */
final class XboxFtpClient implements Closeable {
    static final int PORT=2121;
    static final long UPLOAD_LIMIT=32L*1024*1024, MAX_UPLOAD=512L*1024*1024;
    private long serverUploadLimit=UPLOAD_LIMIT;
    static final int LIST_LIMIT=65536;
    static final class Entry {
        final String name; final boolean directory; final long size;
        Entry(String name,boolean directory,long size){this.name=name;this.directory=directory;this.size=size;}
    }
    interface Progress { void update(long done,long total)throws IOException; }
    private final String ip;
    private volatile Socket control,data;
    private volatile boolean cancelled;
    private InputStream input;
    private OutputStream output;
    XboxFtpClient(String ip){if(!RemoteProtocol.privateIp(ip))throw new IllegalArgumentException("IPv4 privado obrigatório");this.ip=ip;}
    static boolean validName(String name){
        return name!=null&&name.matches("[A-Za-z0-9._ -]{1,42}")&&!name.startsWith(" ")&&!name.endsWith(" ")&&!name.endsWith(".")&&!name.toUpperCase(java.util.Locale.ROOT).startsWith("AXCOPY-");
    }
    static boolean validPath(String path){
        if(path==null||!path.startsWith("/")||path.length()>200)return false;
        if(path.equals("/"))return true;
        for(String part:path.substring(1).split("/",-1))if(!validName(part))return false;
        return true;
    }
    static String child(String path,String name){
        if(!validPath(path)||!validName(name))throw new IllegalArgumentException("Caminho inválido");
        String result=(path.equals("/")?"":path)+"/"+name;
        if(!validPath(result))throw new IllegalArgumentException("Caminho longo demais");
        return result;
    }
    static boolean credentials(String user,String password){
        return user!=null&&user.matches("[A-Za-z0-9_-]{1,32}")&&!user.equalsIgnoreCase("anonymous")&&password!=null&&password.matches("[ -~]{12,64}")&&!user.equals(password);
    }
    private void check()throws IOException{if(cancelled||Thread.currentThread().isInterrupted())throw new InterruptedIOException("Transferência cancelada");}
    private Socket open(int port,boolean isControl)throws IOException{
        check();Socket socket=new Socket();
        if(isControl)control=socket;else data=socket;
        try{check();socket.connect(new InetSocketAddress(ip,port),5000);socket.setSoTimeout(12000);check();return socket;}
        catch(IOException e){socket.close();throw e;}
    }
    void connect(String user,String password)throws IOException{
        if(!credentials(user,password))throw new IOException("Credenciais FTP inválidas");
        Socket socket=open(PORT,true);input=socket.getInputStream();output=socket.getOutputStream();
        expect(readReply(),220);expect(command("USER "+user),331);expect(command("PASS "+password),230);expect(command("TYPE I"),200);
        String reply=command("XMAX");
        if(reply.startsWith("213 "))serverUploadLimit=parseUploadLimit(reply);
        else if(!reply.startsWith("502 ")&&!reply.startsWith("500 "))throw new IOException("Limite FTP indisponível");
    }
    long uploadLimit(){return serverUploadLimit;}
    static long parseUploadLimit(String reply)throws IOException{
        if(!reply.matches("213 [0-9]{1,9}"))throw new IOException("Limite FTP inválido");
        long limit=Long.parseLong(reply.substring(4));
        if(limit>MAX_UPLOAD)throw new IOException("Limite FTP excede capacidade do app");
        return limit;
    }
    private String readReply()throws IOException{
        ByteArrayOutputStream line=new ByteArrayOutputStream();
        while(line.size()<1024){check();int c=input.read();if(c==-1)throw new EOFException("Conexão FTP encerrada");
            if(c=='\n'){String result=new String(line.toByteArray(),StandardCharsets.US_ASCII);if(!result.endsWith("\r")||!result.matches("[0-9]{3} [ -~]*\r"))throw new IOException("Resposta FTP inválida");return result.substring(0,result.length()-1);}
            line.write(c);
        }
        throw new IOException("Resposta FTP longa demais");
    }
    private String command(String text)throws IOException{
        check();if(text.length()>512||!text.matches("[ -~]+"))throw new IOException("Comando inválido");
        output.write((text+"\r\n").getBytes(StandardCharsets.US_ASCII));output.flush();return readReply();
    }
    private static void expect(String reply,int code)throws IOException{
        if(!reply.startsWith(code+" "))throw new IOException("FTP recusou a operação ("+reply.substring(0,3)+")");
    }
    static int passivePort(String reply)throws IOException{
        Matcher match=Pattern.compile("^229 [ -~]*\\(\\|\\|\\|([0-9]{1,5})\\|\\)$").matcher(reply);
        if(!match.matches())throw new IOException("Resposta EPSV inválida");
        int port=Integer.parseInt(match.group(1));if(port<1||port>65535)throw new IOException("Porta inválida");return port;
    }
    private Socket passive()throws IOException{return open(passivePort(command("EPSV")),false);}
    private void cwd(String path)throws IOException{if(!validPath(path))throw new IOException("Pasta inválida");expect(command("CWD "+path),250);}
    static ArrayList<Entry> parseListing(String listing)throws IOException{
        ArrayList<Entry> rows=new ArrayList<>();
        if(listing.isEmpty())return rows;
        if(!listing.endsWith("\r\n"))throw new IOException("Lista truncada");
        Pattern format=Pattern.compile("^([d-])r-x?r-x?r-x?r?-? 1 owner owner ([0-9]{1,19}) Jan 01 2000 (.+)$");
        // Exact listing format emitted by the INFINITY FTP server.
        format=Pattern.compile("^(dr-xr-xr-x|-r--r--r--) 1 owner owner ([0-9]{1,19}) Jan 01 2000 (.+)$");
        for(String line:listing.substring(0,listing.length()-2).split("\r\n",-1)){
            Matcher match=format.matcher(line);if(!match.matches()||!validName(match.group(3))||rows.size()>=512)throw new IOException("Listagem incompatível");
            long size;try{size=Long.parseLong(match.group(2));}catch(NumberFormatException e){throw new IOException("Tamanho inválido");}
            rows.add(new Entry(match.group(3),match.group(1).startsWith("d"),size));
        }
        rows.sort((left,right)->left.directory==right.directory?left.name.compareToIgnoreCase(right.name):left.directory?-1:1);
        return rows;
    }
    ArrayList<Entry> list(String path)throws IOException{
        cwd(path);ByteArrayOutputStream bytes=new ByteArrayOutputStream();
        try(Socket channel=passive()){
            expect(command("LIST"),150);
            InputStream in=channel.getInputStream();byte[] buffer=new byte[4096];int n;
            while((n=in.read(buffer))!=-1){check();if(bytes.size()+n>LIST_LIMIT)throw new IOException("Lista excede limite");bytes.write(buffer,0,n);}
        }finally{data=null;}
        expect(readReply(),226);return parseListing(new String(bytes.toByteArray(),StandardCharsets.US_ASCII));
    }
    void mkdir(String path)throws IOException{
        if(!validPath(path)||path.equals("/")||serverUploadLimit==0)throw new IOException("Criação de pasta indisponível");
        expect(command("MKD "+path),257);
    }
    long size(String path)throws IOException{
        if(!validPath(path)||path.equals("/"))throw new IOException("Arquivo inválido");
        String reply=command("SIZE "+path);expect(reply,213);
        try{String value=reply.substring(4);if(!value.matches("[0-9]{1,19}"))throw new NumberFormatException();return Long.parseLong(value);}
        catch(NumberFormatException e){throw new IOException("Tamanho inválido");}
    }
    void download(String path,OutputStream destination,long expected,Progress progress)throws IOException{
        if(!validPath(path)||path.equals("/")||expected<0)throw new IOException("Arquivo inválido");
        long done=0;progress.update(0,expected);
        try(Socket channel=passive()){
            expect(command("RETR "+path),150);
            InputStream in=channel.getInputStream();byte[] buffer=new byte[32768];int n;
            while((n=in.read(buffer))!=-1){check();if(n>expected-done)throw new IOException("Tamanho remoto mudou");destination.write(buffer,0,n);done+=n;progress.update(done,expected);}
        }finally{data=null;}
        expect(readReply(),226);if(done!=expected)throw new IOException("Download incompleto");destination.flush();
    }
    void upload(String path,InputStream source,long expected,Progress progress)throws IOException{
        if(!validPath(path)||path.equals("/")||expected<0||serverUploadLimit==0||expected>serverUploadLimit)throw new IOException("Upload desabilitado ou acima do limite do servidor");
        expect(command("ALLO "+expected),200);long done=0;progress.update(0,expected);
        try(Socket channel=passive()){
            expect(command("STOR "+path),150);
            OutputStream out=channel.getOutputStream();byte[] buffer=new byte[32768];int n;
            while((n=source.read(buffer))!=-1){check();if(n>expected-done)throw new IOException("Tamanho local mudou");out.write(buffer,0,n);done+=n;progress.update(done,expected);}
            if(done!=expected)throw new IOException("Upload incompleto");
            out.flush();channel.shutdownOutput();
        }finally{data=null;}
        expect(readReply(),226);
    }
    void quit(){try{if(!cancelled&&output!=null)command("QUIT");}catch(IOException ignored){}finally{close();}}
    @Override public void close(){cancelled=true;Socket d=data,c=control;try{if(d!=null)d.close();}catch(IOException ignored){}try{if(c!=null)c.close();}catch(IOException ignored){}}
}
