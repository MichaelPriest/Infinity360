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
    private long lastProgress,transferStarted;
    private static final int MAX_BATCH=32;
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
        addButton(root,"Enviar arquivos do celular",()->pickUpload());
        addButton(root,"Criar pasta no Xbox",()->createFolder());
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
        final double elapsed=Math.max(0.001,(now-transferStarted)/1000.0);
        final long speed=(long)(done/elapsed);
        final String rate=speed>0?android.text.format.Formatter.formatFileSize(this,speed)+"/s":"--";
        final String eta=speed>0?Math.max(0,(total-done)/speed)+" s restantes":"--";
        int percent=total>0?(int)Math.min(100,100.0*done/total):0;
        runOnUiThread(()->{if(!isDestroyed()&&busy&&!cancelled){bar.setProgress(percent);status.setText(android.text.format.Formatter.formatFileSize(this,done)+" / "+android.text.format.Formatter.formatFileSize(this,total)+" • "+rate+" • "+eta);}});
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
            });return entries.size()+" itens. "+(client.uploadLimit()==0?"Somente leitura.":"Limite de envio: "+android.text.format.Formatter.formatFileSize(this,client.uploadLimit())+".");
        });
    }
    private void createFolder(){
        if(busy||connection()==null)return;
        final String folder=directory;
        EditText name=new EditText(this);name.setSingleLine(true);name.setHint("Nome da pasta");
        new AlertDialog.Builder(this).setTitle("Criar pasta em "+folder).setView(name).setNegativeButton("Cancelar",null).setPositiveButton("Criar",(d,w)->{
            String target;try{target=XboxFtpClient.child(folder,name.getText().toString());}catch(IllegalArgumentException e){status.setText("Nome inválido.");return;}
            runJob("Criando pasta...",client->{client.mkdir(target);return "Pasta criada. Atualize a listagem.";});
        }).show();
    }
    private void pickUpload(){
        if(busy||connection()==null)return;
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_ALLOW_MULTIPLE,true);
        try{startActivityForResult(intent,PICK_UPLOAD);}catch(Exception e){status.setText("Seletor de arquivos indisponível.");}
    }
    private void pickDownload(String path,XboxFtpClient.Entry entry){
        if(busy||connection()==null)return;
        pendingDownload=path;pendingName=entry.name;pendingSize=entry.size;
        Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/octet-stream").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_TITLE,entry.name);
        try{startActivityForResult(intent,PICK_DOWNLOAD);}catch(Exception e){pendingDownload="";status.setText("Seletor de destino indisponível.");}
    }
    private long sourceSize(Uri uri){
        try(android.content.res.AssetFileDescriptor descriptor=getContentResolver().openAssetFileDescriptor(uri,"r")){
            if(descriptor!=null&&descriptor.getLength()>=0)return descriptor.getLength();
        }catch(Exception ignored){}
        try(Cursor cursor=getContentResolver().query(uri,new String[]{OpenableColumns.SIZE},null,null,null)){
            if(cursor!=null&&cursor.moveToFirst()&&!cursor.isNull(0))return cursor.getLong(0);
        }catch(Exception ignored){}
        return -1;
    }
    private String filename(Uri uri){
        try(Cursor c=getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)){
            if(c!=null&&c.moveToFirst())return c.getString(0);
        }catch(Exception ignored){}return "arquivo.bin";
    }
    @Override protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);
        if(result!=RESULT_OK||data==null){pendingDownload="";return;}
        if(request==PICK_UPLOAD&&data.getClipData()!=null&&data.getClipData().getItemCount()>1){
            final android.content.ClipData clip=data.getClipData();
            if(clip.getItemCount()>MAX_BATCH){status.setText("Selecione até 32 arquivos por lote.");return;}
            final ArrayList<Uri> files=new ArrayList<>();
            for(int i=0;i<clip.getItemCount();i++){Uri entry=clip.getItemAt(i).getUri();if(entry==null){status.setText("Seleção inválida.");return;}files.add(entry);}
            uploadBatch(files);return;
        }
        final Uri uri=data.getData()!=null?data.getData():data.getClipData()!=null&&data.getClipData().getItemCount()==1?data.getClipData().getItemAt(0).getUri():null;
        if(uri==null){pendingDownload="";return;}
        if(request==PICK_UPLOAD){
            EditText name=new EditText(this);name.setSingleLine(true);String original=filename(uri);name.setText(XboxFtpClient.validName(original)?original:"arquivo.bin");
            final String folder=directory;
            new AlertDialog.Builder(this).setTitle("Enviar para "+folder).setMessage("Nome no Xbox (até 42 caracteres ASCII). O limite depende do servidor (até 512 MiB); arquivos existentes não serão sobrescritos.")
                .setView(name).setNegativeButton("Cancelar",null).setPositiveButton("Enviar",(d,w)->{
                    String target;try{target=XboxFtpClient.child(folder,name.getText().toString());}catch(IllegalArgumentException e){status.setText("Nome inválido. Selecione novamente.");return;}
                    runJob("Preparando arquivo...",client->{
                        long count=sourceSize(uri);
                        if(count<0)throw new IOException("O provedor Android não informou o tamanho. Escolha um arquivo local.");
                        if(client.uploadLimit()==0||count>client.uploadLimit())throw new IOException("Envio desabilitado ou acima do limite do Xbox.");
                        try(InputStream in=getContentResolver().openInputStream(uri)){
                            if(in==null)throw new IOException("Arquivo local indisponível");
                            transferStarted=SystemClock.elapsedRealtime();client.upload(target,in,count,this::progress);
                        }
                        return "Upload concluído. Atualize a pasta do Xbox.";
                    });
                }).show();
        }else if(request==PICK_DOWNLOAD&&!pendingDownload.isEmpty()){
            final String target=pendingDownload,name=pendingName;final long listedSize=pendingSize;pendingDownload="";
            runJob("Recebendo "+name+"...",client->{
                long size=client.size(target);if(size!=listedSize)throw new IOException("Arquivo alterado. Atualize a listagem.");
                if(size>getCacheDir().getUsableSpace())throw new IOException("Espaço temporário insuficiente no celular.");
                File temp=File.createTempFile("ftp-download-",".part",getCacheDir());
                try{
                    try(OutputStream out=new FileOutputStream(temp)){transferStarted=SystemClock.elapsedRealtime();client.download(target,out,size,this::progress);}
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
    private void uploadBatch(ArrayList<Uri> files){
        final String folder=directory;
        new AlertDialog.Builder(this).setTitle("Enviar "+files.size()+" arquivos?")
            .setMessage("Destino: "+folder+". Nomes originais; sem sobrescrita. O lote para na primeira falha e preserva os arquivos concluídos.")
            .setNegativeButton("Cancelar",null).setPositiveButton("Enviar",(d,w)->runJob("Validando lote...",client->{
                ArrayList<String> names=new ArrayList<>();ArrayList<Long> sizes=new ArrayList<>();
                java.util.HashSet<String> seen=new java.util.HashSet<>();long total=0;
                for(Uri uri:files){check();String name=filename(uri);long size=sourceSize(uri);
                    if(!XboxFtpClient.validName(name)||!seen.add(name.toLowerCase(java.util.Locale.ROOT)))throw new IOException("Nome inválido ou duplicado: "+name);
                    if(size<0||client.uploadLimit()==0||size>client.uploadLimit())throw new IOException("Tamanho indisponível ou acima do limite: "+name);
                    names.add(XboxFtpClient.child(folder,name));sizes.add(size);total+=size;
                }
                final long all=total;
                long completed=0;int count=0;transferStarted=SystemClock.elapsedRealtime();
                for(int i=0;i<files.size();i++){
                    final long previous=completed;
                    try(InputStream in=getContentResolver().openInputStream(files.get(i))){
                        if(in==null)throw new IOException("Arquivo indisponível");
                        client.upload(names.get(i),in,sizes.get(i),(done,ignored)->progress(previous+done,all));
                    }catch(IOException e){throw new IOException(count+" arquivos concluídos; lote interrompido: "+e.getMessage());}
                    completed+=sizes.get(i);count++;
                }
                return count+" arquivos enviados. Atualize a pasta.";
            })).show();
    }
    private void cancelTransfer(){if(!busy)return;cancelled=true;XboxFtpClient client=active;if(client!=null)client.close();status.setText("Cancelamento solicitado...");}
    @Override protected void onStop(){if(busy)cancelTransfer();super.onStop();}
    @Override protected void onDestroy(){cancelTransfer();worker.shutdownNow();super.onDestroy();}
}
