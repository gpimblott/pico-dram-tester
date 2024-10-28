import sys
import os
import struct

# Export an individual icon
def do_icon(ico_info, ico, fname):
    f = open(fname + '.h', 'w')
    (width, height, pal_entries, res, planes, bpp, datasize, doffset) = ico_info
    if (res != 0):
        print("Not a valid ICO file.")
        quit()

    if (pal_entries != 16) or (planes != 1) or (bpp != 4):
        print("Unsupported color format.")
        quit()
    imgdata = ico[doffset + 40:][:datasize - 40] # Ignore the BITMAPINFOHEADER
    paldata = imgdata[:16 * 4]
    image = imgdata[16 * 4: (16 * 4) + int(width * height / 2)]
    mask = imgdata[(16*4) + int(width * height / 2) :]

    f.write('// Image: %d x %d\n' % (width, height))

    # Now parse the palette
    pal = []
    for i in range(0, 16 * 4, 4):
        b = paldata[i] >> 3
        g = paldata[i+1] >> 2
        r = paldata[i+2] >> 3
        # Ignoring alpha channel
        # BBBBBGGGGGGRRRRR
        pal.append('0x%.4X' % ((b << 11) | (g << 5) | r))
    f.write('uint16_t %s_palette[] = { ' % (fname) + ', '.join(pal) + ' };\n\n')

    # Image data
    f.write('const uint8_t %s_image[] = {\n' % fname)
    for i in range(0, int(width * height / 2), 16):
        d1 = image[i:i+16]
        f.write(', '.join(["0x%0.2x" % x for x in d1]))
        if i == int(width * height / 2) - 16:
            f.write('\n')
        else:
            f.write(',\n')
    f.write('};\n\n')

    # Image mask
    f.write('const uint8_t %s_mask[] = {\n' % fname)
    w = 4
    for i in range(0, len(mask), w):
        d1 = mask[i:i+w]
        f.write(', '.join(["0x%0.2x" % x for x in d1]))
        if i == len(mask) - w:
            f.write('\n')
        else:
            f.write(',\n')
    f.write('};\n\n')

    f.write('const ico_def_t %s = { %d, %d, %s_palette, %s_image, %s_mask };\n' % (fname, width, height, fname, fname, fname))
    f.close()

if len(sys.argv) != 2:
    print("Usage: %s <filename>" % (sys.argv[0]))
    quit()

fname = os.path.splitext(os.path.basename(sys.argv[1]))[0]
f = open(sys.argv[1], 'rb')
ico = f.read()
f.close()
(res, icotype, num_images) = struct.unpack('<HHH', ico[0:6])
if (res != 0) or (icotype != 1):
    print("Not a valid ICO file.")
    quit()
#if num_images != 1:
#    print("Only one icon per ICO file supported.")
#    quit()

icons = []
for i in range(num_images):
    icons.append(struct.unpack('<BBBBHHLL', ico[6 + i * 16:][:16]))

if len(icons) == 1:
    do_icon(icons[0], ico, fname)
else:
    for i in range(len(icons)):
        do_icon(icons[i], ico, "%s%d" % (fname, i))


