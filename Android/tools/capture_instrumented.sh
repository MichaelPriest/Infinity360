#!/usr/bin/env bash
set -euo pipefail
output="$1"
mkdir -p "$output"
package=com.infinity360.remote
[[ "$(adb get-serialno)" == emulator-* ]] || { echo "Disposable emulator required"; exit 1; }
trap 'adb logcat -d -t 2000 > "$output/logcat.txt" || true' EXIT
adb install -r Android/app/build/outputs/apk/debug/app-debug.apk
adb install -r Android/app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb shell pm clear "$package"
adb shell wm dismiss-keyguard
adb shell am instrument -w -r "$package.test/com.infinity360.remote.GalleryRunner" | tee "$output/instrumentation.txt"
grep -q GALLERY_OK "$output/instrumentation.txt"
for image in 01-home 02-controle 03-catalogo 04-loja 05-sistema 06-conexao 07-ftp; do
    adb exec-out run-as "$package" cat "files/gallery/$image.png" > "$output/$image.png"
    test -s "$output/$image.png"
done
