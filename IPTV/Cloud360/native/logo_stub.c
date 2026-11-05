/* Smoke build without GPU texture shaders. Keeps UI and playlist usable.
 * The full logo renderer is enabled with -DCLOUD360_WITH_LOGOS=ON.
 */
#include "logo.h"
#include <stddef.h>
int cloud_logo_init(void) { return -1; }
int cloud_logo_select(size_t channel_index) { (void)channel_index; return -1; }
void cloud_logo_draw(void) {}
void cloud_logo_close(void) {}
