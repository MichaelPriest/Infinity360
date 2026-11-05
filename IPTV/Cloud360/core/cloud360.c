#include "cloud360.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
static void copy(char *dst, size_t n, const char *src) {
  if(n) snprintf(dst,n,"%s",src?src:"");
}
static void trim(char *s) {
  size_t n=strlen(s),i=0;
  while(n && isspace((unsigned char)s[n-1])) s[--n]=0;
  while(s[i] && isspace((unsigned char)s[i])) i++;
  if(i) memmove(s,s+i,strlen(s+i)+1);
}
static void attribute(const char *line,const char *key,char *out,size_t cap) {
  out[0]=0;
  size_t n=strlen(key);
  const char *p=line;
  while((p=strstr(p,key))) {
    if(p[n]=='=') {
      p+=n+1; char quote=0;
      if(*p=='"' || *p=='\'') quote=*p++;
      size_t i=0;
      while(*p && i+1<cap && (quote?*p!=quote:!isspace((unsigned char)*p))) out[i++]=*p++;
      out[i]=0; return;
    }
    p+=n;
  }
}
void cloud_init(CloudCatalog *cat,CloudItem *buffer,size_t cap) {
  if(cat) { cat->items=buffer; cat->capacity=cap; cat->count=0; }
}
int cloud_load_m3u(CloudCatalog *cat,const char *path) {
  if(!cat || !cat->items || !cat->capacity || !path) return -1;
  FILE *f=fopen(path,"rb"); if(!f) return -3;
  cat->count=0;
  char line[4096]; CloudItem pending={0}; int meta=0;
  while(fgets(line,sizeof line,f)) {
    trim(line);
    if(!strncmp(line,"#EXTINF:",8)) {
      memset(&pending,0,sizeof pending);
      attribute(line,"group-title",pending.group,sizeof pending.group);
      attribute(line,"tvg-logo",pending.logo,sizeof pending.logo);
      attribute(line,"tvg-id",pending.tvg_id,sizeof pending.tvg_id);
      const char *p=line,*title=NULL; char quote=0;
      while(*p) {
        if((*p=='"' || *p=='\'') && (!quote || *p==quote)) quote=quote?0:*p;
        if(*p==',' && !quote) title=p+1;
        p++;
      }
      if(title) copy(pending.name,sizeof pending.name,title);
      trim(pending.name); meta=1;
    } else if(!strncmp(line,"#EXTGRP:",8) && meta) {
      copy(pending.group,sizeof pending.group,line+8);
    } else if(meta && line[0] && line[0]!='#') {
      if(cat->count>=cat->capacity) { fclose(f); return -2; }
      copy(pending.url,sizeof pending.url,line);
      if(!pending.name[0]) copy(pending.name,sizeof pending.name,"Sem nome");
      if(!pending.group[0]) copy(pending.group,sizeof pending.group,"Outros");
      cat->items[cat->count++]=pending; meta=0;
    }
  }
  int fail=ferror(f); fclose(f); return fail?-3:0;
}
static int contains(const char *a,const char *b) {
  if(!b || !*b) return 1;
  if(!a) return 0;
  for(;*a;a++) {
    const char *x=a,*y=b;
    while(*x && *y && tolower((unsigned char)*x)==tolower((unsigned char)*y)) x++,y++;
    if(!*y) return 1;
  }
  return 0;
}
static int suffix(const char *url,const char *ext) {
  if(!url) return 0;
  const char *end=url; while(*end && *end!='?' && *end!='#') end++;
  size_t n=strlen(ext); if((size_t)(end-url)<n) return 0;
  for(const char *a=end-n,*b=ext;a<end;a++,b++)
    if(tolower((unsigned char)*a)!=tolower((unsigned char)*b)) return 0;
  return 1;
}
CloudFormat cloud_format(const char *url) {
  if(!url || (strncmp(url,"https://",8) && strncmp(url,"http://",7))) return CLOUD_UNKNOWN;
  if(suffix(url,".m3u8")) return CLOUD_HLS;
  if(suffix(url,".ts")) return CLOUD_TS;
  if(suffix(url,".mp4") || suffix(url,".m4v")) return CLOUD_MP4;
  if(suffix(url,".mp3") || suffix(url,".aac")) return CLOUD_AUDIO;
  if(suffix(url,".mpd")) return CLOUD_DASH;
  return CLOUD_HTTP;
}
const char *cloud_format_label(CloudFormat f) {
  switch(f) {
    case CLOUD_HLS:return "HLS"; case CLOUD_TS:return "MPEG-TS";
    case CLOUD_MP4:return "MP4"; case CLOUD_AUDIO:return "AUDIO";
    case CLOUD_DASH:return "DASH"; case CLOUD_HTTP:return "HTTP";
    default:return "DESCONHECIDO";
  }
}
CloudTab cloud_item_type(const CloudItem *item) {
  if(!item) return CLOUD_ALL;
  if(contains(item->group,"serie") || contains(item->group,"série") ||
     contains(item->group,"temporada")) return CLOUD_SERIES;
  if(contains(item->group,"filme") || contains(item->group,"movie") ||
     contains(item->group,"vod") || suffix(item->url,".mp4") ||
     suffix(item->url,".mkv")) return CLOUD_MOVIES;
  return CLOUD_LIVE;
}
static uint64_t item_id(const CloudItem *item) {
  if(!item) return 0;
  uint64_t h=UINT64_C(14695981039346656037);
  const char *v[]={item->name,item->url};
  for(size_t i=0;i<2;i++) {
    const unsigned char *p=(const unsigned char*)v[i];
    while(*p) { h^=*p++; h*=UINT64_C(1099511628211); }
    h^=255; h*=UINT64_C(1099511628211);
  }
  return h;
}
int cloud_favorite_has(const CloudFavorites *f,const CloudItem *item) {
  if(!f || !item) return 0;
  uint64_t id=item_id(item);
  for(size_t i=0;i<f->count;i++) if(f->ids[i]==id) return 1;
  return 0;
}
int cloud_favorite_toggle(CloudFavorites *f,const CloudItem *item) {
  if(!f || !item) return -1;
  uint64_t id=item_id(item);
  for(size_t i=0;i<f->count;i++) if(f->ids[i]==id) {
    for(size_t j=i+1;j<f->count;j++) f->ids[j-1]=f->ids[j];
    f->count--; return 0;
  }
  if(f->count>=CLOUD_MAX) return -2;
  f->ids[f->count++]=id; return 1;
}
int cloud_favorites_load(CloudFavorites *f,const char *path) {
  if(!f || !path) return -1;
  FILE *file=fopen(path,"rb"); if(!file) return -3;
  f->count=0; char line[128];
  while(fgets(line,sizeof line,file) && f->count<CLOUD_MAX) {
    unsigned long long raw; char extra;
    if(sscanf(line,"%16llx %c",&raw,&extra)==1) f->ids[f->count++]=(uint64_t)raw;
  }
  int fail=ferror(file); fclose(file); return fail?-4:0;
}
int cloud_favorites_save(const CloudFavorites *f,const char *path) {
  if(!f || !path) return -1;
  FILE *file=fopen(path,"wb"); if(!file) return -3;
  for(size_t i=0;i<f->count;i++)
    if(fprintf(file,"%016llx\n",(unsigned long long)f->ids[i])<0) { fclose(file);return -4; }
  return fclose(file)?-4:0;
}
size_t cloud_filter(const CloudCatalog *cat,CloudTab tab,const CloudFavorites *f,
                    const char *group,const char *query,size_t *indices,size_t cap) {
  if(!cat || (!indices && cap)) return 0;
  size_t count=0;
  for(size_t i=0;i<cat->count;i++) {
    const CloudItem *item=&cat->items[i];
    if(tab==CLOUD_FAVORITES?!cloud_favorite_has(f,item):
       (tab!=CLOUD_ALL && cloud_item_type(item)!=tab)) continue;
    if(group && *group && !contains(item->group,group)) continue;
    if(!contains(item->name,query) && !contains(item->group,query)) continue;
    if(count==cap) break;
    indices[count++]=i;
  }
  return count;
}
