package com.infinity360.remote;

import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.bottomnavigation.BottomNavigationView;
import com.google.android.material.materialswitch.MaterialSwitch;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Build;
import android.text.InputType;
import android.view.Gravity;
import android.view.View;
import android.view.WindowInsets;
import android.widget.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class MainActivity extends AppCompatActivity {
    private static final int PANEL=NativeUi.NORMAL, ACCENT=NativeUi.EMPHASIS, TEXT=NativeUi.NORMAL, MUTED=NativeUi.SECONDARY;
    private EditText consoleIp,phoneIp,key;
    private TextView status,monitor,catalogResult,downloads,storeResult;
    private LinearLayout downloadCards,deviceCards;
    private TextView deviceStatus;
    private EditText storeQuery;
    private LinearLayout storeCards;
    private int storeNext=-1;
    private String storeActiveQuery="";
    private LinearLayout catalogCards;
    private EditText catalogQuery;
    private boolean tvCatalog=false;
    private int catalogNext=-1;
    private String activeQuery="";
    private LinearLayout content;
    private final LinearLayout[] screens=new LinearLayout[6];
    private TextView homeConsole,homeMetrics;
    private boolean metricReady=false;
    private String metricHost="",metricKey="";
    private BottomNavigationView navigation;
    private ScrollView scroll;
    private int activeScreen=0;
    private boolean foreground=false,autoDownloads=false;
    private final android.os.Handler pollHandler=new android.os.Handler(android.os.Looper.getMainLooper());
    private final Runnable downloadPoll=new Runnable(){
        @Override public void run(){
            if(!foreground||!autoDownloads||activeScreen!=2)return;
            if(!busy&&RemoteProtocol.privateIp(consoleIp.getText().toString().trim())&&RemoteProtocol.validKey(key.getText().toString().trim()))refreshDownloads();
            pollHandler.postDelayed(this,5000);
        }
    };
    private void updateDownloadPolling(){
        pollHandler.removeCallbacks(downloadPoll);
        if(foreground&&autoDownloads&&activeScreen==2)pollHandler.postDelayed(downloadPoll,5000);
    }
    private void showScreen(int index){
        activeScreen=index;
        for(int i=0;i<screens.length;i++)if(screens[i]!=null)screens[i].setVisibility(i==index?View.VISIBLE:View.GONE);
        RemoteNavigation.select(navigation,index);
        if(index==5)updateHomeConsole();
        if(scroll!=null){scroll.scrollTo(0,0);scroll.post(()->scroll.scrollTo(0,0));}
        updateDownloadPolling();
    }
    private LinearLayout screen(LinearLayout root,int index){LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);root.addView(panel);screens[index]=panel;return panel;}
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private boolean busy=false;
    private int dp(float n){return Math.round(n*getResources().getDisplayMetrics().density);}
    private TextView text(String s,int size,int role){return NativeUi.text(this,s,size,role);}
    private void label(String s){TextView v=text(s,14,MUTED);v.setPadding(0,dp(12),0,dp(4));content.addView(v);}
    private EditText field(String hint,int type){EditText v=NativeUi.field(this,hint,type);content.addView(v,new LinearLayout.LayoutParams(-1,-2));return v;}
    private Button button(String label,int role,Runnable action){return NativeUi.button(this,label,role,action);}
    private void row(String[] labels,String[] commands){LinearLayout row=new LinearLayout(this);row.setOrientation(LinearLayout.HORIZONTAL);for(int i=0;i<labels.length;i++){final String cmd=commands[i];View b=cmd==null?new View(this):button(labels[i],cmd.equals("accept")?ACCENT:PANEL,()->send(cmd));LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,-2,1);lp.setMargins(dp(3),dp(3),dp(3),dp(3));row.addView(b,lp);}content.addView(row);}
    @Override public void onCreate(Bundle saved){
        RemoteNavigation.applyTheme(this);
        super.onCreate(saved);
        LinearLayout outer=new LinearLayout(this);outer.setOrientation(LinearLayout.VERTICAL);
        setContentView(outer);NativeUi.insets(outer);
        NativeUi.brand(this,outer,getString(R.string.remote_title));
        Button connection=NativeUi.button(this,getString(R.string.nav_connection),PANEL,()->showScreen(3));
        connection.setId(R.id.remote_connection_button);
        outer.addView(connection,new LinearLayout.LayoutParams(-1,-2));
        navigation=RemoteNavigation.create(this);
        scroll=new ScrollView(this);scroll.setFillViewport(true);outer.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
        outer.addView(navigation,new LinearLayout.LayoutParams(-1,-2));
        content=new LinearLayout(this);content.setOrientation(LinearLayout.VERTICAL);content.setPadding(dp(16),dp(8),dp(16),dp(24));scroll.addView(content);
        final LinearLayout root=content;
        status=text(getString(R.string.connect_hint),14,MUTED);status.setPadding(0,dp(10),0,dp(10));status.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);root.addView(status);
        content=screen(root,3);
        label(getString(R.string.xbox_ip));consoleIp=field("192.168.x.x",InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_URI);
        consoleIp.setText(getPreferences(MODE_PRIVATE).getString("console_ip",""));
        label(getString(R.string.pair_key));key=field(getString(R.string.hex_key),InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_PASSWORD);key.setFilters(new android.text.InputFilter[]{new android.text.InputFilter.LengthFilter(32)});
        content.addView(button(getString(R.string.verify_connection),PANEL,()->send("status")),new LinearLayout.LayoutParams(-1,-2));
        content=screen(root,2);
        label(getString(R.string.files_ftp));
        Button ftpButton=button(getString(R.string.ftp_title),ACCENT,this::openFtp);
        ftpButton.setId(R.id.remote_ftp_button);
        content.addView(ftpButton,new LinearLayout.LayoutParams(-1,-2));
        label(getString(R.string.monitor));
        monitor=text(getString(R.string.no_readings),15,TEXT);monitor.setPadding(dp(14),dp(14),dp(14),dp(14));content.addView(monitor);
        content.addView(button(getString(R.string.query_metrics),PANEL,()->refreshMonitor()),new LinearLayout.LayoutParams(-1,-2));
        label(getString(R.string.downloads));
        downloads=text(getString(R.string.no_query),14,TEXT);downloads.setPadding(dp(14),dp(14),dp(14),dp(14));content.addView(downloads);
        content.addView(button(getString(R.string.query_queue),PANEL,()->refreshDownloads()),new LinearLayout.LayoutParams(-1,-2));
        MaterialSwitch auto=new MaterialSwitch(this);auto.setText(getString(R.string.auto_refresh));auto.setChecked(false);
        auto.setOnCheckedChangeListener((view,enabled)->{autoDownloads=enabled;updateDownloadPolling();});
        content.addView(auto);
        content.addView(button(getString(R.string.start_queue),ACCENT,()->confirmQueueCommand("storestart","Iniciar ou retomar a fila do Xbox?","Os pacotes autorizados serão processados pelo downloader nativo. A fila para em caso de falha ou pausa.")),new LinearLayout.LayoutParams(-1,-2));
        content.addView(button(getString(R.string.pause_download),PANEL,()->confirmQueueCommand("storepause","Pausar o download atual?","O console tentará preservar o arquivo parcial para retomar depois.")),new LinearLayout.LayoutParams(-1,-2));
        downloadCards=new LinearLayout(this);downloadCards.setOrientation(LinearLayout.VERTICAL);content.addView(downloadCards);
        content=screen(root,1);
        label(getString(R.string.library));
        catalogQuery=field(getString(R.string.search_catalog),InputType.TYPE_CLASS_TEXT);
        content.addView(button(getString(R.string.query_games),PANEL,()->loadCatalog(false,true)),new LinearLayout.LayoutParams(-1,-2));
        content.addView(button(getString(R.string.query_channels),PANEL,()->loadCatalog(true,true)),new LinearLayout.LayoutParams(-1,-2));
        catalogResult=text(getString(R.string.no_query),15,TEXT);catalogResult.setPadding(dp(14),dp(14),dp(14),dp(14));content.addView(catalogResult);
        catalogCards=new LinearLayout(this);catalogCards.setOrientation(LinearLayout.VERTICAL);content.addView(catalogCards);
        content.addView(button(getString(R.string.next_page),PANEL,()->loadCatalog(tvCatalog,false)),new LinearLayout.LayoutParams(-1,-2));
        content=screen(root,4);
        label("INFINITY Store");
        deviceStatus=text("Destino: consulte os dispositivos do Xbox.",14,MUTED);content.addView(deviceStatus);
        content.addView(button(getString(R.string.query_disks),PANEL,()->loadDevices()),new LinearLayout.LayoutParams(-1,-2));
        deviceCards=new LinearLayout(this);deviceCards.setOrientation(LinearLayout.VERTICAL);content.addView(deviceCards);
        storeQuery=field("Buscar nome ou categoria (game, xbla, homebrew...)",InputType.TYPE_CLASS_TEXT);
        content.addView(button(getString(R.string.query_store),ACCENT,()->loadStore(true)),new LinearLayout.LayoutParams(-1,-2));
        storeResult=text("Catálogo autorizado do console. Nenhum pacote é baixado no celular.",14,MUTED);content.addView(storeResult);
        storeCards=new LinearLayout(this);storeCards.setOrientation(LinearLayout.VERTICAL);content.addView(storeCards);
        content.addView(button(getString(R.string.next_page),PANEL,()->loadStore(false)),new LinearLayout.LayoutParams(-1,-2));
        content.addView(button(getString(R.string.follow_downloads),PANEL,()->{showScreen(2);refreshDownloads();}),new LinearLayout.LayoutParams(-1,-2));
        content=screen(root,0);
        content=controlPanel(R.string.general_nav);
        row(new String[]{"Guide","↑","Atualizar"},new String[]{"guide","up","refresh"});
        row(new String[]{"←","A / OK","→"},new String[]{"left","accept","right"});
        row(new String[]{"LB","↓","RB"},new String[]{"pageup","down","pagedown"});
        row(new String[]{"Voltar","Favorito"},new String[]{"back","favorite"});
        content=screens[0];content=controlPanel(R.string.iptv_controls);
        row(new String[]{"Canais","↑","Qualidade"},new String[]{"channels","up","quality"});
        row(new String[]{"←","A / OK","→"},new String[]{"left","accept","right"});
        row(new String[]{"Vol −","↓","Vol +"},new String[]{"quieter","down","louder"});
        row(new String[]{"Mudo"},new String[]{"mute"});
        Button exit=button("Encerrar reprodução",PANEL,()->new MaterialAlertDialogBuilder(this).setTitle("Encerrar IPTV?").setMessage("A reprodução no Xbox será encerrada.").setNegativeButton(getString(R.string.cancel),null).setPositiveButton("Encerrar",(d,w)->send("back")).show());content.addView(exit,new LinearLayout.LayoutParams(-1,-2));
        content=screens[3];
        label(getString(R.string.preferences_title));
        content.addView(button(getString(R.string.home_theme),PANEL,()->{if(!busy)new MaterialAlertDialogBuilder(this).setTitle(R.string.home_theme).setItems(new String[]{getString(R.string.system_default),getString(R.string.theme_dark),getString(R.string.theme_light)},(d,w)->RemoteNavigation.theme(this,w)).show();}));
        content.addView(text(getString(R.string.language_note),13,MUTED));
        content.addView(button(getString(R.string.language),PANEL,()->{
            if(busy)return;
            new MaterialAlertDialogBuilder(this).setTitle(R.string.language)
                .setItems(new String[]{getString(R.string.system_default),"Português","English","Español"},(d,w)->RemoteNavigation.locale(w)).show();
        }),new LinearLayout.LayoutParams(-1,-2));
        label(getString(R.string.manual_pair));
        phoneIp=field(getString(R.string.phone_ip),InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_URI);
        content.addView(button(getString(R.string.find_phone),PANEL,()->findLocalIp()),new LinearLayout.LayoutParams(-1,-2));
        content.addView(button(getString(R.string.find_xbox),ACCENT,()->discoverConsole()),new LinearLayout.LayoutParams(-1,-2));
        content.addView(button(getString(R.string.export_pair),PANEL,()->exportPairing()),new LinearLayout.LayoutParams(-1,-2));
        content.addView(button(getString(R.string.new_key),PANEL,()->{key.setText(RemoteProtocol.newKey());status.setText("Nova chave gerada. Copie a configuração para o console.");}),new LinearLayout.LayoutParams(-1,-2));
        content.addView(button(getString(R.string.copy_config),PANEL,()->copyConfig()),new LinearLayout.LayoutParams(-1,-2));
        TextView note=text("Salve as duas linhas em cloud360-remote.txt na pasta de dados do Xbox. Para navegar na dashboard, crie enable-dashboard-remote.txt com ENABLE-DASHBOARD-REMOTE e reinicie a dashboard. HTTP local sem criptografia: use somente Wi-Fi confiável. A chave fica apenas na memória do aplicativo.",13,MUTED);note.setPadding(0,dp(12),0,0);content.addView(note);
        content=screen(root,5);buildHome();
        content=root;
        int initial=saved==null?5:saved.getInt("active_screen",5);
        showScreen(initial>=0&&initial<screens.length?initial:5);
        navigation.setOnItemSelectedListener(item->{int index=RemoteNavigation.index(item.getItemId());if(index<0)return false;if(index!=activeScreen)showScreen(index);return true;});
        getOnBackPressedDispatcher().addCallback(this,new androidx.activity.OnBackPressedCallback(true){
            @Override public void handleOnBackPressed(){if(activeScreen!=5)showScreen(5);else finish();}
        });
    }
    @Override protected void onSaveInstanceState(Bundle out){
        super.onSaveInstanceState(out);out.putInt("active_screen",activeScreen);
    }
    private void openFtp(){
        if(busy)return;
        android.content.Intent intent=new android.content.Intent(this,FtpActivity.class);
        intent.putExtra("console_ip",consoleIp.getText().toString().trim());startActivity(intent);
    }
    private void updateHomeConsole(){
        if(homeConsole==null)return;
        String ip=consoleIp.getText().toString().trim();
        boolean valid=RemoteProtocol.privateIp(ip)&&RemoteProtocol.validKey(key.getText().toString().trim());
        boolean verified=valid&&metricReady&&ip.equals(metricHost)&&key.getText().toString().trim().equals(metricKey);
        homeConsole.setText((verified?getString(R.string.home_last_verified):getString(valid?R.string.home_ready:R.string.home_not_paired))+"\n"+(ip.isEmpty()?getString(R.string.home_no_ip):ip));
        if(!verified&&homeMetrics!=null)homeMetrics.setText(getString(R.string.no_readings));
    }
    private void homeTile(LinearLayout row,int icon,int title,Runnable action){
        com.google.android.material.card.MaterialCardView card=NativeUi.card(this);
        LinearLayout inside=new LinearLayout(this);inside.setOrientation(LinearLayout.VERTICAL);inside.setPadding(dp(14),dp(16),dp(14),dp(12));
        ImageView image=new ImageView(this);image.setImageResource(icon);image.setColorFilter(NativeUi.themeColor(this,com.google.android.material.R.attr.colorPrimary));image.setContentDescription(null);
        inside.addView(image,new LinearLayout.LayoutParams(dp(32),dp(32)));
        TextView name=text(getString(title),16,TEXT);name.setPadding(0,dp(10),0,0);inside.addView(name);
        card.addView(inside);card.setClickable(true);card.setFocusable(true);card.setContentDescription(getString(title));card.setOnClickListener(v->action.run());
        LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,-1,1);lp.setMargins(dp(4),dp(4),dp(4),dp(4));row.addView(card,lp);
    }
    private LinearLayout controlPanel(int title){
        com.google.android.material.card.MaterialCardView card=NativeUi.card(this);
        LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);panel.setPadding(dp(10),dp(10),dp(10),dp(10));
        TextView heading=text(getString(title),18,TEXT);heading.setTypeface(null,Typeface.BOLD);heading.setPadding(dp(4),dp(2),0,dp(8));panel.addView(heading);
        card.addView(panel);LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.setMargins(0,dp(8),0,dp(8));content.addView(card,lp);return panel;
    }
    private void buildHome(){
        TextView heading=text(getString(R.string.nav_home),28,TEXT);heading.setTypeface(null,Typeface.BOLD);content.addView(heading);
        com.google.android.material.card.MaterialCardView device=NativeUi.card(this);
        LinearLayout inside=new LinearLayout(this);inside.setOrientation(LinearLayout.VERTICAL);inside.setPadding(dp(16),dp(16),dp(16),dp(16));
        inside.addView(text("Xbox 360",23,TEXT));homeConsole=text("",14,ACCENT);inside.addView(homeConsole);
        homeMetrics=text(getString(R.string.no_readings),14,MUTED);homeMetrics.setPadding(0,dp(10),0,dp(8));inside.addView(homeMetrics);
        inside.addView(button(getString(R.string.query_metrics),ACCENT,()->{
            if(!RemoteProtocol.privateIp(consoleIp.getText().toString().trim())||!RemoteProtocol.validKey(key.getText().toString().trim())){showScreen(3);return;}
            refreshMonitor();
        }));device.addView(inside);content.addView(device,new LinearLayout.LayoutParams(-1,-2));
        label(getString(R.string.home_shortcuts));
        LinearLayout row1=new LinearLayout(this),row2=new LinearLayout(this),row3=new LinearLayout(this);
        homeTile(row1,R.drawable.nav_control,R.string.nav_control,()->showScreen(0));
        homeTile(row1,R.drawable.nav_tv,R.string.home_iptv,()->{showScreen(1);loadCatalog(true,true);});
        homeTile(row2,R.drawable.nav_catalog,R.string.home_library,()->{showScreen(1);loadCatalog(false,true);});
        homeTile(row2,R.drawable.nav_store,R.string.nav_store,()->showScreen(4));
        homeTile(row3,R.drawable.nav_folder,R.string.ftp_title,this::openFtp);
        homeTile(row3,R.drawable.nav_download,R.string.home_downloads,()->{showScreen(2);refreshDownloads();});
        content.addView(row1);content.addView(row2);content.addView(row3);
        updateHomeConsole();
    }
    private String size(org.json.JSONObject data,String name)throws org.json.JSONException{
        if(data.isNull(name))return "Indisponível";
        long value=data.getLong(name);if(value<0)return "Indisponível";
        return android.text.format.Formatter.formatFileSize(this,value);
    }
    private String formatSnapshot(String raw)throws Exception{
        org.json.JSONObject data=new org.json.JSONObject(raw);
        if(data.optInt("schema")!=1)throw new java.io.IOException("Versão incompatível");
        StringBuilder out=new StringBuilder(data.optString("mode").equals("iptv")?"Modo IPTV":"Modo dashboard");
        out.append("\nJogos: ").append(data.getLong("games")).append("  |  Apps: ").append(data.getLong("apps"));
        out.append("\nNotificações não lidas: ").append(data.getLong("notifications"));
        out.append("\nTransferências: ").append(data.getBoolean("transfer_active")?"Ativas":"Sem atividade");
        out.append("\nEtapas na fila local: ").append(data.getLong("queue_steps"));
        out.append("\nRAM física: ").append(size(data,"memory_total"));
        out.append("\nRAM disponível ao título: ").append(size(data,"memory_title_free"));
        org.json.JSONArray temps=data.getJSONArray("temperatures");String[] names={"CPU","GPU","EDRAM","Chassi"};
        for(int i=0;i<4;i++){out.append("\n").append(names[i]).append(": ");
            if(temps.isNull(i))out.append("Indisponível");
            else{int reading=temps.getInt(i);out.append(reading>=10&&reading<=1250?String.format(java.util.Locale.getDefault(),"%.1f °C",reading/10.0):"Indisponível");}}
        org.json.JSONArray disks=data.getJSONArray("storage_cached");
        out.append("\n\nArmazenamento (última consulta da dashboard)");
        if(disks.length()==0)out.append("\nSem leitura");
        for(int i=0;i<Math.min(16,disks.length());i++){org.json.JSONObject d=disks.getJSONObject(i);out.append("\nUnidade ").append(d.getInt("slot")).append(": ").append(size(d,"free")).append(" livres / ").append(size(d,"capacity"));}
        return out.toString();
    }
    private void loadCatalog(boolean tv,boolean reset){
        if(busy)return;
        String ip=consoleIp.getText().toString().trim(),secret=key.getText().toString().trim();
        if(!RemoteProtocol.privateIp(ip)||!RemoteProtocol.validKey(secret)){status.setText("Confira o IP e a chave.");return;}
        if(!reset&&catalogNext<0){status.setText("Fim da consulta. Faça uma nova busca.");return;}
        final int offset=reset?0:catalogNext;
        final String query=reset?catalogQuery.getText().toString().trim():activeQuery;
        if(query.getBytes(java.nio.charset.StandardCharsets.UTF_8).length>64){status.setText("Busca limitada a 64 bytes.");return;}
        busy=true;status.setText("Consultando catálogo...");
        worker.execute(()->{
            String result;int next=-1;boolean success=false;org.json.JSONArray data=new org.json.JSONArray();
            try{
                org.json.JSONObject page=new org.json.JSONObject(RemoteProtocol.catalog(ip,secret,tv,offset,query));
                if(page.optInt("schema")!=1)throw new java.io.IOException("Versão incompatível");
                org.json.JSONArray rows=page.getJSONArray("items");if(rows.length()>16)throw new java.io.IOException("Página inválida");
                data=rows;
                if(!page.isNull("next")){next=page.getInt("next");if(next<=offset||next>999999)throw new java.io.IOException("Cursor inválido");}
                result=(tv?"TV ao vivo":"Biblioteca de jogos")+" • "+rows.length()+" resultados nesta página";success=true;
            }catch(Exception e){result="Consulta indisponível. Confira a conexão e a versão da dashboard.";}
            final String message=result;final int cursor=next;final boolean ok=success;final org.json.JSONArray items=data;
            runOnUiThread(()->{if(isDestroyed())return;busy=false;catalogResult.setText(message);renderCatalog(items,tv,ok,ip,secret);catalogNext=ok?cursor:-1;tvCatalog=tv;activeQuery=query;status.setText(ok?(cursor<0?"Fim da consulta.":"Página recebida."):"Falha na consulta.");});
        });
    }
    private void renderCatalog(org.json.JSONArray items,boolean tv,boolean ok,String ip,String secret){
        catalogCards.removeAllViews();if(!ok)return;
        for(int i=0;i<items.length();i++){
            org.json.JSONObject item=items.optJSONObject(i);if(item==null)continue;
            LinearLayout card=new LinearLayout(this);card.setOrientation(LinearLayout.VERTICAL);NativeUi.separated(this,card);card.setPadding(dp(12),dp(12),dp(12),dp(12));
            LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.setMargins(0,dp(8),0,0);catalogCards.addView(card,lp);
            final String name=item.optString("name"),id=item.optString("id");
            TextView title=text((item.optBoolean("favorite")?"★ ":"")+name,18,TEXT);title.setTypeface(null,Typeface.BOLD);card.addView(title);
            if(!tv&&id.matches("[1-9][0-9]{0,19}")){
                ImageView cover=new ImageView(this);cover.setScaleType(ImageView.ScaleType.FIT_CENTER);cover.setVisibility(View.GONE);card.addView(cover,new LinearLayout.LayoutParams(-1,dp(150)));
                card.addView(button("Carregar capa",PANEL,()->loadArt(id,ip,secret,cover,false)));
            }
            if(tv){
                card.addView(text(item.optString("group"),13,MUTED));
                if(!item.optString("current").isEmpty())card.addView(text("Agora: "+item.optString("current"),14,ACCENT));
                if(!item.optString("next").isEmpty())card.addView(text("A seguir: "+item.optString("next"),14,MUTED));
                if(id.matches("[1-9][0-9]{0,19}")){
                    ImageView logo=new ImageView(this);logo.setScaleType(ImageView.ScaleType.FIT_CENTER);logo.setVisibility(View.GONE);card.addView(logo,new LinearLayout.LayoutParams(-1,dp(88)));
                    Button load=button("Carregar logotipo",PANEL,()->loadArt(id,ip,secret,logo,true));card.addView(load);
                }
                if(id.matches("[1-9][0-9]{0,19}"))card.addView(button("Assistir no Xbox",ACCENT,()->{
                    if(!consoleIp.getText().toString().trim().equals(ip)||!key.getText().toString().trim().equals(secret)){status.setText("Conexão alterada. Consulte novamente.");return;}
                    new MaterialAlertDialogBuilder(this).setTitle("Trocar canal?").setMessage(name).setNegativeButton(getString(R.string.cancel),null).setPositiveButton("Assistir",(d,w)->send("tune/"+id)).show();
                }));
            }
        }
    }
    private void loadArt(String id,String ip,String secret,ImageView target,boolean tv){
        if(busy)return;
        if(!consoleIp.getText().toString().trim().equals(ip)||!key.getText().toString().trim().equals(secret)){status.setText("Conexão alterada. Consulte novamente.");return;}
        busy=true;status.setText("Consultando imagem local...");
        worker.execute(()->{
            android.graphics.Bitmap bitmap=null;String result="Imagem local indisponível.";
            try{
                org.json.JSONObject obj=new org.json.JSONObject(RemoteProtocol.art(ip,secret,id,tv));
                final int width=tv?32:20,height=tv?18:30;
                if(obj.optBoolean("available")&&obj.getInt("w")==width&&obj.getInt("h")==height){
                    String raw=obj.getString("rgba");if(raw.length()!=width*height*8||!raw.matches("[a-f0-9]+"))throw new java.io.IOException("Imagem inválida");
                    int[] pixels=new int[width*height];for(int i=0;i<pixels.length;i++){
                        int p=i*8;int r=Integer.parseInt(raw.substring(p,p+2),16),g=Integer.parseInt(raw.substring(p+2,p+4),16),b=Integer.parseInt(raw.substring(p+4,p+6),16),a=Integer.parseInt(raw.substring(p+6,p+8),16);
                        pixels[i]=android.graphics.Color.argb(a,r,g,b);}
                    bitmap=android.graphics.Bitmap.createBitmap(pixels,width,height,android.graphics.Bitmap.Config.ARGB_8888);result="Imagem recebida.";
                }
            }catch(Exception e){result="Falha ao consultar imagem.";}
            final android.graphics.Bitmap image=bitmap;final String message=result;
            runOnUiThread(()->{if(isDestroyed())return;busy=false;if(image!=null&&target.isAttachedToWindow()){target.setImageBitmap(image);target.setVisibility(View.VISIBLE);}status.setText(message);});
        });
    }
    private void loadDevices(){
        if(busy)return;
        final String ip=consoleIp.getText().toString().trim(),secret=key.getText().toString().trim();
        if(!RemoteProtocol.privateIp(ip)||!RemoteProtocol.validKey(secret)){status.setText("Confira IP e chave.");return;}
        busy=true;status.setText("Consultando armazenamento...");
        worker.execute(()->{
            org.json.JSONArray rows=new org.json.JSONArray();String selected="";boolean ok=false;
            try{
                org.json.JSONObject obj=new org.json.JSONObject(RemoteProtocol.devices(ip,secret));
                if(obj.optInt("schema")!=1)throw new java.io.IOException("Versão incompatível");
                rows=obj.getJSONArray("items");if(rows.length()>5)throw new java.io.IOException("Resposta inválida");
                selected=obj.getString("selected");ok=true;
            }catch(Exception e){}
            final org.json.JSONArray data=rows;final String current=selected;final boolean success=ok;
            runOnUiThread(()->{if(isDestroyed())return;busy=false;deviceCards.removeAllViews();
                deviceStatus.setText(success?"Destino selecionado: "+current:"Armazenamento indisponível.");
                if(success)for(int i=0;i<data.length();i++){
                    org.json.JSONObject item=data.optJSONObject(i);if(item==null)continue;
                    final String id=item.optString("id"),label=item.optString("label");
                    if(!id.matches("local|hdd|usb[012]"))continue;
                    LinearLayout card=new LinearLayout(this);card.setOrientation(LinearLayout.VERTICAL);NativeUi.separated(this,card);card.setPadding(dp(12),dp(12),dp(12),dp(12));
                    LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.setMargins(0,dp(8),0,0);deviceCards.addView(card,lp);
                    card.addView(text(label+(id.equals(current)?" • Selecionado":""),16,TEXT));
                    if(item.optBoolean("available")){
                        long free=item.optLong("free",0),capacity=item.optLong("capacity",0);
                        card.addView(text(android.text.format.Formatter.formatFileSize(this,Math.max(0,free))+" livres / "+android.text.format.Formatter.formatFileSize(this,Math.max(0,capacity)),14,MUTED));
                        if(!id.equals(current))card.addView(button("Selecionar destino",ACCENT,()->{
                            if(!ip.equals(consoleIp.getText().toString().trim())||!secret.equals(key.getText().toString().trim())){status.setText("Conexão alterada. Consulte novamente.");return;}
                            confirmQueueCommand("storedevice/"+id,"Selecionar "+label+"?","A fila atual permanece no dispositivo anterior. Consulte novamente a loja e os downloads.");
                        }));
                    }else card.addView(text("Indisponível ou não preparado",13,MUTED));
                }
                status.setText(success?"Dispositivos recebidos.":"Falha na consulta.");
            });
        });
    }
    private void loadStore(boolean reset){
        if(busy)return;
        String ip=consoleIp.getText().toString().trim(),secret=key.getText().toString().trim();
        if(!RemoteProtocol.privateIp(ip)||!RemoteProtocol.validKey(secret)){status.setText("Confira IP e chave.");return;}
        if(!reset&&storeNext<0){status.setText("Fim do catálogo. Faça nova busca.");return;}
        final int offset=reset?0:storeNext;
        final String query=reset?storeQuery.getText().toString().trim():storeActiveQuery;
        if(query.getBytes(java.nio.charset.StandardCharsets.UTF_8).length>64){status.setText("Busca limitada a 64 bytes.");return;}
        busy=true;status.setText("Consultando INFINITY Store...");
        worker.execute(()->{
            org.json.JSONArray rows=new org.json.JSONArray();int next=-1;boolean ok=false;
            try{
                org.json.JSONObject page=new org.json.JSONObject(RemoteProtocol.store(ip,secret,offset,query));
                if(page.optInt("schema")!=1)throw new java.io.IOException("Versão incompatível");
                rows=page.getJSONArray("items");if(rows.length()>16)throw new java.io.IOException("Página inválida");
                if(!page.isNull("next")){next=page.getInt("next");if(next<=offset||next>999999)throw new java.io.IOException("Cursor inválido");}
                ok=true;
            }catch(Exception e){}
            final org.json.JSONArray data=rows;final int cursor=next;final boolean success=ok;
            runOnUiThread(()->{if(isDestroyed())return;busy=false;storeCards.removeAllViews();storeNext=success?cursor:-1;storeActiveQuery=query;
                storeResult.setText(success?data.length()+" itens nesta página":"Loja indisponível. Confira a conexão e a versão.");
                if(success)for(int i=0;i<data.length();i++){
                    org.json.JSONObject item=data.optJSONObject(i);if(item==null)continue;
                    final String id=item.optString("id"),name=item.optString("name");
                    LinearLayout card=new LinearLayout(this);card.setOrientation(LinearLayout.VERTICAL);NativeUi.separated(this,card);card.setPadding(dp(12),dp(12),dp(12),dp(12));
                    LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.setMargins(0,dp(8),0,0);storeCards.addView(card,lp);
                    TextView title=text(name,18,TEXT);title.setTypeface(null,Typeface.BOLD);card.addView(title);
                    card.addView(text(item.optString("category")+" • "+item.optString("version")+" • "+android.text.format.Formatter.formatFileSize(this,Math.max(0,item.optLong("bytes"))),14,MUTED));
                    if(item.optBoolean("queued"))card.addView(text("Na fila nativa",14,ACCENT));
                    if(item.optBoolean("install_review")&&id.matches("[1-9][0-9]{0,19}")){
                        card.addView(text("Pacote local disponível para revisão",14,ACCENT));
                        card.addView(button("Revisar instalação no Xbox",ACCENT,()->{
                            if(!ip.equals(consoleIp.getText().toString().trim())||!secret.equals(key.getText().toString().trim())){status.setText("Conexão alterada. Consulte novamente.");return;}
                            confirmQueueCommand("storeinstall/"+id,"Abrir instalação de "+name+" no Xbox?","A ficha será aberta no console. Confirme a instalação com o controle físico; o celular não executará a instalação.");
                        }));
                    }else if(item.optBoolean("package")&&id.matches("[1-9][0-9]{0,19}")){
                        card.addView(button("Baixar no Xbox",ACCENT,()->confirmStore(ip,secret,id,name,true)));
                        if(!item.optBoolean("queued"))card.addView(button("Adicionar à fila",PANEL,()->confirmStore(ip,secret,id,name,false)));
                    }else card.addView(text("Apenas referência ou pacote incompatível",13,MUTED));
                }
                status.setText(success?"Catálogo recebido.":"Falha na consulta.");
            });
        });
    }
    private void confirmStore(String ip,String secret,String id,String name,boolean now){
        if(busy)return;
        if(!consoleIp.getText().toString().trim().equals(ip)||!key.getText().toString().trim().equals(secret)){status.setText("Conexão alterada. Consulte novamente.");return;}
        new MaterialAlertDialogBuilder(this).setTitle(now?"Baixar diretamente no Xbox?":"Adicionar à fila do Xbox?")
            .setMessage(name+"\nO destino é o armazenamento selecionado na loja do console. Nenhuma instalação automática será realizada.")
            .setNegativeButton(getString(R.string.cancel),null).setPositiveButton(now?"Baixar":"Adicionar",(d,w)->{
                if(consoleIp.getText().toString().trim().equals(ip)&&key.getText().toString().trim().equals(secret))
                    send((now?"storedownload/":"storequeue/")+id);
            }).show();
    }
    private void confirmQueueCommand(String command,String title,String description){
        if(busy)return;
        final String ip=consoleIp.getText().toString().trim(),secret=key.getText().toString().trim();
        if(!RemoteProtocol.privateIp(ip)||!RemoteProtocol.validKey(secret)){status.setText("Confira IP e chave.");return;}
        new MaterialAlertDialogBuilder(this).setTitle(title).setMessage(description).setNegativeButton(getString(R.string.cancel),null)
            .setPositiveButton("Confirmar",(d,w)->{if(ip.equals(consoleIp.getText().toString().trim())&&secret.equals(key.getText().toString().trim()))send(command);}).show();
    }
    private void renderDownloadCards(org.json.JSONArray rows,String ip,String secret){
        downloadCards.removeAllViews();
        for(int i=0;i<rows.length();i++){
            org.json.JSONObject row=rows.optJSONObject(i);if(row==null)continue;
            final String id=row.optString("id"),name=row.optString("name");
            if(!id.matches("[1-9][0-9]{0,19}"))continue;
            LinearLayout card=new LinearLayout(this);card.setOrientation(LinearLayout.VERTICAL);NativeUi.separated(this,card);card.setPadding(dp(12),dp(12),dp(12),dp(12));
            LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.setMargins(0,dp(8),0,0);downloadCards.addView(card,lp);
            card.addView(text(name,16,TEXT));
            long total=row.optLong("total"),done=row.optLong("received");
            ProgressBar progress=new ProgressBar(this,null,android.R.attr.progressBarStyleHorizontal);progress.setMax(100);progress.setProgress(total>0?(int)Math.min(100,100.0*Math.max(0,done)/total):0);card.addView(progress);
            card.addView(button("Priorizar download",ACCENT,()->{
                if(!ip.equals(consoleIp.getText().toString().trim())||!secret.equals(key.getText().toString().trim())){status.setText("Conexão alterada. Consulte novamente.");return;}
                confirmQueueCommand("storepriority/"+id,"Priorizar "+name+"?","O item irá para o início da fila. O downloader deve estar parado.");
            }));
            card.addView(button("Remover da fila (preservar parcial)",PANEL,()->{
                if(!ip.equals(consoleIp.getText().toString().trim())||!secret.equals(key.getText().toString().trim())){status.setText("Conexão alterada. Consulte novamente.");return;}
                confirmQueueCommand("storeremove/"+id,"Remover "+name+" da fila?","Arquivos parciais serão preservados. O comando só funciona quando o downloader está parado.");
            }));
        }
    }
    private void refreshDownloads(){
        if(busy)return;
        String ip=consoleIp.getText().toString().trim(),secret=key.getText().toString().trim();
        if(!RemoteProtocol.privateIp(ip)||!RemoteProtocol.validKey(secret)){status.setText("Confira o IP e a chave.");return;}
        busy=true;status.setText("Consultando fila...");
        worker.execute(()->{
            String message;boolean success=false;org.json.JSONArray items=new org.json.JSONArray();
            try{
                org.json.JSONObject obj=new org.json.JSONObject(RemoteProtocol.downloads(ip,secret));
                if(obj.optInt("schema")!=1)throw new java.io.IOException("Versão incompatível");
                StringBuilder out=new StringBuilder("Transferência local: "+(obj.getBoolean("local_paused")?"Pausada":obj.getBoolean("local_active")?"Ativa":"Sem atividade"));
                out.append("\n").append(size(obj,"local_done")).append(" / ").append(size(obj,"local_total"));
                if(obj.has("active_name"))out.append("\n\nDownload em andamento: ").append(obj.optString("active_name")).append("\n").append(size(obj,"active_done")).append(" / ").append(size(obj,"active_total"));
                out.append("\n\nFila nativa da loja (registro local): ").append(obj.getBoolean("queue_valid")?obj.getLong("queue_total"):"Indisponível");
                org.json.JSONArray rows=obj.getJSONArray("items");
                if(rows.length()>8)throw new java.io.IOException("Fila inválida");
                items=rows;
                for(int i=0;i<rows.length();i++){
                    org.json.JSONObject row=rows.getJSONObject(i);String state=row.getString("state");
                    String label=state.equals("native_paused")?"Parcial salvo":state.equals("native_verified")||state.equals("verified")?"Verificado":state.equals("receiving")?"Último progresso":state.equals("failed")?"Falhou":"Sem registro";
                    out.append("\n\n").append(row.getString("name")).append("\n").append(label).append(" • ").append(size(row,"received")).append(" / ").append(size(row,"total"));
                }
                org.json.JSONArray history=obj.optJSONArray("history");
                if(history!=null&&history.length()<=16){
                    out.append("\n\nResultados recentes (sessão do Xbox)");
                    if(history.length()==0)out.append("\nNenhum resultado nesta sessão.");
                    for(int i=0;i<history.length();i++){
                        org.json.JSONObject entry=history.getJSONObject(i);
                        out.append("\n").append(entry.getBoolean("success")?"Verificado: ":"Interrompido: ").append(entry.getString("name"));
                    }
                }
                out.append("\n\nAté 8 itens da fila carregada. Registros não equivalem a progresso em tempo real.");
                message=out.toString();success=true;
            }catch(Exception e){message="Fila indisponível. Confira a conexão e a versão do console.";}
            final String text=message;final boolean ok=success;final org.json.JSONArray data=items;
            runOnUiThread(()->{if(isDestroyed())return;busy=false;downloads.setText(text);renderDownloadCards(ok?data:new org.json.JSONArray(),ip,secret);status.setText(ok?"Consulta recebida.":"Falha na consulta.");});
        });
    }
    private void discoverConsole(){
        if(busy)return;
        String local=phoneIp.getText().toString().trim(),secret=key.getText().toString().trim();
        if(!RemoteProtocol.privateIp(local)||!RemoteProtocol.validKey(secret)){status.setText("Identifique o IP do celular e informe a chave já configurada no Xbox.");return;}
        busy=true;status.setText("Buscando console pareado...");
        worker.execute(()->{
            java.util.ArrayList<String> found=new java.util.ArrayList<>();boolean ok=false;
            try{found=ConsoleDiscovery.find(local,secret);ok=true;}catch(Exception e){}
            final String[] choices=found.toArray(new String[0]);final boolean success=ok;
            runOnUiThread(()->{
                if(isDestroyed())return;busy=false;
                if(!phoneIp.getText().toString().trim().equals(local)||!key.getText().toString().trim().equals(secret)){status.setText("Pareamento alterado. Busque novamente.");return;}
                if(choices.length==0){status.setText(success?"Nenhum Xbox pareado respondeu. Confira a rede e habilitação do controle remoto.":"Busca indisponível nesta interface.");return;}
                new MaterialAlertDialogBuilder(this).setTitle("Xbox autenticado encontrado").setItems(choices,(d,w)->{
                    consoleIp.setText(choices[w]);getPreferences(MODE_PRIVATE).edit().putString("console_ip",choices[w]).apply();status.setText("Endereço selecionado. Verifique a conexão.");
                }).setNegativeButton(getString(R.string.cancel),null).show();
            });
        });
    }
    private void refreshMonitor(){
        if(busy)return;
        String ip=consoleIp.getText().toString().trim(),secret=key.getText().toString().trim();
        if(!RemoteProtocol.privateIp(ip)||!RemoteProtocol.validKey(secret)){status.setText("Confira o IP e a chave.");return;}
        busy=true;status.setText("Consultando indicadores...");
        worker.execute(()->{String result;boolean ok;
            try{result=formatSnapshot(RemoteProtocol.snapshot(ip,secret));ok=true;}
            catch(Exception e){result="Não foi possível obter uma leitura autenticada. Confira a conexão e a versão da dashboard.";ok=false;}
            final String message=result;final boolean success=ok;
            runOnUiThread(()->{if(isDestroyed())return;busy=false;monitor.setText(message);status.setText(success?"Leitura recebida.":"Consulta indisponível.");metricReady=success;metricHost=ip;metricKey=secret;updateHomeConsole();if(success&&ip.equals(consoleIp.getText().toString().trim())&&secret.equals(key.getText().toString().trim())&&homeMetrics!=null)homeMetrics.setText(message);});
        });
    }
    private static final int EXPORT_PAIRING=301;
    private void findLocalIp(){
        java.util.ArrayList<String> addresses=new java.util.ArrayList<>();
        try{
            java.util.Enumeration<java.net.NetworkInterface> interfaces=java.net.NetworkInterface.getNetworkInterfaces();
            while(interfaces!=null&&interfaces.hasMoreElements()){
                java.net.NetworkInterface network=interfaces.nextElement();if(!network.isUp()||network.isLoopback())continue;
                java.util.Enumeration<java.net.InetAddress> ips=network.getInetAddresses();
                while(ips.hasMoreElements()){String address=ips.nextElement().getHostAddress();if(RemoteProtocol.privateIp(address)&&!addresses.contains(address))addresses.add(address);}
            }
        }catch(Exception e){status.setText("Não foi possível consultar o IP local.");return;}
        if(addresses.isEmpty()){status.setText("Nenhum IPv4 privado disponível. Confira o Wi-Fi.");return;}
        if(addresses.size()==1){phoneIp.setText(addresses.get(0));status.setText("IP identificado. Confira se corresponde à rede do Xbox.");return;}
        String[] options=addresses.toArray(new String[0]);
        new MaterialAlertDialogBuilder(this).setTitle("Selecione o IP da rede do Xbox").setItems(options,(d,w)->phoneIp.setText(options[w])).show();
    }
    private void exportPairing(){
        if(!RemoteProtocol.privateIp(phoneIp.getText().toString().trim())||!RemoteProtocol.validKey(key.getText().toString().trim())){status.setText("Informe o IP do celular e gere a chave.");return;}
        new MaterialAlertDialogBuilder(this).setTitle("Exportar chave de pareamento?")
            .setMessage("O arquivo contém a chave em texto simples. Salve em armazenamento confiável e remova a cópia após transferir ao Xbox.")
            .setNegativeButton(getString(R.string.cancel),null).setPositiveButton("Exportar",(d,w)->{
                android.content.Intent intent=new android.content.Intent(android.content.Intent.ACTION_CREATE_DOCUMENT);
                intent.addCategory(android.content.Intent.CATEGORY_OPENABLE);intent.setType("text/plain");intent.putExtra(android.content.Intent.EXTRA_TITLE,"cloud360-remote.txt");
                try{startActivityForResult(intent,EXPORT_PAIRING);}catch(Exception e){status.setText("Seletor de arquivos indisponível.");}
            }).show();
    }
    @Override protected void onActivityResult(int request,int result,android.content.Intent data){
        super.onActivityResult(request,result,data);
        if(request!=EXPORT_PAIRING||result!=RESULT_OK||data==null||data.getData()==null)return;
        String phone=phoneIp.getText().toString().trim(),secret=key.getText().toString().trim();
        if(!RemoteProtocol.privateIp(phone)||!RemoteProtocol.validKey(secret)){status.setText("Pareamento alterado; exporte novamente.");return;}
        try(java.io.OutputStream out=getContentResolver().openOutputStream(data.getData(),"wt")){
            if(out==null)throw new java.io.IOException("Arquivo indisponível");
            out.write((phone+"\n"+secret+"\n").getBytes(java.nio.charset.StandardCharsets.UTF_8));status.setText("Arquivo exportado. Transfira para a pasta de dados do Xbox.");
        }catch(Exception e){status.setText("Falha ao salvar arquivo de pareamento.");}
    }
    private void copyConfig(){
        String phone=phoneIp.getText().toString().trim(),secret=key.getText().toString().trim();
        if(!RemoteProtocol.privateIp(phone)||!RemoteProtocol.validKey(secret)){status.setText("Informe o IPv4 do celular e uma chave válida.");return;}
        ClipData clip=ClipData.newPlainText("Configuração IPTV",phone+"\n"+secret+"\n");
        if(Build.VERSION.SDK_INT>=24){android.os.PersistableBundle extras=new android.os.PersistableBundle();extras.putBoolean("android.content.extra.IS_SENSITIVE",true);clip.getDescription().setExtras(extras);}
        ((ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE)).setPrimaryClip(clip);
        status.setText("Configuração copiada. Remova a chave da área de transferência após usar.");
    }
    private void send(String command){
        if(busy)return;
        String ip=consoleIp.getText().toString().trim(),secret=key.getText().toString().trim();
        if(!RemoteProtocol.privateIp(ip)||(command!=null&&!RemoteProtocol.validKey(secret))){status.setText("Confira o IP privado e a chave de pareamento.");return;}
        getPreferences(MODE_PRIVATE).edit().putString("console_ip",ip).apply();
        busy=true;status.setText("Comunicando com o Xbox...");
        worker.execute(()->{String message;try{int code=RemoteProtocol.request(ip,secret,command);message=code==200?(command==null?"Servidor disponível. Chave ainda não verificada.":command.equals("status")?"Pareamento confirmado.":command.startsWith("storedownload/")?"Download solicitado ao Xbox. Consulte a fila.":command.startsWith("storequeue/")?"Item adicionado à fila do Xbox.":command.equals("storestart")?"Início da fila solicitado. Consulte os downloads.":command.equals("storepause")?"Pausa solicitada. Consulte o estado do parcial.":command.startsWith("storeremove/")?"Item removido da fila. Atualize a consulta.":command.startsWith("storepriority/")?"Prioridade atualizada. Consulte a fila.":command.startsWith("storeinstall/")?"Ficha aberta no Xbox. Confirme com o controle físico.":command.startsWith("storedevice/")?"Destino alterado. Consulte os dispositivos e a loja.":"Comando aceito pelo servidor."):"Comando recusado (HTTP "+code+"). Confira a chave ou use o controle físico nesta tela.";}catch(Exception e){message="Console indisponível. Confira a rede e o controle remoto."; } final String msg=message;runOnUiThread(()->{if(isDestroyed())return;busy=false;status.setText(msg);if(command!=null&&command.startsWith("storedevice/")&&msg.startsWith("Destino alterado.")){storeCards.removeAllViews();downloadCards.removeAllViews();deviceCards.removeAllViews();storeNext=-1;storeResult.setText("Consulte o catálogo do novo destino.");downloads.setText("Consulte a fila do novo destino.");deviceStatus.setText("Consulte o destino atualizado.");}});});
    }
    @Override protected void onResume(){super.onResume();foreground=true;updateDownloadPolling();}
    @Override protected void onPause(){foreground=false;pollHandler.removeCallbacks(downloadPoll);super.onPause();}
    @Override protected void onDestroy(){pollHandler.removeCallbacks(downloadPoll);worker.shutdownNow();super.onDestroy();}
}
