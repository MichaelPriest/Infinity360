#ifndef CLOUD360_H
#define CLOUD360_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define CLOUD_MAX 1500
typedef struct {
  char name[128], group[96], url[2048], logo[256], tvg_id[128];
  uint64_t playlist_id; /* Preserves favorites from older M3U title parsing. */
} CloudItem;
typedef struct {
  CloudItem *items;
  size_t count, capacity;
} CloudCatalog;
typedef enum { CLOUD_ALL, CLOUD_LIVE, CLOUD_MOVIES, CLOUD_SERIES, CLOUD_FAVORITES } CloudTab;
typedef enum { CLOUD_UNKNOWN, CLOUD_HLS, CLOUD_TS, CLOUD_MP4, CLOUD_AUDIO, CLOUD_DASH, CLOUD_HTTP } CloudFormat;
typedef struct { uint64_t ids[CLOUD_MAX]; size_t count; } CloudFavorites;
void cloud_init(CloudCatalog *cat, CloudItem *buffer, size_t capacity);
/* The visitor may return nonzero to stop early. Parser errors are negative. */
typedef int (*CloudItemVisitor)(const CloudItem *item, void *context);
int cloud_visit_m3u(const char *path, CloudItemVisitor visit, void *context);
int cloud_load_m3u(CloudCatalog *cat, const char *path);
CloudFormat cloud_format(const char *url);
const char *cloud_format_label(CloudFormat f);
CloudTab cloud_item_type(const CloudItem *item);
uint64_t cloud_item_id(const CloudItem *item);
int cloud_favorite_has_id(const CloudFavorites *f, uint64_t id);
int cloud_favorite_toggle_id(CloudFavorites *f, uint64_t id);
int cloud_favorite_has(const CloudFavorites *f, const CloudItem *item);
int cloud_favorite_toggle(CloudFavorites *f, const CloudItem *item);
int cloud_favorites_load(CloudFavorites *f, const char *path);
int cloud_favorites_save(const CloudFavorites *f, const char *path);
size_t cloud_filter(const CloudCatalog *cat, CloudTab tab, const CloudFavorites *f,
                    const char *group, const char *query, size_t *indices, size_t cap);
#ifdef __cplusplus
}
#endif
#endif
