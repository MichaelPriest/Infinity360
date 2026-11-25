/* XSDK/newlib compatibility for the isolated FFmpeg diagnostics.
 * newlib's clock() pulls _times_r(), which references the missing times().
 * XSDK exposes monotonic and realtime clocks, not POSIX process CPU accounting.
 * Report ENOSYS rather than fake CPU time or confuse it with wall time.
 */
#include <sys/times.h>
#include <errno.h>
clock_t times(struct tms* buffer){
    (void)buffer;
    errno=ENOSYS;
    return (clock_t)-1;
}
