"""Exercise the installer and independently read the SVOD and XDVDFS formats on disk."""
import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

PROBE = Path(sys.argv[1]).resolve()
BLOCK = 4096
SECTOR = 2048
GROUP = 204
PART = GROUP * 203

def sha(data):
    return hashlib.sha1(data).digest()

def xex(disc_number):
    raw = bytearray(9123)
    raw[:4] = b'XEX2'
    struct.pack_into('>I', raw, 8, 0x400)
    struct.pack_into('>IIII', raw, 0x10, 0x200, 1, 0x40006, 0x80)
    struct.pack_into('>IIIIBBBBI', raw, 0x80, 0x12121212, 0x01020304, 0x01010000,
                     0x12345678, 2, 1, disc_number, 2, 0x98765432)
    return raw

def run(base, mode, expected=None):
    p = subprocess.run([PROBE, base, mode], text=True, capture_output=True)
    if expected:
        assert p.returncode == 1 and expected in p.stderr, (mode, p.returncode, p.stdout, p.stderr)
        assert not list((base / 'UserFiles/PackageLibrary').glob('AXCOPY-*'))
        return
    assert p.returncode == 0, (mode, p.stdout, p.stderr)
    return Path(p.stdout.strip())

def unpack(header_path, image, expected_parts):
    header = header_path.read_bytes()
    assert len(header) == 0xb000 and header[:4] == b'LIVE'
    assert header[4:0x22c] == bytes(0x228) and header[0x22c:0x32c] == bytes(0x100)
    assert header[0x32c:0x340] == sha(header[0x344:])
    assert struct.unpack_from('>III', header, 0x340) == (0xad0e, 0x7000, 2)
    assert header[0x379] == 0x24 and header[0x391] == 0
    assert header[0x354:0x36c] == xex(header[0x366])[0x80:0x98]
    blocks = int.from_bytes(header[0x392:0x395], 'little')
    size, = struct.unpack_from('>Q', header, 0x34c)
    assert blocks * BLOCK == size
    count, stored_bytes, volume_type = struct.unpack_from('>IQI', header, 0x39d)
    assert count == expected_parts and volume_type == 1
    folder = Path(str(header_path) + '.data')
    assert sum(f.stat().st_size for f in folder.iterdir()) == stored_bytes
    expected = header[0x37d:0x391]
    with image.open('wb') as out:
        for part in range(count):
            with (folder / f'Data{part:04}').open('rb') as f:
                master = f.read(BLOCK)
                assert sha(master) == expected
                for g in range(203):
                    if not blocks:
                        break
                    table = f.read(BLOCK)
                    assert sha(table) == master[g*20:g*20+20]
                    n = min(blocks, GROUP)
                    data = f.read(n * BLOCK)
                    assert len(data) == n * BLOCK
                    assert b''.join(sha(data[i:i+BLOCK]) for i in range(0, len(data), BLOCK)) == table[:n*20]
                    assert not any(table[n*20:])
                    out.write(data)
                    blocks -= n
                expected = master[203*20:204*20]
                assert not f.read(1)
    assert not blocks and expected == bytes(20) and image.stat().st_size == size

def verify_tree(image, disc):
    with image.open('rb') as f:
        def read(offset, length):
            f.seek(offset)
            raw = f.read(length)
            assert len(raw) == length
            return raw

        def verify_directory(sector, size, path):
            table = read(sector * SECTOR, size)
            found = []
            visited = set()
            def entry(pos):
                assert pos not in visited and pos < size
                visited.add(pos)
                left, right, start, length, flags, n = struct.unpack_from('<HHIIBB', table, pos)
                assert pos % SECTOR + 14 + n <= SECTOR and n
                name = table[pos+14:pos+14+n].decode('ascii')
                if left:
                    entry(left * 4)
                found.append(name)
                item = path / name
                if flags & 0x10:
                    assert item.is_dir()
                    verify_directory(start, length, item)
                else:
                    assert item.stat().st_size == length
                    with item.open('rb') as src:
                        for offset in range(0, length, 65536):
                            expected = src.read(min(65536, length - offset))
                            assert read(start * SECTOR + offset, len(expected)) == expected
                if right:
                    entry(right * 4)
            if size:
                entry(0)
            assert found == sorted((item.name for item in path.iterdir()), key=str.upper), path

        descriptor = read(32 * SECTOR, SECTOR)
        assert descriptor[:20] == descriptor[-20:] == b'MICROSOFT*XBOX*MEDIA'
        verify_directory(*struct.unpack_from('<II', descriptor, 20), disc)

with tempfile.TemporaryDirectory() as tmp:
    base = Path(tmp)
    disc = base / 'dvd'
    (disc / '$SystemUpdate').mkdir(parents=True)
    (disc / 'empty').mkdir()
    (disc / 'default.xex').write_bytes(xex(1))
    (disc / '$SystemUpdate' / 'test+file.bin').write_bytes(b'nested content')
    (disc / 'empty.file').touch()
    for i in range(111):
        (disc / f'entry-{i:03}-some-padding.bin').write_bytes(bytes([i]) * (i % 4))
    (disc / 'data.bin').write_bytes(bytes(range(251)) * 6200)
    run(base, 'normal', 'Habilite')
    (base / 'enable-dvd-install.txt').write_text('ENABLE-DVD-INSTALL\n')
    run(base, 'wrong', 'alterado')
    for mode in ('cancel-plan', 'cancel-write', 'cancel-verify'):
        run(base, mode, 'cancelada')
    run(base, 'damage', 'divergente')
    run(base, 'swap', 'mudou')
    (disc / 'default.xex').write_bytes(xex(1))
    (disc / 'alias').symlink_to(disc / 'default.xex')
    run(base, 'normal', 'indisponivel')
    (disc / 'alias').unlink()
    assert not list((base / 'UserFiles/PackageLibrary').iterdir())
    # Cross the real 203 x 204 block fragment boundary with a sparse test input.
    with (disc / 'large.bin').open('wb') as f:
        f.truncate(PART * BLOCK + 317)
        f.seek(PART * BLOCK - 14); f.write(b'bytes across the fragment boundary')
    first = run(base, 'normal')
    original = first.read_bytes()
    unpack(first, base / 'reconstructed.iso', 2)
    verify_tree(base / 'reconstructed.iso', disc)
    run(base, 'normal', 'ja existe')
    assert first.read_bytes() == original
    (disc / 'large.bin').unlink()
    (disc / 'default.xex').write_bytes(xex(2))
    second = run(base, 'normal')
    assert first != second and first.read_bytes() == original
    unpack(second, base / 'reconstructed.iso', 1)
    verify_tree(base / 'reconstructed.iso', disc)
print('PASS: DVD to GOD, independent hash chain, XDVDFS round trip and failure cleanup')
