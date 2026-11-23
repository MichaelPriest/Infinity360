# Cloud360 v0.5.2 - display and Aurora-return investigation

The user reports Cloud360 v0.5.1 garbled the application's interface and
also the Xbox 360/Aurora dashboard on return. Stop running v0.5.1.

## Sources studied
- Orbita public ELF/libxenon GPU presentation: gpu_present_finish(),
  GPU fences before buffer reuse and before giving the framebuffer back:
  https://github.com/sergiolimasks/orbita/blob/main/app/h264play/source/gpu_present.c
- XSDK hardware reports and GPU/XAM state handoff issues:
  https://github.com/xenosworks/xsdk/blob/main/docs/HARDWARE.md
- XSDK documented startup, present, shutdown sequence:
  https://github.com/xenosworks/xsdk/blob/main/docs/GUIDE.md

The public Orbita project is ELF/libxenon, not source-compatible with our
XSDK/XEX. The closed XEX bundle lacks reusable source. No Orbita code or
proprietary binaries were copied into Cloud360.

## Suspected issues
1. The previous UI had fixed 1280x720 positions, regardless of display
   dimensions returned by xsdk_present_open; XSDK can return 640x480.
2. The previous interface rendered text into initial undefined eDRAM content
   before the eDRAM had been cleared through a resolve.
3. The Xbox dashboard also being garbled suggests an additional GPU/XAM
   ownership or teardown issue. The exact cause is NOT proven; a firmware,
   video mode or XSDK hardware compatibility issue may remain.

## Changes
- New responsive, clipped, low-overhead text UI based on actual width/height.
- First two frames clear eDRAM and show no UI.
- Three presentation buffers; explicit GPU fence before shutdown.
- B or Back exits; frame cap of 900 (about 15s at 60Hz).
- No FFmpeg, texture logos, streaming, extra shaders or online services.
- Explicit PASS/FAIL startup and shutdown logs.
- Separate cloud360_probe.xex that never takes over the GPU or display.

## Hardware testing order: DO NOT SKIP
0. Do not run the previous v0.5.1 again. If dashboard is corrupted,
   reboot the Xbox 360 before any further test.
1. Extract zip to a NEW folder Hdd1:/Homebrew/Cloud360-Test-v052/.
2. Run cloud360_probe.xex in Aurora. It exits after about 4 seconds.
3. Confirm the Aurora menu looks completely normal after the return.
   If anything is wrong STOP, reboot and do NOT run default.xex.
4. Only if step 3 passes, run default.xex; it draws simple text for up
   to 900 frames, then exits. Back/B exits sooner.
5. If the dashboard looks wrong after exit, STOP, reboot and do not
   repeat until the log and environment are examined.
6. Read the XSDK logs beside each executable, normally named after it.
   Remove personal information before sharing them.

For investigation we need: Xbox video cable HDMI/component, output mode
720p/1080p or 480i, Aurora version, whether the home menu was distorted
before launching Cloud360, and the new XSDK logs.

Build success only proves XEX generation, not hardware compatibility.

The decoder remains present in the project but disabled in this build.
