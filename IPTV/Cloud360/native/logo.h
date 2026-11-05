#ifndef CLOUD360_NATIVE_LOGO_H
#define CLOUD360_NATIVE_LOGO_H
#include <stddef.h>
/* Optional prepared RGBA thumbnail for the selected channel (128x72).
 * Files are generated on the PC at logos/<catalog_index>.rgba.
 * 0 success, negative value means unavailable or unsupported.
 */
int cloud_logo_init(void);
int cloud_logo_select(size_t channel_index);
void cloud_logo_draw(void);
void cloud_logo_close(void);
#endif
