"""Exercise real HTTP media and the same streaming decoder linked into the XDK target."""
import concurrent.futures
import functools
import importlib.util
import os
import shutil
import ssl
import subprocess
import sys
import tempfile
import threading
from contextlib import contextmanager
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from unittest.mock import patch
from urllib.error import HTTPError
from urllib.request import urlopen

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("stream_relay", ROOT / "IPTV/Cloud360/tools/stream_relay.py")
relay = importlib.util.module_from_spec(spec)
spec.loader.exec_module(relay)
PROBE, FFMPEG = sys.argv[1:3]


class Origin(SimpleHTTPRequestHandler):
    def log_message(self, *_):
        pass

    def do_GET(self):
        if self.path == "/clip.mp4" and self.headers.get("Range", "").startswith("bytes="):
            # A non-faststart MP4 needs byte ranges to read its moov atom over HTTP.
            data = (Path(self.directory) / "clip.mp4").read_bytes()
            first, last = self.headers["Range"][6:].split("-", 1)
            offset, end = int(first), int(last) if last else len(data) - 1
            if not 0 <= offset <= end < len(data):
                self.send_error(416)
                return
            self.send_response(206)
            self.send_header("Content-Type", "video/mp4")
            self.send_header("Content-Length", str(end - offset + 1))
            self.send_header("Content-Range", f"bytes {offset}-{end}/{len(data)}")
            self.end_headers()
            try:
                self.wfile.write(data[offset:end + 1])
            except (BrokenPipeError, ConnectionResetError):
                pass
            return
        if self.path != "/live":
            return super().do_GET()
        body = (Path(self.directory) / "direct.mpg").read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", "video/mpeg")
        self.end_headers()
        split = len(body) * 2 // 3
        self.wfile.write(body[:split])
        self.wfile.flush()
        self.server.release.wait(10)
        self.wfile.write(body[split:])


@contextmanager
def running(server):
    with server:
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        try:
            yield
        finally:
            server.shutdown()
            worker.join()


def encode(*args):
    subprocess.run([FFMPEG, "-hide_banner", "-loglevel", "error", "-nostdin", "-y", *args],
                   check=True, timeout=25)


def pump(url, proc):
    try:
        with urlopen(url, timeout=10) as response:
            shutil.copyfileobj(response, proc.stdin, 4096)
    finally:
        proc.stdin.close()


def play(url, release=None, audio=True):
    with subprocess.Popen([PROBE], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:
        with concurrent.futures.ThreadPoolExecutor(2) as pool:
            writer = pool.submit(pump, url, proc)
            try:
                first = pool.submit(proc.stderr.readline).result(timeout=8)
                if first.strip() != b"frame":
                    writer.result(timeout=5)
                    raise AssertionError(first)
                if release:
                    assert not writer.done(), "a primeira imagem chegou depois do fim da transmissão"
                    release.set()
                writer.result(timeout=12)
                proc.wait(timeout=8)
            finally:
                if release:
                    release.set()
                if proc.poll() is None:
                    proc.kill()
        assert proc.returncode == 0, proc.stderr.read().decode()
        result = proc.stdout.read().decode().strip()
    frames, changes, samples, nonzero, crossings, rate, width, height = map(int, result.split())
    assert 47 <= frames <= 51 and changes >= 44, result
    assert (width, height) == (640, 360), result
    if audio:
        assert rate == 44100 and 85000 <= samples <= 93000 and nonzero > 60000, result
        assert 840 <= crossings <= 960, result  # 440 Hz for approximately two seconds
    else:
        assert (rate, samples) == (0, 0), result
    return result


def status(url):
    try:
        with urlopen(url, timeout=12) as resp:
            return resp.status
    except HTTPError as exc:
        return exc.code


def https_playback(folder):
    openssl = shutil.which("openssl")
    if not openssl:
        return
    cert, key = folder / "cert.pem", folder / "key.pem"
    subprocess.run([openssl, "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
                    "-subj", "/CN=localhost", "-addext", "subjectAltName=DNS:localhost",
                    "-keyout", str(key), "-out", str(cert)], check=True, capture_output=True)
    origin = ThreadingHTTPServer(("127.0.0.1", 0), functools.partial(Origin, directory=str(folder)))
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(cert, key)
    origin.socket = context.wrap_socket(origin.socket, server_side=True)
    source = f"https://localhost:{origin.server_port}/live.m3u8"
    channel = "1" * 32
    local = relay.Relay("127.0.0.1", 0, [(channel, source, "#EXTINF:-1,HTTPS")], FFMPEG)
    url = f"http://127.0.0.1:{local.server_port}/channel/{channel}.mpg"
    with running(origin), running(local):
        assert status(url) == 502, "a origem sem certificado confiável precisa falhar"
        with patch.dict(os.environ, {"SSL_CERT_FILE": str(cert)}):
            print("HLS por HTTPS", play(url))


def test(folder):
    clip = str(folder / "clip.mp4")
    encode("-f", "lavfi", "-i", "testsrc2=size=640x360:rate=25",
           "-f", "lavfi", "-i", "sine=frequency=440:sample_rate=44100", "-t", "2",
           "-c:v", "libx264", "-preset", "ultrafast", "-g", "10", "-threads", "2", "-c:a", "aac", clip)
    encode("-i", clip, "-c", "copy", "-hls_time", "0.4", "-hls_list_size", "0", str(folder/"live.m3u8"))
    encode("-i", clip, "-c", "copy", "-f", "mpegts", str(folder/"station.ts"))
    encode("-i", clip, "-c:v", "mpeg1video", "-b:v", "1500k", "-bf", "0", "-c:a", "mp2",
           "-ar", "44100", "-ac", "2", "-f", "mpeg", str(folder/"direct.mpg"))
    encode("-i", clip, "-an", "-c:v", "mpeg1video", "-b:v", "1500k", "-f", "mpeg", str(folder/"silent.mpg"))
    handler = functools.partial(Origin, directory=str(folder))
    origin = ThreadingHTTPServer(("127.0.0.1", 0), handler)
    origin.release = threading.Event()
    base = f"http://127.0.0.1:{origin.server_port}"
    m3u = folder / "source.m3u"
    paths = ["live.m3u8", "clip.mp4", "station.ts", "not-video.html"]
    (folder / paths[-1]).write_text("<html>indisponível</html>", encoding="utf-8")
    m3u.write_text("#EXTM3U\n" + "".join(
        f'#EXTINF:-1 tvg-id="station" group-title="TV, Filmes",Canal {i}, parte 2\n{base}/{path}\n'
        for i, path in enumerate(paths)
    ) + "#EXTINF:-1,Referência\nbrazucaplay:sample\n", encoding="utf-8")
    entries, skipped = relay.read_playlist(m3u)
    assert len(entries) == 4 and skipped == 1
    server = relay.Relay("127.0.0.1", 0, entries, FFMPEG)
    with running(origin), running(server):
        print("HTTP ao vivo", play(base+"/live", origin.release))
        print("Vídeo sem áudio", play(base+"/silent.mpg", audio=False))
        relay_url = f"http://127.0.0.1:{server.server_port}"
        with urlopen(relay_url+"/playlist.m3u", timeout=5) as response:
            listing = response.read().decode()
        assert base not in listing and 'tvg-id="station" group-title="TV, Filmes",Canal 0, parte 2' in listing
        urls = [line for line in listing.splitlines() if line.startswith("http://")]
        for kind, url in zip(["HLS", "MP4", "TS"], urls[:3]):
            print(kind, play(url))
        assert status(urls[3]) == 502 and status(relay_url+"/channel/unknown") == 404
    https_playback(folder)
    bad = subprocess.run([PROBE], input=b"<html>erro</html>", capture_output=True, timeout=4)
    assert bad.returncode == 1 and b"MPEG-PS" in bad.stderr


if __name__ == "__main__":
    with tempfile.TemporaryDirectory() as tmp:
        test(Path(tmp))
