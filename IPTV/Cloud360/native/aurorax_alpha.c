/*
 * AuroraX / Cloud360 first open-XSDK XEX alpha.
 *
 * Uses the same tightly bounded present lifecycle as Cloud360 SAFE v0.5.2.
 * Deliberately excludes game launching, streaming, codecs and boot changes.
 * A visual/navigation smoke test, NOT the final PDF-faithful XDK dashboard.
 * Do not test until the no-GPU probe and Cloud360 v0.5.2 return are safe.
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

#define BG             0xFF031710u
#define LIME           0xFF9DEC31u
#define WHITE          0xFFF2F7F3u
#define MUTED          0xFFB7C9C1u
#define PANEL          0xFF172C28u
#define MAX_FRAMES     900u
#define DISPLAY_WAIT   1500u
#define GPU_WAIT       1500u
#define MAX_ROWS       8u
#define CHAR_WIDTH     10u

static CloudItem items[CLOUD_MAX];
static CloudCatalog catalogue;
static CloudFavorites favorites;
static size_t visible[CLOUD_MAX];
static size_t count;
static size_t selected;
static unsigned tab;
static unsigned home_focus;
static unsigned page; /* 0 home; 1 Cloud360; 2 other section */
static int playlist_status;
static char info[132] = "Alpha grafica: teste de navegacao, sem player nem launch.";
static const char *const home_labels[] = {
    "Biblioteca", "Cloud360 IPTV",
    "Aplicativos", "Emuladores", "Arquivos", "Sistema",
    "Recentes", "Favoritos"
};
static const char *const tab_labels[] = {
    "TODOS", "TV AO VIVO", "FILMES", "SERIES", "FAVORITOS"
};

static CloudTab category(void)
{
    switch (tab) {
        case 1: return CLOUD_LIVE;
        case 2: return CLOUD_MOVIES;
        case 3: return CLOUD_SERIES;
        case 4: return CLOUD_FAVORITES;
        default: return CLOUD_ALL;
    }
}

static void filter_items(void)
{
    count = cloud_filter(&catalogue, category(), &favorites,
                         NULL, NULL, visible, CLOUD_MAX);
    if (selected >= count) selected = 0;
}

static void read_playlist(void)
{
    playlist_status = cloud_load_m3u(&catalogue, "game:\\playlist.m3u");
    filter_items();
    snprintf(info, sizeof info, "M3U local: %u itens (status %d)",
             (unsigned)catalogue.count, playlist_status);
    xsdk_log_write("aurorax: M3U status=%d count=%u",
                   playlist_status, (unsigned)catalogue.count);
}

/* Clip text to the actual framebuffer width and an ASCII-safe buffer. */
static void fitted(int x, int y, unsigned scale, uint32_t color,
                   const char *src, unsigned width, unsigned height)
{
    if (x < 0 || y < 0 || (unsigned)x >= width || (unsigned)y >= height ||
        !scale || !src) return;
    unsigned max_chars = (width - (unsigned)x) / (CHAR_WIDTH * scale);
    if (max_chars > 123u) max_chars = 123u;
    char trimmed[124];
    unsigned n = 0;
    const unsigned char *p = (const unsigned char *)src;
    while (*p && n < max_chars) {
        unsigned char ch = *p++;
        /* XSDK text font is ASCII; do not slice UTF-8 partial code points. */
        if (ch >= 0x80u) {
            while ((*p & 0xc0u) == 0x80u) ++p;
            trimmed[n++] = '?';
        } else trimmed[n++] = (char)ch;
    }
    trimmed[n] = 0;
    xsdk_text_draw(x, y, scale, color, trimmed);
}

static void draw(unsigned width, unsigned height)
{
    unsigned big = width >= 1000u && height >= 650u ? 2u : 1u;
    int margin = (int)(width / 25u);
    int top = (int)(height / 17u);
    int gap = height >= 650u ? 49 : 32;
    int footer = (int)height - 61;
    char line[168];
    fitted(margin, top, big, LIME, "INFINITY 360", width, height);
    fitted(margin + (int)(135u * big), top+6, 1u, MUTED,
           "| CLOUD360 IPTV | ALPHA XSDK", width, height);
    fitted(margin, top+39*(int)big, 1u, MUTED,
           "Referencia visual: AuroraX_Telas.pdf / validacao inicial",
           width, height);

    if (page == 0) {
        int y = top + 76*(int)big;
        fitted(margin, y, big, WHITE, "Inicio", width, height);
        for (unsigned i = 0; i < 8u; ++i) {
            int row = (int)(i/2u);
            int col = (int)(i%2u);
            int x = margin + col*(int)(width/2u-10u);
            int item_y = y + 45 + row*gap;
            snprintf(line, sizeof line, "%c %s", i==home_focus ? '>' : ' ',
                     home_labels[i]);
            fitted(x,item_y,1u,i==home_focus?LIME:WHITE,line,width,height);
        }
        fitted(margin,footer-42,1u,MUTED,
               "Biblioteca / IPTV / Apps / Emuladores / Arquivos / Sistema",
               width,height);
    } else if(page == 1) {
        fitted(margin,top+76*(int)big,big,WHITE,"Cloud360 IPTV",width,height);
        int ty = top + 116*(int)big;
        for(unsigned i=0;i<5u;i++) {
            int x = margin+(int)i*(int)((width-2u*(unsigned)margin)/5u);
            snprintf(line,sizeof line,"%c%s",tab==i?'>':' ',tab_labels[i]);
            fitted(x,ty,1u,tab==i?LIME:MUTED,line,width,height);
        }
        int list_y=ty+44;
        if(!count) {
            fitted(margin,list_y,1u,MUTED,
                   "Sem conteudo nesta aba. Adicione playlist.m3u.",width,height);
        } else {
            size_t first=selected>3u ? selected-3u : 0u;
            if(first+MAX_ROWS>count && count>MAX_ROWS) first=count-MAX_ROWS;
            for(size_t i=first;i<count && i<first+MAX_ROWS;i++) {
                const CloudItem *item=&catalogue.items[visible[i]];
                snprintf(line,sizeof line,"%c %02u %c %.70s",
                         i==selected?'>':' ',(unsigned)(i+1u),
                         cloud_favorite_has(&favorites,item)?'*':' ',item->name);
                int iy=list_y+(int)(i-first)*33;
                if(iy<footer-80) fitted(margin,iy,1u,
                                      i==selected?LIME:WHITE,line,width,height);
            }
        }
        fitted(margin,footer-75,1u,MUTED,
               "A: detalhes / X: favorito / Y: atualizar / B: Inicio",
               width,height);
    } else {
        snprintf(line,sizeof line,"%s",home_labels[home_focus]);
        fitted(margin,top+76*(int)big,big,WHITE,line,width,height);
        fitted(margin,top+132*(int)big,1u,MUTED,
               "Modulo completo disponivel no projeto nativo XDK.",
               width,height);
        fitted(margin,top+177*(int)big,1u,LIME,
               "Esta versao XSDK confirma apenas interface e controle.",
               width,height);
        fitted(margin,top+222*(int)big,1u,MUTED,
               "B volta para o Inicio; Back encerra o teste.",
               width,height);
    }
    fitted(margin,footer,1u,LIME,
           "A: Selecionar    B: Voltar    X: Favorito    Y: Atualizar",
           width,height);
    fitted(margin,footer+25,1u,MUTED,
           "Back: Sair   |   encerramento automatico de seguranca",
           width,height);
    if(page==1)fitted(margin,footer-28,1u,MUTED,info,width,height);
}

int main(void)
{
    cloud_init(&catalogue,items,CLOUD_MAX);
    (void)cloud_favorites_load(&favorites,"game:\\favorites.txt");
    read_playlist();
    xsdk_log_write("aurorax: Alpha XSDK diagnostic begin (no media)");
    NTSTATUS status=xsdk_gpu_open();
    if(!NT_SUCCESS(status)) {
        xsdk_log_write("FAIL: aurorax GPU open=0x%08X",(unsigned)status);
        return 1;
    }
    uint32_t width=0,height=0;
    status=xsdk_present_open(XSDK_DISPLAY_720P,3,1,&width,&height);
    if(!NT_SUCCESS(status)) {
        xsdk_log_write("FAIL: aurorax present open=0x%08X",(unsigned)status);
        (void)xsdk_gpu_close();
        return 2;
    }
    status=xsdk_text_open();
    if(!NT_SUCCESS(status)) {
        xsdk_log_write("FAIL: aurorax text open=0x%08X",(unsigned)status);
        xsdk_present_close();
        (void)xsdk_gpu_close();
        return 3;
    }
    uint16_t previous=0;
    bool failed=false;
    unsigned frame=0;
    for(;frame<MAX_FRAMES;frame++) {
        INPUT_STATE input={0};
        uint16_t buttons=0;
        if(XamInputGetState(0,0,&input)==ERROR_SUCCESS)
            buttons=input.gamepad.buttons;
        uint16_t edge=buttons & (uint16_t)~previous;
        previous=buttons;
        if(edge & INPUT_GAMEPAD_BACK) break;
        if(edge & INPUT_GAMEPAD_B) {
            if(page==0u)break;
            page=0u;
        } else if(page==0u) {
            if(edge & INPUT_GAMEPAD_DPAD_LEFT && (home_focus&1u))
                home_focus--;
            if(edge & INPUT_GAMEPAD_DPAD_RIGHT && !(home_focus&1u))
                home_focus++;
            if(edge & INPUT_GAMEPAD_DPAD_UP && home_focus>=2u)
                home_focus-=2u;
            if(edge & INPUT_GAMEPAD_DPAD_DOWN && home_focus<6u)
                home_focus+=2u;
            if(edge & INPUT_GAMEPAD_A) {
                page=home_focus==1u?1u:2u;
                selected=0u;
                filter_items();
            }
        } else if(page==1u) {
            if(edge & INPUT_GAMEPAD_DPAD_LEFT) {
                tab=(tab+4u)%5u; selected=0; filter_items();
            }
            if(edge & INPUT_GAMEPAD_DPAD_RIGHT) {
                tab=(tab+1u)%5u; selected=0; filter_items();
            }
            if(edge & INPUT_GAMEPAD_DPAD_UP && selected>0u) --selected;
            if(edge & INPUT_GAMEPAD_DPAD_DOWN && selected+1u<count)
                ++selected;
            if(edge & INPUT_GAMEPAD_Y) read_playlist();
            if(edge & INPUT_GAMEPAD_X && count) {
                const CloudItem *item=&catalogue.items[visible[selected]];
                int changed=cloud_favorite_toggle(&favorites,item);
                int saved=cloud_favorites_save(&favorites,"game:\\favorites.txt");
                snprintf(info,sizeof info,"Favorito %s (gravar=%d)",
                         changed>0?"adicionado":"removido",saved);
                filter_items();
            }
            if(edge & INPUT_GAMEPAD_A && count) {
                const CloudItem *item=&catalogue.items[visible[selected]];
                snprintf(info,sizeof info,"Fonte: %s | player ainda nao incluido",
                         cloud_format_label(cloud_format(item->url)));
            }
        }
        uint32_t address=0;
        status=xsdk_present_begin(&address,DISPLAY_WAIT);
        if(!NT_SUCCESS(status)) {
            xsdk_log_write("FAIL: aurorax present begin frame=%u status=0x%08X",
                           frame,(unsigned)status);
            failed=true;
            break;
        }
        xsdk_gpu_set_target(width,height);
        /* Two clean frames before any text. No extra shaders or textures. */
        if(frame>=2u)draw(width,height);
        xsdk_gpu_resolve(address,width,height,BG);
        xsdk_present_end();
        if(frame==3u)xsdk_log_write("PASS: aurorax first frame submitted");
    }
    xsdk_log_write("aurorax: graphics exit frame=%u failed=%u",
                   frame,(unsigned)failed);
    status=xsdk_gpu_wait(xsdk_gpu_fence(),GPU_WAIT);
    xsdk_log_write("%s: aurorax GPU fence=0x%08X",
                   NT_SUCCESS(status)?"PASS":"FAIL",(unsigned)status);
    if(!NT_SUCCESS(status))failed=true;
    xsdk_text_close();
    xsdk_present_close();
    status=xsdk_gpu_close();
    xsdk_log_write("%s: aurorax GPU return to XAM=0x%08X",
                   NT_SUCCESS(status)?"PASS":"FAIL",(unsigned)status);
    return failed||!NT_SUCCESS(status)?4:0;
}
