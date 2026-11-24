import ftplib
import io
from pathlib import Path
import socket
import subprocess
import sys
import tempfile


def reject(action):
    try:
        action()
    except (ftplib.error_perm, ftplib.error_temp):
        return
    raise AssertionError('Expected FTP permission rejection')


with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    personal = root / 'UserFiles'
    personal.mkdir()
    (personal / 'Documents').mkdir()
    payload = bytes(range(256)) * 4096
    (personal / 'sample.bin').write_bytes(payload)
    (root / 'private.txt').write_text('not shared')
    (personal / 'AXCOPY-0001.part').write_text('temporary')
    (personal / 'link.txt').symlink_to(root / 'private.txt')
    password = 'Loopback-test-only-123'
    server = subprocess.Popen([sys.argv[1], str(root)], stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE, text=True)
    try:
        server.stdin.write(password + '\n')
        server.stdin.flush()
        port = int(server.stdout.readline())
        with ftplib.FTP() as ftp:
            ftp.connect('127.0.0.1', port, timeout=5)
            reject(lambda: ftp.sendcmd('PASV'))
            ftp.login('owner', password)
            assert ftp.pwd() == '/'
            assert sorted(ftp.nlst()) == ['Documents', 'sample.bin']
            entries = []
            ftp.retrlines('LIST', entries.append)
            assert len(entries) == 2
            ftp.voidcmd('TYPE I')
            assert ftp.size('sample.bin') == len(payload)
            received = io.BytesIO()
            ftp.retrbinary('RETR sample.bin', received.write)
            assert received.getvalue() == payload
            # Exercise EPSV explicitly as IPv4 ftplib defaults to PASV.
            host, data_port = ftplib.parse229(ftp.sendcmd('EPSV'), ftp.sock.getpeername())
            with socket.create_connection((host, data_port), timeout=5) as data:
                ftp.sendcmd('RETR sample.bin')
                blocks = []
                while True:
                    chunk = data.recv(65536)
                    if not chunk:
                        break
                    blocks.append(chunk)
            assert ftp.voidresp().startswith('226')
            assert b''.join(blocks) == payload
            reject(lambda: ftp.retrbinary('RETR ../private.txt', lambda _: None))
            reject(lambda: ftp.retrbinary('RETR link.txt', lambda _: None))
            reject(lambda: ftp.retrbinary('RETR AXCOPY-0001.part', lambda _: None))
            reject(lambda: ftp.sendcmd('STOR forbidden.bin'))
            ftp.voidcmd('ALLO ' + str(len(payload)))
            ftp.storbinary('STOR uploaded.bin', io.BytesIO(payload))
            assert (personal / 'uploaded.bin').read_bytes() == payload
            # A second upload must not replace the first file.
            ftp.voidcmd('ALLO 3')
            reject(lambda: ftp.storbinary('STOR uploaded.bin', io.BytesIO(b'bad')))
            assert (personal / 'uploaded.bin').read_bytes() == payload
            ftp.voidcmd('ALLO 9')
            reject(lambda: ftp.storbinary('STOR incomplete.bin', io.BytesIO(b'abc')))
            assert not (personal / 'incomplete.bin').exists()
            ftp.voidcmd('ALLO 2')
            reject(lambda: ftp.storbinary('STOR excess.bin', io.BytesIO(b'abc')))
            assert not (personal / 'excess.bin').exists()
            ftp.voidcmd('ALLO 3')
            reject(lambda: ftp.storbinary('STOR ../private.txt', io.BytesIO(b'bad')))
            reject(lambda: ftp.sendcmd('ALLO 33554433'))
            assert not list(personal.glob('AXCOPY-FTP-*.part'))
            reject(lambda: ftp.sendcmd('PORT 127,0,0,1,1,1'))
            ftp.cwd('Documents')
            assert ftp.pwd() == '/Documents'
            ftp.cwd('..')
            assert ftp.pwd() == '/'
            reject(lambda: ftp.sendcmd('CWD /../'))
            ftp.quit()
        assert server.wait(timeout=5) == 0
        assert not (personal / 'forbidden.bin').exists()
        assert (root / 'private.txt').read_text() == 'not shared'
    finally:
        if server.poll() is None:
            server.kill()
            server.wait()
print('FTP loopback integration passed')
