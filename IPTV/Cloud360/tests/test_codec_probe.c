#include "codec_probe.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char ts[188*2];
static void make_pat(void) {
  unsigned char *p=ts;
  memset(p,0xff,188);p[0]=0x47;p[1]=0x40;p[2]=0;p[3]=0x10;p[4]=0;
  unsigned char pat[]={0x00,0xb0,0x0d,0,1,0xc1,0,0, 0,1,0xe1,0x00, 0,0,0,0};
  memcpy(p+5,pat,sizeof pat);
}
static void make_pmt(unsigned char type_video,unsigned char type_audio) {
  unsigned char *p=ts+188;
  memset(p,0xff,188);p[0]=0x47;p[1]=0x41;p[2]=0;p[3]=0x10;p[4]=0;
  unsigned char pmt[]={0x02,0xb0,0x17,0,1,0xc1,0,0,0xe1,0x01,0xf0,0,
    0,0xe1,0x01,0xf0,0,
    0,0xe1,0x02,0xf0,0,
    0,0,0,0};
  pmt[12]=type_video;pmt[17]=type_audio;
  memcpy(p+5,pmt,sizeof pmt);
}
int main(void) {
  CloudTransportInfo t;
  assert(cloud_probe_mpegts(NULL,0,&t)==-1);
  assert(cloud_probe_mpegts(ts,0,&t)==0);
  make_pat();make_pmt(0x1b,0x0f);
  assert(cloud_probe_mpegts(ts,sizeof ts,&t)==1);
  assert(t.pat_found && t.pmt_found);
  assert(t.pmt_pid==256 && t.video_pid==257 && t.audio_pid==258);
  assert(t.video_codec==CLOUD_CODEC_H264 && t.audio_codec==CLOUD_CODEC_AAC);
  assert(cloud_codec_software_candidate(t.video_codec));
  assert(strcmp(cloud_codec_name(t.audio_codec),"AAC")==0);
  make_pmt(0x02,0x81);
  assert(cloud_probe_mpegts(ts,sizeof ts,&t)==1);
  assert(t.video_codec==CLOUD_CODEC_MPEG2_VIDEO && t.audio_codec==CLOUD_CODEC_AC3);
  make_pmt(0x24,0x87);
  assert(cloud_probe_mpegts(ts,sizeof ts,&t)==1);
  assert(t.video_codec==CLOUD_CODEC_H265 && t.audio_codec==CLOUD_CODEC_EAC3);
  assert(!cloud_codec_software_candidate(t.video_codec));
  puts("MPEG-TS PAT/PMT probe and codec profiles: PASS");
  return 0;
}
