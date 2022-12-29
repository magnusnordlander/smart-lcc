import struct
import serial

# ser = serial.Serial('/dev/cu.usbserial-0001', 115200, timeout=4)
ser = serial.Serial('/dev/cu.usbmodem12101', 115200, timeout=4)

input("foo")

syncdata = b"\x07\x07\x12\x20" + 32 * b"\x55"

syncpkt = struct.pack(b"<BBHI", 0x00, 0x08, len(syncdata), 0x00) + syncdata

buf = (
        b"\xc0"
        + (syncpkt.replace(b"\xdb", b"\xdb\xdd").replace(b"\xc0", b"\xdb\xdc"))
        + b"\xc0"
)

while True:
    print("Attempting to sync")
    ser.write(buf)

    c = ser.read(1)
    if c != b'':
        break

while c != b'':
    print(c)
    c = ser.read(1)

print("reading")

CHIP_DETECT_MAGIC_REG_ADDR = 0x40001000

data = struct.pack("<I", CHIP_DETECT_MAGIC_REG_ADDR)

pkt = struct.pack(b"<BBHI", 0x00, 0x0a, len(data), 0x00) + data

buf = (
        b"\xc0"
        + (pkt.replace(b"\xdb", b"\xdb\xdd").replace(b"\xc0", b"\xdb\xdc"))
        + b"\xc0"
)

print(buf)
ser.write(buf)

while True:
    c = ser.read(1)
    print(c)