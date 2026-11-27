package com.infinity360.remote;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.os.Build;
import android.text.InputType;
import android.view.Gravity;
import android.view.View;
import android.view.WindowInsets;
import android.widget.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class MainActivity extends Activity {
    private static final int BG=0xff101b15, PANEL=0xff20382b, ACCENT=0xffa8ff00, TEXT=0xffeff7f0, MUTED=0xffb1c8bb;
    private EditText consoleIp,phoneIp,key;
    private TextView status;
    private LinearLayout content;
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private boolean busy=false;
    private int dp(float n){return Math.round(n*getResources().getDisplayMetrics().density);}
    private GradientDrawable shape(int color){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(14));return d;}
    private TextView text(String s,int size,int color){TextView v=new TextView(this);v.setText(s);v.setTextSize(size);v.setTextColor(color);return v;}
    private void label(String s){TextView v=text(s,13,MUTED);v.setPadding(0,dp(10),0,dp(4));content.addView(v);}
    private EditText field(String hint,int type){EditText v=new EditText(this);v.setSingleLine(true);v.setInputType(type);v.setHint(hint);v.setTextColor(TEXT);v.setHintTextColor(MUTED);v.setPadding(dp(14),dp(10),dp(14),dp(10));v.setBackground(shape(PANEL));v.setSaveEnabled(false);content.addView(v,new LinearLayout.LayoutParams(-1,dp(56)));return v;}
    private Button button(String label,int color,Runnable action){Button b=new Button(this);b.setText(label);b.setAllCaps(false);b.setTextColor(color==ACCENT?BG:TEXT);b.setTextSize(16);b.setBackground(shape(color));b.setMinHeight(dp(52));b.setOnClickListener(v->action.run());return b;}
    private void row(String[] labels,String[] commands){LinearLayout row=new LinearLayout(this);row.setOrientation(LinearLayout.HORIZONTAL);for(int i=0;i<labels.length;i++){final String cmd=commands[i];View b=cmd==null?new View(this):button(labels[i],cmd.equals("accept")?ACCENT:PANEL,()->send(cmd));LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,dp(61),1);lp.setMargins(dp(3),dp(3),dp(3),dp(3));row.addView(b,lp);}content.addView(row);}
    @Override public void onCreate(Bundle saved){
        super.onCreate(saved);getWindow().setStatusBarColor(BG);getWindow().setNavigationBarColor(BG);
        ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);scroll.setBackgroundColor(BG);
        content=new LinearLayout(this);content.setOrientation(LinearLayout.VERTICAL);content.setPadding(dp(18),dp(18),dp(18),dp(24));scroll.addView(content);setContentView(scroll);
        scroll.setOnApplyWindowInsetsListener((view,insets)->{
            if(Build.VERSION.SDK_INT>=30){android.graphics.Insets safe=insets.getInsets(WindowInsets.Type.systemBars()|WindowInsets.Type.displayCutout());content.setPadding(dp(18)+safe.left,dp(18)+safe.top,dp(18)+safe.right,dp(24)+safe.bottom);}
            return insets;
        });
        TextView brand=text("∞  INFINITY 360",29,ACCENT);brand.setTypeface(null,Typeface.BOLD);content.addView(brand);
        content.addView(text("REMOTE  •  CLOUD360 IPTV",13,MUTED));
        label("IP do Xbox 360");consoleIp=field("192.168.x.x",InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_URI);
        consoleIp.setText(getPreferences(MODE_PRIVATE).getString("console_ip",""));
        label("Chave de pareamento (32 caracteres)");key=field("Chave hexadecimal",InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_PASSWORD);key.setFilters(new android.text.InputFilter[]{new android.text.InputFilter.LengthFilter(32)});
        content.addView(button("Verificar conexão",PANEL,()->send(null)),new LinearLayout.LayoutParams(-1,dp(54)));
        status=text("Inicie a reprodução IPTV no Xbox.",14,MUTED);status.setPadding(0,dp(10),0,dp(10));status.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);content.addView(status);
        row(new String[]{"Canais","↑","Qualidade"},new String[]{"channels","up","quality"});
        row(new String[]{"←","A / OK","→"},new String[]{"left","accept","right"});
        row(new String[]{"Vol −","↓","Vol +"},new String[]{"quieter","down","louder"});
        row(new String[]{"Mudo"},new String[]{"mute"});
        Button exit=button("Encerrar reprodução",0xff702e34,()->new AlertDialog.Builder(this).setTitle("Encerrar IPTV?").setMessage("A reprodução no Xbox será encerrada.").setNegativeButton("Cancelar",null).setPositiveButton("Encerrar",(d,w)->send("back")).show());content.addView(exit,new LinearLayout.LayoutParams(-1,dp(56)));
        label("Configuração manual de pareamento");
        phoneIp=field("IPv4 do celular na rede Wi-Fi",InputType.TYPE_CLASS_TEXT|InputType.TYPE_TEXT_VARIATION_URI);
        content.addView(button("Gerar nova chave",PANEL,()->{key.setText(RemoteProtocol.newKey());status.setText("Nova chave gerada. Copie a configuração para o console.");}),new LinearLayout.LayoutParams(-1,dp(54)));
        content.addView(button("Copiar configuração do Xbox",PANEL,()->copyConfig()),new LinearLayout.LayoutParams(-1,dp(54)));
        TextView note=text("Salve as duas linhas em cloud360-remote.txt na pasta de dados IPTV do Xbox. HTTP local sem criptografia: use somente Wi-Fi confiável. A chave fica apenas na memória do aplicativo.",13,MUTED);note.setPadding(0,dp(12),0,0);content.addView(note);
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
        worker.execute(()->{String message;try{int code=RemoteProtocol.request(ip,secret,command);message=code==200?(command==null?"Servidor disponível. Chave ainda não verificada.":"Comando aceito pelo servidor."):"Comando recusado (HTTP "+code+"). Confira a chave e o IP autorizado.";}catch(Exception e){message="Console indisponível. Confira a rede e a reprodução."; } final String msg=message;runOnUiThread(()->{if(isDestroyed())return;busy=false;status.setText(msg);});});
    }
    @Override protected void onDestroy(){worker.shutdownNow();super.onDestroy();}
}
