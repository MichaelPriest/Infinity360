#include "cloud360.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static CloudItem items[8];
int main(void) {
  CloudCatalog cat;
  cloud_init(&cat,items,8);
  assert(cloud_load_m3u(&cat,CLOUD360_FIXTURE)==0);
  assert(cat.count==3);
  assert(!strcmp(cat.items[0].name,"Natureza Demo"));
  assert(!strcmp(cat.items[0].group,"Documentarios"));
  assert(!strcmp(cat.items[0].tvg_id,"demo.nature"));
  assert(cloud_format(items[0].url)==CLOUD_HLS);
  assert(cloud_format("https://example.org/channel.ts?token=1")==CLOUD_TS);
  assert(cloud_format("rtmp://example.org/video")==CLOUD_UNKNOWN);
  CloudFavorites f={0};
  assert(cloud_favorite_toggle(&f,&items[0])==1);
  assert(cloud_favorite_has(&f,&items[0]));
  size_t indices[8];
  assert(cloud_filter(&cat,CLOUD_FAVORITES,&f,NULL,NULL,indices,8)==1);
  assert(cloud_filter(&cat,CLOUD_ALL,&f,"Educacao",NULL,indices,8)==1);
  assert(cloud_filter(&cat,CLOUD_ALL,&f,NULL,"Radio",indices,8)==1);
  assert(cloud_favorite_toggle(&f,&items[0])==0);
  puts("OK - M3U parser, stream detection, favorites and filters");
  return 0;
}
