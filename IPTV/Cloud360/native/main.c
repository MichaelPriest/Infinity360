/*
 * Cloud360 IPTV - SAFE UI DIAGNOSTIC v0.5.2.
 *
 * The previous demo used fixed coordinates and could draw outside displays
 * smaller than 1280x720. This build limits fonts and layout to actual mode.
 *
 * IMPORTANT: no player, online logos, TLS, HLS, FFmpeg or GPU video.
 * All graphics run on the drawing thread; never run the 0.5.1 build again
 * if it disturbed the Aurora/XAM display.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xsdk/kernel.h"
#include "xsdk/gpu.h"
#include "xsdk/present.h"
#include "xsdk/text.h"
#include "xsdk/log.h"
#include "cloud360.h"

#define COLOR_BG    0xFF091827u
#define COLOR_BLUE  0xFF5FB5F4u
#define COLOR_WHITE 0xFFF6FBFFu
#define COLOR_MUTED 0xFF9DAFC1u
#define COLOR_GREEN 0xFF7AD6A9u

/* At 60 Hz this is approximately 15 seconds; a slower display takes longer.
 * Frame-bounded so it cannot keep corrupt-looking graphics indefinitely.
 */
#define MAX_FRAMES 900u
#define VBLANK_TIMEOUT_MS 1500u
#define GPU_CLOSE_TIMEOUT_MS 1500u
#define MAX_ROWS 7
#define CHAR_PIXEL_WIDTH 10

static CloudItem channels[CLOUD_MAX];
static CloudCatalog catalog;
static CloudFavorites favorites;
static size_t visible[CLOUD_MAX];
static size_t visible_count;
static size_t selected_index;
static unsigned tab_index;
static const char *const tabs[] = {
    "INICIO", "TV AO VIVO", "FILMES", "SERIES", "FAVORITOS", "CONFIG"
};
static int catalog_status=-3;
static char notice[128]="Teste da interface: Back/B sai; saida automatica habilitada.";

static CloudTab section(void)
{
    switch(tab_index) {
        case 1: return CLOUD_LIVE;
        case 2: return CLOUD_MOVIES;
        case 3: return CLOUD_SERIES;
        case 4: return CLOUD_FAVORITES;
        default: return CLOUD_ALL;
    }
}
static void refresh(void)
{
    visible_count=cloud_filter(&catalog,section(),&favorites,NULL,NULL,visible,CLOUD_MAX);
    if(selected_index>=visible_count) selected_index=0;
}
static void load_playlist(void)
{
    catalog_status=cloud_load_m3u(&catalog,"game:\\playlist.m3u");
    refresh();
    xsdk_log_write("cloud360: playlist status=%d entries=%u",
        catalog_status,(unsigned)catalog.count);
    snprintf(notice,sizeof notice,"Lista: %u itens; status=%d",
        (unsigned)catalog.count,catalog_status);
}

/* Screen-width clipping; preserve multibyte UTF-8 code points in names. */
static void fit_columns(char *dst,size_t capacity,const char *src,unsigned columns)
{
    unsigned used=0;
    size_t out=0;
    const unsigned char *p=(const unsigned char *)src;
    if(!capacity)return;
    while(*p && used<columns && out+5<capacity) {
        size_t count=1;
        if(*p>=0xc2 && *p<=0xdf)count=2;
        else if(*p>=0xe0 && *p<=0xef)count=3;
        else if(*p>=0xf0 && *p<=0xf4)count=4;
        if(count>1) {
            bool good=true;
            for(size_t i=1;i<count;i++)
                if(!p[i] || (p[i]&0xc0)!=0x80)good=false;
            if(!good)count=1;
        }
        if(out+count>=capacity)break;
        for(size_t i=0;i<count;i++)dst[out++]=(char)*p++;
        used++;
    }
    dst[out]=0;
}
static void draw_fitted(int x,int y,unsigned scale,uint32_t color,const char *src,
                        unsigned width)
{
    if(x<0 || y<0 || (unsigned)x>=width)return;
    const unsigned pixels_per_char=CHAR_PIXEL_WIDTH*scale;
    const unsigned maxchars=(width-(unsigned)x-8)/pixels_per_char;
    if(!maxchars)return;
    char safe[172];
    fit_columns(safe,sizeof safe,src,maxchars);
    xsdk_text_draw(x,y,scale,color,safe);
}
static void draw_screen(uint32_t width,uint32_t height,unsigned frame)
{
    const unsigned large=width>=1050 && height>=650?2:1;
    const int margin=(int)(width/32);
    const int upper=(int)(height/22);
    char line[176];

    draw_fitted(margin,upper,large,COLOR_BLUE,
        "CLOUD360 IPTV  |  XBOX 360",width);
    draw_fitted(margin,upper+25*(int)large+7,1,COLOR_MUTED,
        "TESTE DE EXIBICAO v0.5.2 - sem video",width);

    const int tabs_y=upper+39*(int)large+15;
    const int cell=(int)(width-2*(unsigned)margin)/6;
    for(unsigned i=0;i<6;i++) {
        char tab[32];
        snprintf(tab,sizeof tab,"%c%.18s%c",
            i==tab_index?'[':' ',tabs[i],i==tab_index?']':' ');
        draw_fitted(margin+(int)i*cell,tabs_y,1,
            i==tab_index?COLOR_BLUE:COLOR_MUTED,tab,width);
    }
    const int heading_y=tabs_y+48;
    snprintf(line,sizeof line,"SECAO: %s   |   %u itens",
        tabs[tab_index],(unsigned)visible_count);
    draw_fitted(margin,heading_y,1,COLOR_WHITE,line,width);

    const int list_y=heading_y+37;
    const int footer_y=(int)height-84;
    const int row_height=height>=650?40:32;
    if(tab_index==5) {
        draw_fitted(margin,list_y,1,COLOR_MUTED,
            "Arquivo: game:\\playlist.m3u",width);
        draw_fitted(margin,list_y+row_height,1,COLOR_MUTED,
            "Log: default.log na pasta do jogo",width);
        draw_fitted(margin,list_y+2*row_height,1,COLOR_WHITE,
            "Este teste NAO reproduz canais ou filmes.",width);
        draw_fitted(margin,list_y+3*row_height,1,COLOR_GREEN,
            "Back ou B retorna ao Aurora.",width);
    } else if(!visible_count) {
        draw_fitted(margin,list_y,1,COLOR_MUTED,
            catalog_status==-3?"playlist.m3u nao encontrada":"Lista ou secao vazia",
            width);
        draw_fitted(margin,list_y+row_height,1,COLOR_MUTED,
            "Consulte a aba CONFIG para detalhes.",width);
    } else {
        size_t first=selected_index>MAX_ROWS/2?
            selected_index-(size_t)(MAX_ROWS/2):0;
        if(first+MAX_ROWS>visible_count && visible_count>MAX_ROWS)
            first=visible_count-MAX_ROWS;
        for(size_t i=first;i<visible_count && i<first+MAX_ROWS;i++) {
            const CloudItem *item=&catalog.items[visible[i]];
            char title[120];
            fit_columns(title,sizeof title,item->name,
                (width>=1000)?67u:43u);
            snprintf(line,sizeof line,"%c %02u %c %s",
                i==selected_index?'>':' ',
                (unsigned)(i+1),
                cloud_favorite_has(&favorites,item)?'*':' ',
                title);
            const int y=list_y+(int)(i-first)*row_height;
            if(y+20< footer_y)
                draw_fitted(margin,y,1,i==selected_index?
                    COLOR_BLUE:COLOR_WHITE,line,width);
        }
        const CloudItem *selected=&catalog.items[visible[selected_index]];
        snprintf(line,sizeof line,"GRUPO: %.42s",selected->group);
        draw_fitted(margin,footer_y-39,1,COLOR_MUTED,line,width);
    }
    draw_fitted(margin,footer_y,1,COLOR_GREEN,
        "D-Pad: navegar    A: detalhes    X: favorito    Y: atualizar",width);
    draw_fitted(margin,footer_y+25,1,COLOR_MUTED,
        "Back/B: sair    Limite de frames para retorno automatico",width);

    /* Status never contains the stream URL or account credentials. */
    if(frame%90u<45u)
        draw_fitted(margin,(int)height-25,1,COLOR_MUTED,notice,width);
}

int main(void)
{
    cloud_init(&catalog,channels,CLOUD_MAX);
    (void)cloud_favorites_load(&favorites,"game:\\favorites.txt");
    load_playlist();
    xsdk_log_write("cloud360: v0.5.2 screen-safe diagnostic started");

    NTSTATUS status=xsdk_gpu_open();
    if(!NT_SUCCESS(status)) {
        xsdk_log_write("FAIL: gpu_open=0x%08X",(unsigned)status);
        return 1;
    }
    xsdk_log_write("PASS: gpu opened");
    uint32_t width=0,height=0;
    status=xsdk_present_open(XSDK_DISPLAY_720P,3,1,&width,&height);
    if(!NT_SUCCESS(status)) {
        xsdk_log_write("FAIL: present_open=0x%08X",(unsigned)status);
        (void)xsdk_gpu_close();
        return 2;
    }
    xsdk_log_write("PASS: display=%ux%u",(unsigned)width,(unsigned)height);
    status=xsdk_text_open();
    if(!NT_SUCCESS(status)) {
        xsdk_log_write("FAIL: text_open=0x%08X",(unsigned)status);
        xsdk_present_close();
        (void)xsdk_gpu_close();
        return 3;
    }
    xsdk_log_write("PASS: text opened");

    uint16_t previous=0;
    unsigned frames=0;
    bool begin_failed=false;
    for(;frames<MAX_FRAMES;frames++) {
        INPUT_STATE input={0};
        uint16_t keys=0;
        if(XamInputGetState(0,0,&input)==ERROR_SUCCESS)
            keys=input.gamepad.buttons;
        uint16_t pressed=keys & (uint16_t)~previous;
        previous=keys;
        if(pressed&(INPUT_GAMEPAD_BACK|INPUT_GAMEPAD_B)) {
            xsdk_log_write("cloud360: exit requested by controller");
            break;
        }
        if(pressed&INPUT_GAMEPAD_DPAD_LEFT) {
            tab_index=(tab_index+5u)%6u;
            selected_index=0;refresh();
        }
        if(pressed&INPUT_GAMEPAD_DPAD_RIGHT) {
            tab_index=(tab_index+1u)%6u;
            selected_index=0;refresh();
        }
        if(pressed&INPUT_GAMEPAD_DPAD_UP && selected_index)
            selected_index--;
        if(pressed&INPUT_GAMEPAD_DPAD_DOWN && selected_index+1<visible_count)
            selected_index++;
        if(pressed&INPUT_GAMEPAD_Y)load_playlist();
        if(pressed&INPUT_GAMEPAD_X && tab_index!=5 && visible_count) {
            const CloudItem *item=&catalog.items[visible[selected_index]];
            int value=cloud_favorite_toggle(&favorites,item);
            int saved=cloud_favorites_save(&favorites,"game:\\favorites.txt");
            snprintf(notice,sizeof notice,"Favorito: %s (salvar=%d)",
                value>0?"adicionado":"removido",saved);
            refresh();
        }
        if(pressed&INPUT_GAMEPAD_A && tab_index!=5 && visible_count) {
            const CloudItem *item=&catalog.items[visible[selected_index]];
            snprintf(notice,sizeof notice,"Tipo: %s; player ainda nao integrado.",
                cloud_format_label(cloud_format(item->url)));
        }

        uint32_t address=0;
        status=xsdk_present_begin(&address,VBLANK_TIMEOUT_MS);
        if(!NT_SUCCESS(status)) {
            xsdk_log_write("FAIL: present_begin=0x%08X frame=%u",
                (unsigned)status,frames);
            begin_failed=true;
            break;
        }
        xsdk_gpu_set_target(width,height);
        /* XSDK resolves the previous eDRAM content, then clears it.
         * The first two frames MUST clear stale content before any text.
         */
        if(frames>=2u)
            draw_screen(width,height,frames);
        xsdk_gpu_resolve(address,width,height,COLOR_BG);
        xsdk_present_end();

        if(frames==3u)xsdk_log_write("PASS: first UI frame submitted");
    }
    xsdk_log_write("cloud360: leaving render loop frames=%u failure=%u",
        frames,(unsigned)begin_failed);

    /* Stop new draw commands and let queued GPU work finish before freeing
     * resources and giving the graphics device back to XAM.
     * Orbita uses similar fence-before-buffer-reuse lifecycle, but its
     * libxenon API is not directly compatible with XSDK.
     */
    status=xsdk_gpu_wait(xsdk_gpu_fence(),GPU_CLOSE_TIMEOUT_MS);
    xsdk_log_write("%s: final GPU fence=0x%08X",
        NT_SUCCESS(status)?"PASS":"FAIL",(unsigned)status);
    xsdk_text_close();
    xsdk_present_close();
    status=xsdk_gpu_close();
    xsdk_log_write("%s: GPU returned to XAM status=0x%08X",
        NT_SUCCESS(status)?"PASS":"FAIL",(unsigned)status);
    return begin_failed||!NT_SUCCESS(status)?4:0;
}
