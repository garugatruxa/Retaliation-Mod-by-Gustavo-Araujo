"""Builds retstar.txd (RenderWare texture dictionary, GTA SA PC / D3D9) with
one 64x64 A8R8G8B8 texture named "retstar": the red star from the artwork
(1280x1217 five-point star). The texture is white so the game can tint it
(red by default, yellow during the max-heat conversion).
"""
import struct
import sys

SIZE = 64
SS = 4  # supersampling for smooth edges

# star outline from the artwork (pixel coordinates in a 1280 x 1217 image)
STAR = [(640, 0), (791, 465), (1280, 465), (885, 752), (1036, 1217),
        (640, 930), (244, 1217), (395, 752), (0, 465), (489, 465)]
W0, H0 = 1280.0, 1217.0


def inside(px, py, poly):
    c = False
    n = len(poly)
    for i in range(n):
        x1, y1 = poly[i]
        x2, y2 = poly[(i + 1) % n]
        if (y1 > py) != (y2 > py):
            xi = x1 + (py - y1) * (x2 - x1) / (y2 - y1)
            if px < xi:
                c = not c
    return c


def render():
    scale = (SIZE - 2) / W0              # 1 px transparent margin left/right
    h = H0 * scale
    ox, oy = 1.0, (SIZE - h) / 2.0
    poly = [(ox + x * scale, oy + y * scale) for x, y in STAR]
    pixels = bytearray()
    for y in range(SIZE):
        for x in range(SIZE):
            hits = 0
            for sy in range(SS):
                for sx in range(SS):
                    if inside(x + (sx + 0.5) / SS, y + (sy + 0.5) / SS, poly):
                        hits += 1
            a = round(255 * hits / (SS * SS))
            pixels += bytes((255, 255, 255, a))   # B G R A
    return bytes(pixels)


VER = 0x1803FFFF


def chunk(kind, payload):
    return struct.pack('<III', kind, len(payload), VER) + payload


def texture_native(name, pixels):
    s = struct.pack('<I', 9)                         # platform: D3D9 (SA PC)
    s += struct.pack('<I', 0x1102)                   # filter linear, wrap U/V
    s += name.encode().ljust(32, b'\0') + b''.ljust(32, b'\0')
    s += struct.pack('<I', 0x0500)                   # raster format 8888
    s += struct.pack('<I', 21)                       # D3DFMT_A8R8G8B8
    s += struct.pack('<HHBBBB', SIZE, SIZE, 32, 1, 4, 1)   # w h depth levels type flags(alpha)
    s += struct.pack('<I', len(pixels)) + pixels
    return chunk(0x15, chunk(0x01, s) + chunk(0x03, b''))


def main(out):
    tex = texture_native('retstar', render())
    txd = chunk(0x16, chunk(0x01, struct.pack('<HH', 1, 2)) + tex + chunk(0x03, b''))
    with open(out, 'wb') as f:
        f.write(txd)
    print(f"wrote {out} ({len(txd)} bytes)")


if __name__ == '__main__':
    main(sys.argv[1])
