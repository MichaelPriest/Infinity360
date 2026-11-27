package com.infinity360.remote;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.View;
import java.io.File;
import java.io.FileOutputStream;

/** Actual APK rendering in an isolated emulator; no pairing or simulated Xbox data. */
public final class GalleryRunner extends Instrumentation {
    @Override public void onCreate(Bundle args) { super.onCreate(args); start(); }

    private void click(Activity activity,int id) {
        runOnMainSync(()->{
            View view=activity.findViewById(id);
            if(view==null||!view.isShown()||!view.performClick())throw new AssertionError("Cannot click resource "+id);
        });
        waitForIdleSync();
    }
    private void visible(Activity activity,int id) {
        runOnMainSync(()->{
            View view=activity.findViewById(id);
            if(view==null||!view.isShown())throw new AssertionError("Section not visible: "+id);
        });
    }
    private void shot(String name) throws Exception {
        waitForIdleSync(); android.os.SystemClock.sleep(700);
        Bitmap image=getUiAutomation().takeScreenshot();
        if(image==null)throw new IllegalStateException("No Android screenshot");
        File directory=new File(getTargetContext().getFilesDir(),"gallery");
        if(!directory.isDirectory()&&!directory.mkdirs())throw new IllegalStateException("Cannot create gallery");
        try(FileOutputStream output=new FileOutputStream(new File(directory,name))){
            if(!image.compress(Bitmap.CompressFormat.PNG,100,output))throw new IllegalStateException("PNG encoding failed");
        }finally{image.recycle();}
    }
    @Override public void onStart(){
        Bundle result=new Bundle();
        try {
            Intent launch=new Intent(getTargetContext(),MainActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            Activity main=startActivitySync(launch);
            visible(main,R.id.screen_home);shot("01-home.png");
            click(main,R.id.nav_control);visible(main,R.id.screen_control);shot("02-controle.png");
            click(main,R.id.nav_catalog);visible(main,R.id.screen_catalog);shot("03-catalogo.png");
            click(main,R.id.nav_store);visible(main,R.id.screen_store);shot("04-loja.png");
            click(main,R.id.nav_system);visible(main,R.id.screen_system);shot("05-sistema.png");
            click(main,R.id.remote_connection_button);visible(main,R.id.screen_connection);shot("06-conexao.png");
            click(main,R.id.nav_system);visible(main,R.id.screen_system);
            ActivityMonitor monitor=addMonitor(FtpActivity.class.getName(),null,false);
            click(main,R.id.remote_ftp_button);
            Activity ftp=waitForMonitorWithTimeout(monitor,10000);
            if(ftp==null)throw new AssertionError("FTP did not open");
            shot("07-ftp.png");
            runOnMainSync(()->{ftp.finish();main.finish();});
            result.putString("stream","\nGALLERY_OK\n");
            finish(Activity.RESULT_OK,result);
        }catch(Throwable error){
            android.util.Log.e("InfinityGallery","Screenshot failure",error);
            result.putString("stream",android.util.Log.getStackTraceString(error));
            finish(Activity.RESULT_CANCELED,result);
        }
    }
}
