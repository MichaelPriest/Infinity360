// Read-only production C++ smoke test. No GPU, no network, no game launch.
#include "HomeCanvas.h"
#include "MediaUrl.h"
#include "TvCatalog.h"
#include "GameScanner.h"
#include "xsdk/log.h"
#include <vector>

int main() {
    xsdk_log_write("INFINITY 360: production C++ probe (no GPU)");
    HomeCanvas canvas;
    if (!canvas.LoadArt("game:\\home.tga")) {
        xsdk_log_write("FAIL: production artwork could not be read");
        return 1;
    }
    canvas.Background();
    if (canvas.Pixels()[0] != 0xff031310u) {
        xsdk_log_write("FAIL: background pixel");
        return 2;
    }
    canvas.Box(10,10,40,40,0xff9dec31u);
    canvas.Text(80,80,"INFINITY 360",24,0xfff2f7f3u,400);
    if (canvas.Pixels()[20*HomeCanvas::Width+20] != 0xff9dec31u) {
        xsdk_log_write("FAIL: canvas drawing");
        return 3;
    }
    MediaUrl url;
    if (!url.Parse("https://example.invalid/live.m3u8") || !url.secure ||
        url.port != 443 || !url.NeedsConverter() || url.Parse("file:///bad")) {
        xsdk_log_write("FAIL: URL classification");
        return 4;
    }
    TvCatalog catalog;
    if (!catalog.Load("game:\\probe-playlist.m3u") || catalog.Items().size()!=1u ||
        catalog.Items()[0].name!="INFINITY Probe") {
        xsdk_log_write("FAIL: production M3U catalog");
        return 5;
    }
    // Exercise directory listing only, not an unbounded scan of user media.
    // XSDK newlib paths use a device prefix; try both separator spellings.
    std::vector<FolderEntry> entries;
    if (!ReadFolder("game:/",entries) && !ReadFolder("game:\\",entries)) {
        xsdk_log_write("FAIL: production filesystem directory listing");
        return 6;
    }
    xsdk_log_write("PASS: canvas, artwork, URL parser, M3U, directory listing (%u entries)",
                   static_cast<unsigned>(entries.size()));
    return 0;
}
