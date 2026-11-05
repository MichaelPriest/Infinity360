/*
 * Cloud360 IPTV: native Xbox 360 XSDK shell.
 * Catalog browsing only. No network client or video decoder yet.
 */
#include <stdint.h>
#include <stdio.h>
#include "xsdk/kernel.h"
#include "xsdk/gpu.h"
#include "xsdk/present.h"
#include "xsdk/text.h"
#include "xsdk/log.h"
#include "cloud360.h"
#include "logo.h"
#define BG 0xFF071426u
#define BLUE 0xFF49A8FFu
#define WHITE 0xFFF8FBFFu
#define MUTED 0xFF97B3CDu
#define LIME 0xFF85DEC0u
static CloudItem channels[CLOUD_MAX];
static CloudCatalog catalog;
static CloudFavorites favorites;
static size_t visible[CLOUD_MAX], visible_count, current;
static int tab;
static int logo_ready=0;
static void sync_logo(void) {
    if(logo_ready && visible_count) (void)cloud_logo_select(visible[current]);
}
static const char *tabs[]={"INICIO","TV AO VIVO","FILMES","SERIES","FAVORITOS","CONFIG"};
static char message[180]="Leitor de catalogo - ainda sem player";

static CloudTab section(void) {
    switch(tab) {
      case 1: return CLOUD_LIVE;
      case 2: return CLOUD_MOVIES;
      case 3: return CLOUD_SERIES;
      case 4: return CLOUD_FAVORITES;
      default: return CLOUD_ALL;
    }
}
static void refresh_view(void) {
    visible_count=cloud_filter(&catalog, section(), &favorites, NULL, NULL, visible, CLOUD_MAX);
    if(current >= visible_count) current=0;
    sync_logo();
}
static void reload(void) {
    int code=cloud_load_m3u(&catalog,"game:/playlist.m3u");
    refresh_view();
    snprintf(message,sizeof message,"Playlist: %u entradas (codigo %d)",(unsigned)catalog.count,code);
    xsdk_log_write("cloud360 reload: code=%d count=%u",code,(unsigned)catalog.count);
}
static void text(int x,int y,uint32_t color,const char *str) {
    xsdk_text_draw(x,y,1,color,str);
}
static void render(void) {
    xsdk_text_draw(55,35,2,BLUE,"CLOUD360");
    text(56,88,MUTED,"IPTV  |  Xbox 360  |  Protótipo nativo");
    for(int i=0;i<6;i++) {
        char label[58];
        snprintf(label,sizeof label,"%c %s",tab==i?'>':' ',tabs[i]);
        text(56,150+i*63,tab==i?BLUE:WHITE,label);
    }
    xsdk_text_draw(363,110,2,WHITE,tabs[tab]);
    char line[160];
    if(tab==5) {
        text(365,188,WHITE,"Fonte: game:/playlist.m3u");
        text(365,226,MUTED,"Xtream: exportacao pelo computador");
        text(365,264,MUTED,"O player e a rede estao em desenvolvimento.");
        text(365,302,MUTED,"Y: recarregar catalogo");
    } else {
        snprintf(line,sizeof line,"Entradas visiveis: %u",(unsigned)visible_count);
        text(365,169,MUTED,line);
        if(!visible_count) {
            text(365,226,WHITE,"Nenhum item nesta categoria.");
            text(365,264,MUTED,"Copie uma playlist.m3u autorizada");
            text(365,301,MUTED,"para a pasta do aplicativo.");
        }
        size_t first=current>4?current-4:0;
        for(size_t i=first;i<visible_count && i<first+10;i++) {
            const CloudItem *item=&catalog.items[visible[i]];
            snprintf(line,sizeof line,"%c %c %.53s",i==current?'>':' ',
                cloud_favorite_has(&favorites,item)?'*':' ',item->name);
            text(365,205+(int)(i-first)*36,i==current?BLUE:WHITE,line);
        }
        if(visible_count) {
            const CloudItem *item=&catalog.items[visible[current]];
            snprintf(line,sizeof line,"Grupo: %.45s | Tipo: %s",item->group,
                cloud_format_label(cloud_format(item->url)));
            text(365,598,MUTED,line);
        }
    }
    text(56,657,LIME,"D-PAD navegar   X favorito   Y atualizar   A detalhes   BACK sair");
    text(56,683,MUTED,message);
    cloud_logo_draw();
}
int main(void) {
    cloud_init(&catalog,channels,CLOUD_MAX);
    (void)cloud_favorites_load(&favorites,"game:/favorites.txt");
    reload();
    NTSTATUS status=xsdk_gpu_open();
    if(!NT_SUCCESS(status)) return 1;
    uint32_t width=0,height=0;
    status=xsdk_present_open(XSDK_DISPLAY_720P,2,1,&width,&height);
    if(!NT_SUCCESS(status)) { xsdk_gpu_close(); return 2; }
    status=xsdk_text_open();
    if(!NT_SUCCESS(status)) { xsdk_present_close(); xsdk_gpu_close(); return 3; }
    logo_ready=(cloud_logo_init()==0);
    sync_logo();
    uint16_t previous=0;
    for(;;) {
        INPUT_STATE input={0};
        uint16_t keys=0;
        if(XamInputGetState(0,0,&input)==ERROR_SUCCESS) keys=input.gamepad.buttons;
        uint16_t pressed=keys & (uint16_t)~previous;
        previous=keys;
        if(pressed & INPUT_GAMEPAD_BACK) break;
        if(pressed & INPUT_GAMEPAD_DPAD_LEFT) { tab=(tab+5)%6; current=0;refresh_view(); }
        if(pressed & INPUT_GAMEPAD_DPAD_RIGHT) { tab=(tab+1)%6; current=0;refresh_view(); }
        if((pressed & INPUT_GAMEPAD_DPAD_UP) && current>0) current--;
        if((pressed & INPUT_GAMEPAD_DPAD_DOWN) && current+1<visible_count) current++;
        if(pressed & (INPUT_GAMEPAD_DPAD_UP | INPUT_GAMEPAD_DPAD_DOWN)) sync_logo();
        if(pressed & INPUT_GAMEPAD_Y) reload();
        if((pressed & INPUT_GAMEPAD_X) && visible_count && tab!=5) {
            int added=cloud_favorite_toggle(&favorites,&catalog.items[visible[current]]);
            int saved=cloud_favorites_save(&favorites,"game:/favorites.txt");
            snprintf(message,sizeof message,"Favorito %s (salvar: %d)",
                added>0?"adicionado":"removido",saved);
            refresh_view();
        }
        if((pressed & INPUT_GAMEPAD_A) && visible_count && tab!=5) {
            const CloudItem *item=&catalog.items[visible[current]];
            snprintf(message,sizeof message,"%.68s - %s - player pendente",
                item->name,cloud_format_label(cloud_format(item->url)));
        }
        uint32_t addr=0;
        if(!NT_SUCCESS(xsdk_present_begin(&addr,5000))) break;
        xsdk_gpu_set_target(width,height);
        render();
        xsdk_gpu_resolve(addr,width,height,BG);
        xsdk_present_end();
    }
    if(logo_ready) cloud_logo_close();
    xsdk_text_close();
    xsdk_present_close();
    xsdk_gpu_close();
    return 0;
}
