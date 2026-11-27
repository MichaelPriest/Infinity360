package com.infinity360.remote;
public final class XboxFtpClientTest {
    private static void require(boolean ok){if(!ok)throw new AssertionError();}
    public static void main(String[] args)throws Exception{
        require(XboxFtpClient.validName("save 01.zip"));
        require(!XboxFtpClient.validName("../x")&&!XboxFtpClient.validName("AXCOPY-FTP.part")&&!XboxFtpClient.validName("bad\r\nSTOR"));
        require(XboxFtpClient.validPath("/Games/Save")&&!XboxFtpClient.validPath("/../x"));
        require(XboxFtpClient.child("/","test.bin").equals("/test.bin"));
        require(XboxFtpClient.passivePort("229 Entering Extended Passive Mode (|||12345|)")==12345);
        try{XboxFtpClient.passivePort("229 Entering Extended Passive Mode (|||65536|)");throw new AssertionError();}catch(java.io.IOException expected){}
        require(XboxFtpClient.parseListing("dr-xr-xr-x 1 owner owner 0 Jan 01 2000 Saves\r\n-r--r--r-- 1 owner owner 123 Jan 01 2000 Test.bin\r\n").size()==2);
        require(XboxFtpClient.parseListing("").isEmpty());
        try{XboxFtpClient.parseListing("-r--r--r-- 1 owner owner 2 Jan 01 2000 ../x\r\n");throw new AssertionError();}catch(java.io.IOException expected){}
        require(XboxFtpClient.credentials("infinity","long-password")&&!XboxFtpClient.credentials("anonymous","long-password"));
        require(XboxFtpClient.parseUploadLimit("213 0")==0);
        require(XboxFtpClient.parseUploadLimit("213 33554432")==33554432L);
        require(XboxFtpClient.parseUploadLimit("213 536870912")==536870912L);
        try{XboxFtpClient.parseUploadLimit("213 536870913");throw new AssertionError();}catch(java.io.IOException expected){}
        require(XboxFtpClient.child("/Batch","Save").equals("/Batch/Save"));
        require(!XboxFtpClient.validPath("/Batch/../Escape"));
        try{XboxFtpClient.parseListing("-r--r--r-- 1 owner owner 1 Jan 01 2000 Save.bin\r\n-r--r--r-- 1 owner owner 1 Jan 01 2000 save.bin\r\n");throw new AssertionError();}catch(java.io.IOException expected){}
        require(!XboxFtpClient.validPath("/a//b")&&!XboxFtpClient.validPath("/a/./b"));
        System.out.println("FTP parser and bounds OK");
    }
}
