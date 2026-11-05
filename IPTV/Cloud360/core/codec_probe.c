#include "codec_probe.h"
#include <string.h>
#define TS_SIZE 188
static unsigned be16(const uint8_t *p) { return (unsigned)p[0]*256u+p[1]; }
static CloudCodec stream_codec(uint8_t type, const uint8_t *desc, size_t desc_size) {
    switch(type) {
      case 0x01: case 0x02: return CLOUD_CODEC_MPEG2_VIDEO;
      case 0x1b: return CLOUD_CODEC_H264;
      case 0xea: return CLOUD_CODEC_VC1;
      case 0x24: return CLOUD_CODEC_H265;
      case 0x0f: case 0x11: return CLOUD_CODEC_AAC;
      case 0x03: case 0x04: return CLOUD_CODEC_MP3;
      case 0x81: return CLOUD_CODEC_AC3;
      case 0x87: return CLOUD_CODEC_EAC3;
      case 0x06: {
        size_t i=0;
        while(i+2<=desc_size) {
          size_t len=desc[i+1];
          if(i+2+len>desc_size) break;
          if(desc[i]==0x6a) return CLOUD_CODEC_AC3;
          if(desc[i]==0x7a) return CLOUD_CODEC_EAC3;
          if(desc[i]==0x7c) return CLOUD_CODEC_AAC;
          i+=2+len;
        }
        return CLOUD_CODEC_UNKNOWN;
      }
      default: return CLOUD_CODEC_UNKNOWN;
    }
}
static int is_video(CloudCodec id) {
    return id==CLOUD_CODEC_MPEG2_VIDEO || id==CLOUD_CODEC_H264 ||
      id==CLOUD_CODEC_VC1 || id==CLOUD_CODEC_H265;
}
static int is_audio(CloudCodec id) {
    return id==CLOUD_CODEC_AAC || id==CLOUD_CODEC_MP3 ||
      id==CLOUD_CODEC_AC3 || id==CLOUD_CODEC_EAC3;
}
static int section(const uint8_t *packet, uint8_t *table, const uint8_t **data, size_t *length) {
    if(packet[0]!=0x47) return 0;
    if(!(packet[1]&0x40)) return 0;
    unsigned control=(packet[3]>>4)&3;
    if(control!=1 && control!=3) return 0;
    size_t offset=4;
    if(control==3) offset+=(size_t)packet[4]+1;
    if(offset>=TS_SIZE) return 0;
    offset+=(size_t)packet[offset]+1; /* pointer_field */
    if(offset+3>TS_SIZE) return 0;
    const uint8_t *start=packet+offset;
    size_t size=((size_t)(start[1]&0x0f)<<8)|start[2];
    if(size<9 || size+3>TS_SIZE-offset) return 0; /* no multi-packet PSI yet */
    *table=start[0];*data=start;*length=size+3;return 1;
}
int cloud_probe_mpegts(const uint8_t *data,size_t length,CloudTransportInfo *out) {
    if(!data || !out) return -1;
    memset(out,0,sizeof *out);
    if(length<TS_SIZE) return 0;
    for(size_t pos=0;pos+TS_SIZE<=length;pos+=TS_SIZE) {
      const uint8_t *p=data+pos;
      out->packets_scanned++;
      if(p[0]!=0x47) return 0;
      if(p[1]&0x80) continue;
      unsigned pid=((unsigned)(p[1]&0x1f)<<8)|p[2];
      if(pid!=0 && (!out->pat_found || pid!=out->pmt_pid)) continue;
      uint8_t table=0;const uint8_t *s=NULL;size_t n=0;
      if(!section(p,&table,&s,&n)) continue;
      if(pid==0 && table==0x00) {
        if(n<16 || (s[5]&1)==0) continue;
        size_t end=n-4; /* last four bytes: CRC */
        for(size_t i=8;i+4<=end;i+=4) {
          unsigned program=be16(s+i);
          if(program==0) continue;
          out->pmt_pid=(uint16_t)(((unsigned)(s[i+2]&0x1f)<<8)|s[i+3]);
          out->pat_found=1;break;
        }
      } else if(out->pat_found && pid==out->pmt_pid && table==0x02) {
        if(n<16 || (s[5]&1)==0) continue;
        size_t end=n-4; /* CRC */
        size_t prog_info=(((size_t)s[10]&0x0f)<<8)|s[11];
        size_t cursor=12+prog_info;
        if(cursor>end) continue;
        while(cursor+5<=end) {
          uint8_t type=s[cursor];
          uint16_t elementary=(uint16_t)(((unsigned)(s[cursor+1]&0x1f)<<8)|s[cursor+2]);
          size_t info_length=(((size_t)s[cursor+3]&0x0f)<<8)|s[cursor+4];
          if(cursor+5+info_length>end) break;
          CloudCodec codec=stream_codec(type,s+cursor+5,info_length);
          if(is_video(codec) && !out->video_codec) {
            out->video_codec=codec;out->video_pid=elementary;
          }
          if(is_audio(codec) && !out->audio_codec) {
            out->audio_codec=codec;out->audio_pid=elementary;
          }
          cursor+=5+info_length;
        }
        out->pmt_found=1;
        return 1;
      }
    }
    return 0;
}
const char *cloud_codec_name(CloudCodec c) {
    switch(c){
      case CLOUD_CODEC_MPEG2_VIDEO:return "MPEG-2 Video";
      case CLOUD_CODEC_H264:return "H.264/AVC";
      case CLOUD_CODEC_VC1:return "VC-1";
      case CLOUD_CODEC_H265:return "HEVC/H.265";
      case CLOUD_CODEC_AAC:return "AAC";
      case CLOUD_CODEC_MP3:return "MP3";
      case CLOUD_CODEC_AC3:return "AC-3";
      case CLOUD_CODEC_EAC3:return "E-AC-3";
      case CLOUD_CODEC_UNKNOWN:return "Desconhecido";
      default:return "Nenhum";
    }
}
int cloud_codec_software_candidate(CloudCodec c) {
  return c==CLOUD_CODEC_MPEG2_VIDEO||c==CLOUD_CODEC_H264||
    c==CLOUD_CODEC_VC1||c==CLOUD_CODEC_AAC||
    c==CLOUD_CODEC_MP3||c==CLOUD_CODEC_AC3||c==CLOUD_CODEC_EAC3;
}
