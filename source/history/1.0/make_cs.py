"""Assembles Retaliation.cs (CLEO 4 script) by hand.

Script logic:
    0ADF: add_dynamic_GXT_entry <key> text <text>   (one per HUD string)
    :LOOP
    0001: wait 0
    1FA5: retaliation_tick                          (custom opcode from Retaliation.asi)
    0002: jump @LOOP
"""
import struct
import sys

TEXTS = [
    ("RETPTS", "RETALIATION ~1~/50"),
    ("RETLVL", "Nivel de pressao: ~1~"),
    ("RETDEC", "Perde 10 pts em ~1~s"),
    ("RETCLR", "Sem pressao"),
    ("RETHNT", "Cacadores ativos: ~1~"),
    ("RETNXT", "Proxima emboscada: ~1~s"),
    ("RETOFF", "Nivel 0: ninguem te persegue"),
    ("RETPAM", "Pausado: em missao"),
    ("RETPAI", "Pausado: em interior"),
    ("RETLOD", "Emboscada se preparando..."),
    ("RETWAS", "Aguardando o jogador..."),
    ("RETEVK", "+~1~ NPC assassinado"),
    ("RETEVC", "+~1~ Veiculo roubado"),
    ("RETEVD", "-~1~ Pressao diminuiu"),
    ("RETEVA", "EMBOSCADA A CAMINHO!"),
    ("RETEVX", "-~1~ Morte ou prisao"),
    ("RETEVG", "Debug: ~1~ pts"),
]


def op(n):
    return struct.pack("<H", n)


def vstr(s):
    b = s.encode("ascii")
    assert len(b) < 128
    return b"\x0E" + bytes([len(b)]) + b


code = bytearray()
code += op(0x0000)                                   # nop (so no label sits at offset 0)
for key, text in TEXTS:
    assert len(key) <= 7
    code += op(0x0ADF) + vstr(key) + vstr(text)
loop = len(code)
code += op(0x0001) + b"\x04\x00"                     # wait 0
code += op(0x1FA5)                                   # custom tick
code += op(0x0002) + b"\x01" + struct.pack("<i", -loop)  # jump (negative = local label)

with open(sys.argv[1], "wb") as f:
    f.write(code)
print(f"wrote {len(code)} bytes, loop at {loop}")
