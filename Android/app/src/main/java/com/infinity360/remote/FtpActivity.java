package com.infinity360.remote;

import android.app.*;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.provider.OpenableColumns;
import android.text.InputType;
import android.view.View;
import android.widget.*;
import java.io.*;
import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** User-selected Android files only. No broad storage permission. */
public final class FtpActivity extends Activity {
    private static final int BG=0xff101b15,PANEL=0xff20382b,ACCENT=0xffa8ff00,TEXT=0xffeff7f0,MUTED=0xffb1c8bb;
    private static final int PICK_UPLOAD=701,PICK_DOWNLOAD=702;
    private EditText ip,user,password;
    private TextView location,status;
    private ProgressBar bar;
    private LinearLayout rows,root;
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private volatile XboxFtpClient active;
    private volatile boolean cancelled;
    private boolean busy;
    private String directory="/",pendingDownload="",pendingName="";
    private long pendingSize,lastFinish;
    private long lastProgress;
    private int dp(float x){return Math.round(x*getResources().getDisplayMetrics().density);}
    private TextView label(String value,int size,int color){TextView text=new TextView(this);text.setText(value);text.setTextColor(color);text.setTextSize(size);text.setPadding(0,dp(5),0,dp(5));return text;}
    private Button button(String value,Runnable action){Button button=new Button(this);button.setAllCaps(false);button.setText(value);button.setTextColor(TEXT);button.setBackgroundColor(PANEL);button.setOnClickListener(v->action.run());return button;}
    private void addButton(LinearLayout parent,String value,Runnable action){LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,dp(50));lp.setMargins(0,dp(4),0,dp(4));parent.addView(button(value,action),lp);}
    private EditText field(String hint,int type){EditText field=new EditText(this);field.setSingleLine(true);field.setHint(hint);field.setTextColor(TEXT);field.setHintTextColor(MUTED);field.setInputType(type);field.setSaveEnabled(false);root.addView(field);return field;}
    @Override public void onCreate(Bundle saved){
        super.onCreate(saved);
        ScrollView scroll=new ScrollView(this);scroll.setBackgroundColor(BG);
        root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);root.setPadding(dp(18),dp(28),dp(18),dp(24));scroll.addView(root);setContentView(scroll);
        if(android.os.Build.VERSION.SDK_INT>=30)scroll.setOnApplyWindowInsetsListener((view,insets)->{
            android.graphics.Insets safe=insets.getInsets(android.view.WindowInsets.Type.systemBars()|android.view.WindowInsets.Type.displayCutout());
            root.setPadding(dp(18)+safe.left,dp(18)+safe.top,dp(18)+safe.right,dp(24)+safe.bottom);return insets;});
        root.addView(label("∞ INFINITY 360 • FTP",25,ACCENT));
        root.addView(label("FTP local sem criptografia. Use apenas Wi-Fi confiável. Ative o servidor em Sistema > Rede e FTP no Xbox. Para enviar arquivos, habilite upload.",14,MUTED));
        ip=field("IP do Xbox",InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_URI);
        ip.setText(getIntent().getStringExtra("console_ip"));
        user=field("Usuário FTP",InputType.TYPE_CLASS_TEXT);
        password=field("Senha FTP",InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_PASSWORD);
        addButton(root,"Conectar / atualizar",()->browse(directory));
        location=label("/",16,ACCENT);root.addView(location);
        addButton(root,"Voltar uma pasta",()->{if(!busy){int i=directory.lastIndexOf('/');browse(i<=0?"/":directory.substring(0,i));}});
        addButton(root,"Enviar arquivo do celular",()->pickUpload());
        addButton(root,"Cancelar transferência",()->cancelTransfer());
        bar=new ProgressBar(this,null,android.R.attr.progressBarStyleHorizontal);bar.setMax(100);root.addView(bar,new LinearLayout.LayoutParams(-1,dp(14)));
        status=label("Servidor FTP: porta 2121.",14,MUTED);status.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);root.addView(status);
        rows=new LinearLayout(this);rows.setOrientation(LinearLayout.VERTICAL);root.addView(rows);
        addButton(root,"Voltar ao Remote",()->{if(!busy)finish();else status.setText("Cancele a transferência primeiro.");});
    }
    private String[] connection(){
        String host=ip.getText().toString().trim(),name=user.getText().toString().trim(),secret=password.getText().toString();
        if(!RemoteProtocol.privateIp(host)||!XboxFtpClient.credentials(name,secret)){status.setText("Informe IP privado e credenciais FTP válidas (senha de 12 a 64 caracteres).");return null;}
        return new String[]{host,name,secret};
    }
    private void check()throws IOException{if(cancelled||Thread.currentThread().isInterrupted())throw new InterruptedIOException("Cancelado");}
    private void progress(long done,long total)throws IOException{
        check();long now=SystemClock.elapsedRealtime();if(done!=total&&now-lastProgress<250)return;lastProgress=now;
        int percent=total>0?(int)Math.min(100,100.0*done/total):0;
        runOnUiThread(()->{if(!isDestroyed()&&busy&&!cancelled){bar.setProgress(percent);status.setText(android.text.format.Formatter.formatFileSize(this,done)+" / "+android.text.format.Formatter.formatFileSize(this,total));}});
    }
    interface Job{String run(XboxFtpClient client)throws Exception;}
    private void runJob(String message,Job job){
        if(busy)return;String[] conn=connection();if(conn==null)return;
        cancelled=false;busy=true;bar.setProgress(0);status.setText(message);lastProgress=0;
        worker.execute(()->{
            String result;
            XboxFtpClient client=new XboxFtpClient(conn[0]);active=client;
            try{
                // Server permits only one client and enforces a 2-second reconnect cooldown.
                long wait=2100-(SystemClock.elapsedRealtime()-lastFinish);if(wait>0)Thread.sleep(wait);
                check();client.connect(conn[1],conn[2]);check();result=job.run(client);
                client.quit();
            }catch(Exception e){result=cancelled?"Transferência cancelada. Confira o destino.":e instanceof IOException&&e.getMessage()!=null?e.getMessage():"Operação FTP não concluída.";}
            finally{client.close();active=null;lastFinish=SystemClock.elapsedRealtime();}
            final String report=result;
            runOnUiThread(()->{if(isDestroyed())return;busy=false;status.setText(report);if(cancelled)bar.setProgress(0);});
        });
    }
    private void browse(String path){
        if(!XboxFtpClient.validPath(path))return;
        runJob("Consultando pasta...",client->{
            ArrayList<XboxFtpClient.Entry> entries=client.list(path);check();
            runOnUiThread(()->{if(isDestroyed()||cancelled)return;directory=path;location.setText(path);rows.removeAllViews();
                if(entries.isEmpty())rows.addView(label("Pasta vazia.",14,MUTED));
                for(XboxFtpClient.Entry entry:entries){
                    String title=(entry.directory?"[Pasta] ":"[Arquivo] ")+entry.name+(entry.directory?"":" • "+android.text.format.Formatter.formatFileSize(this,entry.size));
                    addButton(rows,title,()->{if(busy)return;try{String target=XboxFtpClient.child(directory,entry.name);if(entry.directory)browse(target);else pickDownload(target,entry);}catch(IllegalArgumentException e){status.setText("Caminho inválido ou muito longo.");}});
                }
            });return entries.size()+" itens. Conexão encerrada após consulta.";
        });
    }
    private void pickUpload(){
        if(busy||connection()==null)return;
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE);
        try{startActivityForResult(intent,PICK_UPLOAD);}catch(Exception e){status.setText("Seletor de arquivos indisponível.");}
    }
    private void pickDownload(String path,XboxFtpClient.Entry entry){
        if(busy||connection()==null)return;
        pendingDownload=path;pendingName=entry.name;pendingSize=entry.size;
        Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/octet-stream").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_TITLE,entry.name);
        try{startActivityForResult(intent,PICK_DOWNLOAD);}catch(Exception e){pendingDownload="";status.setText("Seletor de destino indisponível.");}
    }
    private String filename(Uri uri){
        try(Cursor c=getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)){
            if(c!=null&&c.moveToFirst())return c.getString(0);
        }catch(Exception ignored){}return "arquivo.bin";
    }
    @Override protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);
        if(result!=RESULT_OK||data==null||data.getData()==null){pendingDownload="";return;}
        final Uri uri=data.getData();
        if(request==PICK_UPLOAD){
            EditText name=new EditText(this);name.setSingleLine(true);String original=filename(uri);name.setText(XboxFtpClient.validName(original)?original:"arquivo.bin");
            final String folder=directory;
            new AlertDialog.Builder(this).setTitle("Enviar para "+folder).setMessage("Nome no Xbox (até 42 caracteres ASCII). Limite de 32 MiB; arquivos existentes não serão sobrescritos.")
                .setView(name).setNegativeButton("Cancelar",null).setPositiveButton("Enviar",(d,w)->{
                    String target;try{target=XboxFtpClient.child(folder,name.getText().toString());}catch(IllegalArgumentException e){status.setText("Nome inválido. Selecione novamente.");return;}
                    runJob("Preparando arquivo...",client->{
                        File temp=File.createTempFile("ftp-upload-",".part",getCacheDir());
                        try{
                            long count=0;
                            try(InputStream source=getContentResolver().openInputStream(uri);OutputStream out=new FileOutputStream(temp)){
                                if(source==null)throw new IOException("Arquivo local indisponível");byte[] buffer=new byte[32768];int n;
                                while((n=source.read(buffer))!=-1){check();if(n>XboxFtpClient.UPLOAD_LIMIT-count)throw new IOException("Arquivo excede 32 MiB");out.write(buffer,0,n);count+=n;}
                            }
                            try(InputStream in=new FileInputStream(temp)){client.upload(target,in,count,this::progress);}
                            return "Upload concluído. Atualize a pasta do Xbox.";
                        }finally{temp.delete();}
                    });
                }).show();
        }else if(request==PICK_DOWNLOAD&&!pendingDownload.isEmpty()){
            final String target=pendingDownload,name=pendingName;final long listedSize=pendingSize;pendingDownload="";
            runJob("Recebendo "+name+"...",client->{
                long size=client.size(target);if(size!=listedSize)throw new IOException("Arquivo alterado. Atualize a listagem.");
                if(size>getCacheDir().getUsableSpace())throw new IOException("Espaço temporário insuficiente no celular.");
                File temp=File.createTempFile("ftp-download-",".part",getCacheDir());
                try{
                    try(OutputStream out=new FileOutputStream(temp)){client.download(target,out,size,this::progress);}
                    check();
                    try(InputStream in=new FileInputStream(temp);OutputStream out=getContentResolver().openOutputStream(uri,"wt")){
                        if(out==null)throw new IOException("Destino indisponível");byte[] buffer=new byte[32768];int n;long copied=0;
                        while((n=in.read(buffer))!=-1){check();out.write(buffer,0,n);copied+=n;}
                        if(copied!=size)throw new IOException("Cópia local incompleta");out.flush();
                    }catch(IOException e){throw new IOException("Falha ao salvar no destino Android. Confira se ficou um arquivo incompleto.");}
                    return "Download salvo no destino Android.";
                }finally{temp.delete();}
            });
        }
    }
    private void cancelTransfer(){if(!busy)return;cancelled=true;XboxFtpClient client=active;if(client!=null)client.close();status.setText("Cancelamento solicitado...");}
    @Override protected void onStop(){if(busy)cancelTransfer();super.onStop();}
    @Override protected void onDestroy(){cancelTransfer();worker.shutdownNow();super.onDestroy();}
}
