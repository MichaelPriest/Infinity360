package com.infinity360.remote;

import android.content.Context;
import android.graphics.Color;
import android.os.Build;
import android.util.TypedValue;
import android.view.View;
import android.view.WindowInsets;
import android.widget.*;

final class NativeUi {
    static final int NORMAL=0,SECONDARY=1,EMPHASIS=2;
    private NativeUi(){}
    static int dp(Context c,float n){return Math.round(n*c.getResources().getDisplayMetrics().density);}
    static TextView text(Context c,String value,int sp,int role){
        TextView t=new TextView(c);
        t.setTextAppearance(android.R.style.TextAppearance_Material_Body1);
        TypedValue attr=new TypedValue();
        int color=role==SECONDARY?android.R.attr.textColorSecondary:role==EMPHASIS?android.R.attr.colorAccent:android.R.attr.textColorPrimary;
        if(c.getTheme().resolveAttribute(color,attr,true)){
            if(attr.resourceId!=0)t.setTextColor(c.getColorStateList(attr.resourceId));
            else if(attr.type>=TypedValue.TYPE_FIRST_COLOR_INT&&attr.type<=TypedValue.TYPE_LAST_COLOR_INT)t.setTextColor(attr.data);
        }
        t.setText(value);t.setTextSize(sp);return t;
    }
    static Button button(Context c,String value,Runnable action){
        Button b=new Button(c);b.setText(value);b.setAllCaps(false);b.setMinHeight(dp(c,48));b.setOnClickListener(v->action.run());return b;
    }
    static EditText field(Context c,String hint,int type){
        EditText e=new EditText(c);e.setSingleLine(true);e.setInputType(type);e.setHint(hint);e.setSaveEnabled(false);e.setMinHeight(dp(c,48));return e;
    }
    static void brand(Context c,LinearLayout parent,String subtitle){
        ImageView logo=new ImageView(c);logo.setImageResource(R.drawable.infinity_official_logo);
        logo.setScaleType(ImageView.ScaleType.FIT_CENTER);logo.setBackgroundColor(Color.rgb(16,28,22));
        logo.setPadding(dp(c,16),dp(c,8),dp(c,16),dp(c,8));logo.setContentDescription("INFINITY 360");
        parent.addView(logo,new LinearLayout.LayoutParams(-1,dp(c,62)));
        TextView title=text(c,subtitle,18,NORMAL);title.setPadding(dp(c,16),dp(c,8),dp(c,16),dp(c,4));parent.addView(title);
    }
    static void separated(Context c,LinearLayout row){
        TypedValue v=new TypedValue();
        if(c.getTheme().resolveAttribute(android.R.attr.listDivider,v,true)&&v.resourceId!=0){
            row.setDividerDrawable(c.getDrawable(v.resourceId));row.setShowDividers(LinearLayout.SHOW_DIVIDER_BEGINNING);
        }
    }
    static void insets(View root){
        if(Build.VERSION.SDK_INT>=30){
            root.setOnApplyWindowInsetsListener((v,insets)->{
                android.graphics.Insets safe=insets.getInsets(WindowInsets.Type.systemBars()|WindowInsets.Type.displayCutout());
                v.setPadding(safe.left,safe.top,safe.right,safe.bottom);return insets;
            });root.requestApplyInsets();
        }
    }
}
