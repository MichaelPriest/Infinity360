/*
 * Offline channel logos on Xbox 360/XSDK.
 * No HTTP client or PNG/JPEG decoder is required on the console:
 * prepare_logos.py downloads authorized assets and converts them to 128x72 RGBA.
 */
#include "logo.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "logo_shaders.h"
#include "xsdk/kernel.h"
#include "xsdk/gpu.h"
#include "xsdk/shader.h"
#include "xsdk/texture.h"
#include "xsdk/log.h"

enum { LOGO_WIDTH=128, LOGO_HEIGHT=72, LOGO_ROW_BYTES=LOGO_WIDTH*4, LOGO_DATA_BYTES=LOGO_HEIGHT*LOGO_ROW_BYTES, LOGO_VERTEX_WORDS=16 };
static XsdkShader vs,ps;
static XsdkTexture texture;
static XsdkSamplerState sampler;
static volatile uint32_t *vertices;
static bool ready=false, available=false;
static size_t loaded_index=(size_t)-1;
static unsigned char rgba[LOGO_DATA_BYTES];

static const XsdkVertexElement layout[] = {
  { .usage=XSDK_USAGE_POSITION, .format=XSDK_VERTEX_FLOAT2, .offset_bytes=0 },
  { .usage=XSDK_USAGE_TEXCOORD, .format=XSDK_VERTEX_FLOAT2, .offset_bytes=8 }
};
static void write_vertices(void) {
  /* 720p, upper right; quad in clip space, CCW triangle strip. */
  const float left=0.68f, right=0.88f, top=0.78f, bottom=0.58f;
  const float v[16]={left,top,0,0, right,top,1,0, left,bottom,0,1, right,bottom,1,1};
  for(unsigned i=0;i<LOGO_VERTEX_WORDS;i++) {
    uint32_t word; memcpy(&word,&v[i],sizeof word);
    vertices[i]=word;
  }
  xsdk_gpu_invalidate(xsdk_gpu_address(vertices),sizeof v);
}
int cloud_logo_init(void) {
  if(ready) return 0;
  NTSTATUS status=xsdk_shader_load(&vs,logo_vert.container,logo_vert.container_bytes);
  if(!NT_SUCCESS(status)) return -1;
  status=xsdk_shader_load(&ps,logo_frag.container,logo_frag.container_bytes);
  if(!NT_SUCCESS(status)) { xsdk_shader_free(&vs);return -2; }
  status=xsdk_shader_set_vertex_layout(&vs,layout,2,(const uint32_t[]){16});
  if(!NT_SUCCESS(status)) { xsdk_shader_free(&ps);xsdk_shader_free(&vs);return -3; }
  status=xsdk_texture_create(&texture,XSDK_FORMAT_R8G8B8A8,LOGO_WIDTH,LOGO_HEIGHT,1,true);
  if(!NT_SUCCESS(status)) {xsdk_shader_free(&ps);xsdk_shader_free(&vs);return -4;}
  vertices=xsdk_gpu_alloc(sizeof(float)*LOGO_VERTEX_WORDS);
  if(!vertices) {xsdk_texture_free(&texture);xsdk_shader_free(&ps);xsdk_shader_free(&vs);return -5;}
  write_vertices();
  const XsdkSampler s={.mag_filter=XSDK_FILTER_LINEAR,.min_filter=XSDK_FILTER_LINEAR,
    .mip_filter=XSDK_FILTER_POINT,.address_u=XSDK_ADDRESS_CLAMP,.address_v=XSDK_ADDRESS_CLAMP};
  xsdk_texture_sampler_state(&sampler,&s);
  ready=true;return 0;
}
int cloud_logo_select(size_t channel_index) {
  if(!ready) return -1;
  if(loaded_index==channel_index) return available?0:-2;
  loaded_index=channel_index;
  available=false;
  char path[80];
  snprintf(path,sizeof path,"game:/logos/%u.rgba",(unsigned)channel_index);
  FILE *f=fopen(path,"rb");
  if(!f) return -3;
  size_t n=fread(rgba,1,sizeof rgba,f);
  int extra=fgetc(f);
  fclose(f);
  if(n!=sizeof rgba || extra!=EOF) return -4;
  /* A previous render may still be reading this texture. Wait before overwriting. */
  NTSTATUS status=xsdk_gpu_wait(xsdk_gpu_fence(),1000);
  if(!NT_SUCCESS(status)) return -5;
  xsdk_texture_upload(&texture,0,rgba,LOGO_ROW_BYTES);
  xsdk_gpu_invalidate(texture.address,texture.size_bytes);
  available=true;
  return 0;
}
void cloud_logo_draw(void) {
  if(!ready || !available) return;
  xsdk_gpu_set_shaders(&vs,&ps);
  xsdk_built_shader_set_literals(&logo_vert);
  xsdk_built_shader_set_literals(&logo_frag);
  xsdk_gpu_set_texture(LOGO_FRAG_SAMPLER_0_0,&texture,&sampler);
  xsdk_gpu_set_vertices(0,xsdk_gpu_address(vertices),sizeof(float)*LOGO_VERTEX_WORDS);
  xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_STRIP,4);
}
void cloud_logo_close(void) {
  if(!ready) return;
  (void)xsdk_gpu_wait(xsdk_gpu_fence(),1000);
  xsdk_gpu_free(vertices);
  xsdk_texture_free(&texture);
  xsdk_shader_free(&ps);
  xsdk_shader_free(&vs);
  ready=false; available=false;loaded_index=(size_t)-1;
}
