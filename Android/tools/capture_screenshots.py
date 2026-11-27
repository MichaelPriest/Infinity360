"""Capture actual Android APK screens on a disposable, freshly installed emulator."""
import pathlib
import re
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

PACKAGE = "com.infinity360.remote"
ID = PACKAGE + ":id/"
SCREENS = [
    ("01-home.png", "nav_home"),
    ("02-controle.png", "nav_control"),
    ("03-catalogo.png", "nav_catalog"),
    ("04-loja.png", "nav_store"),
    ("05-sistema.png", "nav_system"),
    ("06-conexao.png", "remote_connection_button"),
    ("07-ftp.png", "remote_ftp_button"),
]

def adb(*args):
    return subprocess.check_output(["adb", *args], timeout=60)

def hierarchy():
    adb("shell", "uiautomator", "dump", "/sdcard/infinity-window.xml")
    return ET.fromstring(adb("exec-out", "cat", "/sdcard/infinity-window.xml"))

def wait_node(resource, selected=False):
    deadline = time.monotonic() + 40
    while time.monotonic() < deadline:
        try:
            nodes = hierarchy().iter("node")
            for node in nodes:
                if node.get("resource-id") == ID + resource and (not selected or node.get("selected") == "true"):
                    bounds = list(map(int, re.findall(r"\d+", node.get("bounds", ""))))
                    if len(bounds) == 4 and bounds[2] > bounds[0] and bounds[3] > bounds[1]:
                        return bounds
        except (ET.ParseError, subprocess.CalledProcessError):
            pass
        time.sleep(1)
    raise RuntimeError("Visible control not found: " + resource)

def tap(resource):
    x1, y1, x2, y2 = wait_node(resource)
    adb("shell", "input", "tap", str((x1+x2)//2), str((y1+y2)//2))
    time.sleep(1)

def capture(path):
    data = adb("exec-out", "screencap", "-p")
    if not data.startswith(b"\x89PNG\r\n\x1a\n") or len(data) < 10000:
        raise RuntimeError("Invalid screenshot: " + str(path))
    path.write_bytes(data)

def assert_activity(activity):
    output = adb("shell", "dumpsys", "activity", "activities").decode(errors="replace")
    lines = [line for line in output.splitlines() if "mResumedActivity" in line or "topResumedActivity" in line]
    if not any(PACKAGE + "/." + activity in line or PACKAGE + "/" + PACKAGE + "." + activity in line for line in lines):
        raise RuntimeError("Expected foreground Activity: " + activity)

def main():
    out = pathlib.Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    # Never clear or capture user data from a physical phone.
    if not adb("get-serialno").decode().strip().startswith("emulator-"):
        raise RuntimeError("Use a disposable Android emulator, not a physical device")
    adb("shell", "pm", "clear", PACKAGE)
    adb("shell", "input", "keyevent", "82")
    adb("shell", "am", "start", "-W", "-n", PACKAGE + "/.MainActivity")
    wait_node("nav_home", selected=True)
    for name, resource in SCREENS:
        if resource == "remote_ftp_button":
            tap("nav_system")
            wait_node("nav_system", selected=True)
        if resource != "nav_home":
            tap(resource)
        if resource.startswith("nav_"):
            wait_node(resource, selected=True)
        assert_activity("FtpActivity" if resource == "remote_ftp_button" else "MainActivity")
        if resource == "remote_connection_button":
            root = hierarchy()
            if not any(n.get("class") == "android.widget.EditText" for n in root.iter("node")):
                raise RuntimeError("Connection fields not displayed")
        time.sleep(1)
        capture(out / name)
    print("Captured seven APK screens:", out)

if __name__ == "__main__":
    main()
