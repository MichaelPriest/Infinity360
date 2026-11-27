package com.infinity360.remote;

import android.app.*;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.provider.OpenableColumns;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import java.util.HashSet;
import java.util.Locale;
import android.text.InputType;
import android.view.View;
import android.widget.*;
import java.io.*;
import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** User-selected Android files only. No broad storage permission. */
public final class FtpActivity extends Activity {
    private static final int ACCENT=NativeUi.EMPHASIS,TEXT=NativeUi.NORMAL,MUTED=NativeUi.SECONDARY;
    private static final int PICK_UPLOAD=701,PICK_DOWNLOAD=702,PICK_UPLOAD_TREE=703,PICK_DOWNLOAD_TREE=704;
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
    private static final int MAX_BATCH=32,MAX_TREE_NODES=256,MAX_TREE_DEPTH=8;
    private static final long MAX_TREE_BYTES=16L*1024*1024*1024;
    private final ArrayList<XboxFtpClient.Entry> selected=new ArrayList<>();
    private ArrayList<XboxFtpClient.Entry> pendingTreeDownload=new ArrayList<>();
    private String pendingTreePath="/";
    private File resumeFile;
    private String resumePath="",resumeStamp="",resumeHost="",resumeUser="";
    private Uri resumeDestination;
    private long resumeSize;
    private String jobHost,jobUser;
    private int dp(float x){return Math.round(x*getResources().getDisplayMetrics().density);}
    private TextView label(String value,int size,int role){TextView text=NativeUi.text(this,value,size,role);text.setPadding(0,dp(5),0,dp(5));return text;}
    private Button button(String value,Runnable action){return NativeUi.button(this,value,action);}
    private void addButton(LinearLayout parent,String value,Runnable action){LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.setMargins(0,dp(4),0,dp(4));parent.addView(button(value,action),lp);}
    private EditText field(String hint,int type){EditText field=NativeUi.field(this,hint,type);root.addView(field);return field;}
    @Override public void onCreate(Bundle saved){
        super.onCreate(saved);
        LinearLayout outer=new LinearLayout(this);outer.setOrientation(LinearLayout.VERTICAL);
        setContentView(outer);NativeUi.insets(outer);NativeUi.brand(this,outer,"Gerenciador FTP");
        ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);outer.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
        root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);root.setPadding(dp(16),dp(8),dp(16),dp(24));scroll.addView(root);
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
        addButton(root,"Enviar pasta inteira",()->pickTree(true));
        addButton(root,"Receber itens selecionados",()->pickTree(false));
        addButton(root,"Renomear item selecionado",()->renameSelected());
        addButton(root,"Retomar último download",()->resumeLast());
        addButton(root,"Descartar download parcial",()->{if(!busy){clearResume();status.setText("Parcial descartado.");}});
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
        jobHost=conn[0];jobUser=conn[1];cancelled=false;busy=true;bar.setProgress(0);status.setText(message);lastProgress=0;
        worker.execute(()->{
            String result;
            XboxFtpClient client=new XboxFtpClient(conn[0]);active=client;
            try{
                // Server permits only one client and enforces a 2-second reconnect cooldown.
                long wait=2100-(SystemClock.elapsedRealtime()-lastFinish);if(wait>0)Thread.sleep(wait);
                check();client.connect(conn[1],conn[2]);check();result=job.run(client);
                client.quit();
            }catch(Exception e){result=cancelled?"Transferência cancelada. Confira o destino.":e instanceof IOException&&e.getMessage()!=null?e.getMessage():"Operação FTP não concluída.";}
            finally{client.close();active=null;lastFinish=SystemClock.elapsedRealtime();if(isDestroyed())clearResume();}
            final String report=result;
            runOnUiThread(()->{if(isDestroyed())return;busy=false;status.setText(report);if(cancelled)bar.setProgress(0);});
        });
    }
    private void browse(String path){
        if(!XboxFtpClient.validPath(path))return;
        runJob("Consultando pasta...",client->{
            ArrayList<XboxFtpClient.Entry> entries=client.list(path);check();
            runOnUiThread(()->{if(isDestroyed()||cancelled)return;directory=path;location.setText(path);rows.removeAllViews();selected.clear();
                if(entries.isEmpty())rows.addView(label("Pasta vazia.",14,MUTED));
                for(XboxFtpClient.Entry entry:entries){
                    String title=(entry.directory?"[Pasta] ":"[Arquivo] ")+entry.name+(entry.directory?"":" • "+android.text.format.Formatter.formatFileSize(this,entry.size));
                    LinearLayout line=new LinearLayout(this);line.setOrientation(LinearLayout.HORIZONTAL);
                    CheckBox choose=new CheckBox(this);choose.setContentDescription("Selecionar "+entry.name);choose.setOnCheckedChangeListener((view,checked)->{
                        if(checked){if(selected.size()>=MAX_BATCH){view.setChecked(false);status.setText("Selecione até 32 itens.");return;}selected.add(entry);}else selected.remove(entry);
                    });line.addView(choose,new LinearLayout.LayoutParams(dp(48),dp(50)));
                    line.addView(button(title,()->{if(busy)return;try{String target=XboxFtpClient.child(directory,entry.name);if(entry.directory)browse(target);else pickDownload(target,entry);}catch(IllegalArgumentException e){status.setText("Caminho inválido ou muito longo.");}}),new LinearLayout.LayoutParams(0,-2,1));
                    rows.addView(line);
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
        if(request==PICK_UPLOAD_TREE||request==PICK_DOWNLOAD_TREE){
            Uri tree=data.getData();if(tree==null||!DocumentsContract.isTreeUri(tree)){status.setText("Pasta inválida.");return;}
            if(request==PICK_UPLOAD_TREE)uploadTree(tree,pendingTreePath);
            else downloadTree(tree,pendingTreePath,new ArrayList<>(pendingTreeDownload));
            pendingTreeDownload.clear();return;
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
                clearResume();
                long size=client.size(target);if(size!=listedSize)throw new IOException("Arquivo alterado. Atualize a listagem.");
                if(size>getCacheDir().getUsableSpace())throw new IOException("Espaço temporário insuficiente no celular.");
                String stamp=client.stamp(target);
                resumeFile=File.createTempFile("ftp-download-",".part",getCacheDir());
                resumePath=target;resumeStamp=stamp;resumeSize=size;resumeDestination=uri;resumeHost=jobHost;resumeUser=jobUser;
                return receiveSingle(client);
            });
        }
    }
    private void renameSelected(){
        if(busy||connection()==null)return;
        if(selected.size()!=1){status.setText("Marque exatamente um arquivo ou pasta.");return;}
        final String folder=directory,old=selected.get(0).name;
        EditText name=new EditText(this);name.setSingleLine(true);name.setText(old);
        new AlertDialog.Builder(this).setTitle("Renomear "+old).setView(name).setNegativeButton("Cancelar",null)
            .setPositiveButton("Renomear",(d,w)->{
                final String target;try{target=XboxFtpClient.child(folder,name.getText().toString());}catch(IllegalArgumentException e){status.setText("Nome inválido.");return;}
                runJob("Renomeando...",client->{client.rename(XboxFtpClient.child(folder,old),target);return "Renomeado. Atualize a pasta.";});
            }).show();
    }
    private void clearResume(){
        if(resumeFile!=null)resumeFile.delete();
        resumeFile=null;resumePath="";resumeStamp="";resumeDestination=null;resumeHost="";resumeUser="";resumeSize=0;
    }
    private void resumeLast(){
        if(busy)return;
        if(resumeFile==null||!resumeFile.isFile()||resumeStamp.isEmpty()){status.setText("Nenhum parcial retomável nesta sessão.");return;}
        String[] conn=connection();if(conn==null)return;
        if(!resumeHost.equals(conn[0])||!resumeUser.equals(conn[1])){status.setText("Use o mesmo console e usuário.");return;}
        new AlertDialog.Builder(this).setTitle("Retomar "+resumePath+"?")
            .setMessage("O Xbox verificará tamanho, data de modificação e raiz. Não retome se o arquivo original foi alterado.")
            .setNegativeButton("Cancelar",null).setPositiveButton("Retomar",(d,w)->runJob("Retomando...",client->{
                if(!resumeHost.equals(jobHost)||!resumeUser.equals(jobUser))throw new IOException("Conexão alterada.");
                return receiveSingle(client);
            })).show();
    }
    private String receiveSingle(XboxFtpClient client)throws IOException{
        boolean saved=false;
        try{
            long offset=resumeFile.length();
            if(offset>resumeSize||client.size(resumePath)!=resumeSize||
               (!resumeStamp.isEmpty()&&!resumeStamp.equals(client.stamp(resumePath)))){
                clearResume();throw new IOException("Arquivo ou destino FTP alterado; parcial descartado.");
            }
            if(resumeSize-offset>getCacheDir().getUsableSpace())throw new IOException("Espaço temporário insuficiente.");
            transferStarted=SystemClock.elapsedRealtime();
            try(OutputStream out=new FileOutputStream(resumeFile,true)){
                client.downloadFrom(resumePath,out,resumeSize,offset,this::progress);
            }
            if(!resumeStamp.isEmpty()&&!resumeStamp.equals(client.stamp(resumePath))){
                clearResume();throw new IOException("Arquivo alterado durante o download.");
            }
            check();
            try(InputStream in=new FileInputStream(resumeFile);OutputStream out=getContentResolver().openOutputStream(resumeDestination,"wt")){
                if(out==null)throw new IOException("Destino indisponível");
                byte[] buffer=new byte[32768];int n;long copied=0;
                while((n=in.read(buffer))!=-1){check();out.write(buffer,0,n);copied+=n;}
                if(copied!=resumeSize)throw new IOException("Cópia local incompleta.");out.flush();
            }
            saved=true;return "Download salvo no destino Android.";
        }finally{
            if(saved||resumeStamp.isEmpty())clearResume();
        }
    }
    private void pickTree(boolean upload){
        if(busy||connection()==null)return;
        if(!upload&&selected.isEmpty()){status.setText("Marque arquivos ou pastas da listagem.");return;}
        pendingTreePath=directory;pendingTreeDownload=new ArrayList<>(selected);
        Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|(upload?0:Intent.FLAG_GRANT_WRITE_URI_PERMISSION));
        try{startActivityForResult(intent,upload?PICK_UPLOAD_TREE:PICK_DOWNLOAD_TREE);}
        catch(Exception e){pendingTreeDownload.clear();status.setText("Seletor de pastas indisponível.");}
    }
    private static final class TreeNode{
        final String path,parent,name;final Uri source;final long bytes;final boolean folder;
        TreeNode(String path,String parent,String name,Uri source,long bytes,boolean folder){
            this.path=path;this.parent=parent;this.name=name;this.source=source;this.bytes=bytes;this.folder=folder;
        }
    }
    private static final class TreePlan{
        final ArrayList<TreeNode> nodes=new ArrayList<>();final HashSet<String> paths=new HashSet<>();
        long total;int files;
        void add(TreeNode node,int depth)throws IOException{
            if(depth>MAX_TREE_DEPTH||nodes.size()>=MAX_TREE_NODES||!paths.add(node.path.toLowerCase(Locale.ROOT)))throw new IOException("Árvore muito grande ou nomes duplicados.");
            if(!node.folder){if(node.bytes<0||node.bytes>MAX_TREE_BYTES-total)throw new IOException("Tamanho da árvore indisponível ou acima de 16 GiB.");total+=node.bytes;files++;}
            nodes.add(node);
        }
    }
    private Uri rootDocument(Uri tree){return DocumentsContract.buildDocumentUriUsingTree(tree,DocumentsContract.getTreeDocumentId(tree));}
    private Cursor children(Uri tree,Uri parent){
        return getContentResolver().query(DocumentsContract.buildChildDocumentsUriUsingTree(tree,DocumentsContract.getDocumentId(parent)),
            new String[]{Document.COLUMN_DOCUMENT_ID,Document.COLUMN_DISPLAY_NAME,Document.COLUMN_MIME_TYPE,Document.COLUMN_SIZE},null,null,null);
    }
    private String documentName(Uri doc)throws IOException{
        try(Cursor cursor=getContentResolver().query(doc,new String[]{Document.COLUMN_DISPLAY_NAME},null,null,null)){
            if(cursor!=null&&cursor.moveToFirst()&&XboxFtpClient.validName(cursor.getString(0)))return cursor.getString(0);
        }throw new IOException("Nome da pasta incompatível com Xbox.");
    }
    private void planUpload(Uri tree,Uri source,String parent,String name,int depth,TreePlan plan,HashSet<String> visited,long limit)throws IOException{
        check();String path=XboxFtpClient.child(parent,name);
        if(!visited.add(DocumentsContract.getDocumentId(source)))throw new IOException("Árvore Android contém referência repetida.");
        plan.add(new TreeNode(path,parent,name,source,0,true),depth);
        try(Cursor cursor=children(tree,source)){
            if(cursor==null)throw new IOException("Não foi possível listar a pasta Android.");
            while(cursor.moveToNext()){
                check();String id=cursor.getString(0),child=cursor.getString(1),mime=cursor.getString(2);
                if(id==null||mime==null||!XboxFtpClient.validName(child))throw new IOException("Nome ou tipo de arquivo incompatível.");
                Uri doc=DocumentsContract.buildDocumentUriUsingTree(tree,id);
                if(Document.MIME_TYPE_DIR.equals(mime))planUpload(tree,doc,path,child,depth+1,plan,visited,limit);
                else{
                    long bytes=cursor.isNull(3)?sourceSize(doc):cursor.getLong(3);
                    if(!visited.add(id)||bytes<0||bytes>limit)throw new IOException("Arquivo repetido, tamanho indisponível ou acima do limite: "+child);
                    plan.add(new TreeNode(XboxFtpClient.child(path,child),path,child,doc,bytes,false),depth+1);
                }
            }
        }
    }
    private void uploadTree(Uri tree,String target){
        new AlertDialog.Builder(this).setTitle("Enviar pasta inteira ao Xbox?")
            .setMessage("Será criada uma nova pasta em "+target+". Até 256 entradas, 8 níveis e 16 GiB por lote; limite do servidor por arquivo. Sem mesclar pastas ou sobrescrever arquivos.")
            .setNegativeButton("Cancelar",null).setPositiveButton("Enviar",(d,w)->runJob("Validando pasta...",client->{
                if(client.uploadLimit()==0)throw new IOException("Upload não habilitado.");
                Uri source=rootDocument(tree);TreePlan plan=new TreePlan();
                planUpload(tree,source,target,documentName(source),0,plan,new HashSet<>(),client.uploadLimit());
                final long total=plan.total;long completed=0;int count=0;transferStarted=SystemClock.elapsedRealtime();
                for(TreeNode node:plan.nodes){check();try{
                    if(node.folder)client.mkdir(node.path);
                    else{final long before=completed;
                        try(InputStream in=getContentResolver().openInputStream(node.source)){
                            if(in==null)throw new IOException("Arquivo indisponível.");
                            client.upload(node.path,in,node.bytes,(done,ignored)->progress(before+done,total));
                        }completed+=node.bytes;count++;
                    }
                }catch(IOException e){throw new IOException(count+" arquivos concluídos; pasta interrompida: "+e.getMessage());}}
                return count+" arquivos enviados com estrutura de pastas. Atualize a listagem.";
            })).show();
    }
    private void planDownload(XboxFtpClient client,String parent,XboxFtpClient.Entry entry,int depth,TreePlan plan)throws IOException{
        check();String path=XboxFtpClient.child(parent,entry.name);
        plan.add(new TreeNode(path,parent,entry.name,null,entry.size,entry.directory),depth);
        if(entry.directory)for(XboxFtpClient.Entry child:client.list(path))planDownload(client,path,child,depth+1,plan);
    }
    private Uri createExclusive(Uri tree,Uri parent,String mime,String name)throws IOException{
        check();
        try(Cursor cursor=children(tree,parent)){
            if(cursor==null)throw new IOException("Destino Android indisponível.");
            while(cursor.moveToNext())if(name.equalsIgnoreCase(cursor.getString(1)))throw new IOException("Destino já existe: "+name);
        }
        Uri created=DocumentsContract.createDocument(getContentResolver(),parent,mime,name);
        if(created==null)throw new IOException("Não foi possível criar "+name);
        if(!name.equals(documentName(created))){
            try{DocumentsContract.deleteDocument(getContentResolver(),created);}catch(Exception ignored){}
            throw new IOException("Provedor alterou o nome de destino.");
        }
        return created;
    }
    private void downloadTree(Uri tree,String folder,ArrayList<XboxFtpClient.Entry> items){
        if(items.isEmpty())return;
        new AlertDialog.Builder(this).setTitle("Receber "+items.size()+" itens do Xbox?")
            .setMessage("Preserva subpastas e nomes. Não mescla pastas existentes. Até 256 entradas e 16 GiB por lote. Usa cache de um arquivo por vez.")
            .setNegativeButton("Cancelar",null).setPositiveButton("Receber",(d,w)->runJob("Validando árvore do Xbox...",client->{
                TreePlan plan=new TreePlan();for(XboxFtpClient.Entry entry:items)planDownload(client,folder,entry,0,plan);
                java.util.HashMap<String,Uri> parents=new java.util.HashMap<>();parents.put(folder,rootDocument(tree));
                final long total=plan.total;long completed=0;int count=0;transferStarted=SystemClock.elapsedRealtime();
                for(TreeNode node:plan.nodes){
                    check();Uri parent=parents.get(node.parent);if(parent==null)throw new IOException("Pasta de destino ausente.");
                    try{
                        if(node.folder){parents.put(node.path,createExclusive(tree,parent,Document.MIME_TYPE_DIR,node.name));continue;}
                        if(client.size(node.path)!=node.bytes)throw new IOException("Arquivo remoto alterado.");
                        if(node.bytes>getCacheDir().getUsableSpace())throw new IOException("Cache Android insuficiente.");
                        File temp=File.createTempFile("ftp-tree-",".part",getCacheDir());Uri created=null;boolean saved=false;
                        try{
                            final long before=completed;
                            try(OutputStream out=new FileOutputStream(temp)){client.download(node.path,out,node.bytes,(done,ignored)->progress(before+done,total));}
                            check();created=createExclusive(tree,parent,"application/octet-stream",node.name);
                            try(InputStream in=new FileInputStream(temp);OutputStream out=getContentResolver().openOutputStream(created,"wt")){
                                if(out==null)throw new IOException("Destino indisponível.");
                                byte[] buffer=new byte[32768];int n;long copied=0;
                                while((n=in.read(buffer))!=-1){check();out.write(buffer,0,n);copied+=n;}
                                if(copied!=node.bytes)throw new IOException("Cópia local incompleta.");out.flush();
                            }
                            saved=true;completed+=node.bytes;count++;
                        }finally{temp.delete();if(!saved&&created!=null)try{DocumentsContract.deleteDocument(getContentResolver(),created);}catch(Exception ignored){}}
                    }catch(IOException e){throw new IOException(count+" arquivos salvos; lote interrompido: "+e.getMessage());}
                }
                return count+" arquivos recebidos. Estrutura de pastas preservada.";
            })).show();
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
    @Override protected void onDestroy(){cancelTransfer();if(!busy)clearResume();worker.shutdownNow();super.onDestroy();}
}
