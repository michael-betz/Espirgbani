#!/usr/bin/python3
"""
    convert extracted runDMD image file to a series of .gif files
    usage:
        dmd2gif.py ./dmd.img ./outFolder
"""
import sys
from os.path import join
from struct import unpack
from PIL import Image

HEADER_OFFS = 0x0000C800
HEADER_SIZE = 0x00000200
ANI_DAT_START = 0x004E2000

if len(sys.argv) != 3:
    print(__doc__)
    sys.exit()

fName = sys.argv[1]
outFolder = sys.argv[2]


def parse_header(f, headerIndex):
    """
      f: file object of RunDMD_BXXX.img
      headerIndex: integer, which animation to fetch
      returns: dict with header info
    """
    f.seek(HEADER_OFFS + HEADER_SIZE * headerIndex)
    rawDat = unpack(">HxBIBBB9x32s", f.read(52))
    header = {
        "animation_id": rawDat[0],
        "n_bitmaps": rawDat[1],
        "byte_offset": rawDat[2] * HEADER_SIZE,
        "n_frames": rawDat[3],
        "width": rawDat[4],
        "height": rawDat[5],
        "name": rawDat[6].strip(b'\x00').decode("ascii")
    }
    f.seek(header["byte_offset"])
    buff = f.read(header["n_frames"] * 2)
    header["frame_ids"] = list(buff[0::2])
    header["frame_durs"] = list(buff[1::2])
    return header


def get_frame_dat(f, header, frame_offset):
    """
        f: open .img file
        header: dict from parse_header()
        frame_offset: which frame (0 = first)
        returns byte buffer of size 128 x 64, 1 pixel per byte
    """
    byte_offset = header["byte_offset"] + HEADER_SIZE
    byte_offset += 128 * 32 * frame_offset // 2
    f.seek(byte_offset)
    buff_4 = f.read(128 * 32 // 2)      # 2 pixel per byte
    # unpack 2 pixels / byte to 1 pixel / byte
    buff_8 = bytearray(128 * 32)        # 1 pixel per byte
    buff_8[0::2] = [(i & 0xF0) for i in buff_4]
    buff_8[1::2] = [(i & 0x0F) << 4 for i in buff_4]
    return bytes(buff_8)


with open(fName, "rb") as f:
    if f.read(3) != b'DGD':
        raise RuntimeError('Invalid DGD header')
    n_ani = unpack(">H", f.read(2))[0]
    f.seek(0x1ef)
    buildStr = f.read(8).strip(b'\x00').decode("ascii")
    print("Found", n_ani, "animations in", buildStr)
    for i_ani in range(n_ani):
        try:
            h = parse_header(f, i_ani)
            full_file_name = join(outFolder, h["name"] + ".gif")
            imgs = []
            for f_id, f_dur in zip(h["frame_ids"], h["frame_durs"]):
                img_dat = get_frame_dat(f, h, f_id - 1)
                img = Image.frombytes("P", (128, 32), img_dat)
                img.info["duration"] = f_dur
                imgs.append(img)
            imgs[0].save(
                full_file_name,
                "GIF",
                save_all=True,
                append_images=imgs[1:],
                optimize=False
                # There seems to be a bug in pillow for transparency :(
                # transparency=0xA0,
                # disposal=2
            )
            print(full_file_name)
        except Exception as e:
            print("Ohh snap ", e, full_file_name)
