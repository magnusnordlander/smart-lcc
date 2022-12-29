import base64
import zlib
import codecs
import os

stub = eval(zlib.decompress(base64.b64decode(b"""
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\
""")))



outlen = 8
swap_bytes = False
# set EOL
eol = "\n"
# columns per line
cols = 12
# show data content in comments
showDataContent = False
hname = 'stubs'

badchars = ("\\", "+", "-", "*", " ")

# uppercase name for header
hname_upper = hname.upper()
hname_upper += "_H"

# *** START: read/write *** #

# declare read/write streams
bytes_written = 0

wordbytes = int(outlen / 8)

# open file stream for writing
ofs = codecs.open("stub/stub.h", "w", "utf-8")

text = "#ifndef {0}{1}#define {0}{1}{1}#include <cstdint>{1}".format(hname_upper, eol)

data_type = "char"
text += "{0}static const unsigned {1} {2}[] = {{{0}".format(eol, data_type, 'stub_text')

ofs.write(text)

eof = False # to check if we are at the end of file
comment = ""

byte_idx = 0
while byte_idx < len(stub['text']):
    if eof:
        break

    if bytes_written % (cols * wordbytes) == 0:
        ofs.write("\t")
        comment = ""

    byte_order = range(0, wordbytes)

    word = ""
    for i in byte_order:
        word += "%02x" % stub['text'][byte_idx + i]
    byte_idx += wordbytes

    ofs.write("0x{}".format(word))
    bytes_written += wordbytes

    if bytes_written % (cols * wordbytes) == 0:
        ofs.write(",")
        if showDataContent:
            ofs.write(" /* {} */".format(comment))
        ofs.write(eol)
    else:
        ofs.write(", ")

ofs.write(eol)

text = "}};{0}".format(eol)



data_type = "char"
text += "{0}static const unsigned {1} {2}[] = {{{0}".format(eol, data_type, 'stub_data')

ofs.write(text)

eof = False # to check if we are at the end of file
comment = ""

byte_idx = 0
while byte_idx < len(stub['data']):
    if eof:
        break

    if bytes_written % (cols * wordbytes) == 0:
        ofs.write("\t")
        comment = ""

    byte_order = range(0, wordbytes)

    word = ""
    for i in byte_order:
        word += "%02x" % stub['data'][byte_idx + i]
    byte_idx += wordbytes

    ofs.write("0x{}".format(word))
    bytes_written += wordbytes

    if bytes_written % (cols * wordbytes) == 0:
        ofs.write(",")
        if showDataContent:
            ofs.write(" /* {} */".format(comment))
        ofs.write(eol)
    else:
        ofs.write(", ")

ofs.write(eol)

text = "}};{0}".format(eol)


text +="{0}uint32_t stub_text_start = {1};{0}".format(eol, stub['text_start'])
text +="{0}uint32_t stub_data_start = {1};{0}".format(eol, stub['data_start'])
text +="{0}uint32_t stub_entry = {1};{0}".format(eol, stub['entry'])



text +="{0}#endif /* {1} */{0}".format(eol, hname_upper)

ofs.write(text)

# close file write stream
ofs.close()
