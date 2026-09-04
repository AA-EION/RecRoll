#!/usr/bin/env python3
"""
Generates the RecRoll application/plugin icon.

This mark belongs to the app, not to the studio: EION STUDIOS keeps its own
symbol (see Resources/EION_Simbolo_Tinta.svg) and is never substituted for it.
RecRoll's mark is a rolling loop - an open cyan ring caught mid-rotation, a red
record head sitting on the ring where the buffer is written, and the captured
waveform held in the middle.

    python3 tools/make_icon.py

writes Resources/RecRoll_Icon_1024.png, _256.png, _128.png and RecRoll_Icon.ico.
Everything is drawn at 4x and resampled, so edges stay clean at 32 px.
"""

import math
import os

from PIL import Image, ImageDraw

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Resources")

SS = 4096                      # supersampled canvas
BG_TOP = (23, 28, 39)          # panel top
BG_BOTTOM = (8, 10, 15)        # panel bottom, matches the plugin's #121318 family
CYAN = (0, 229, 255)           # the app accent, #00e5ff
CYAN_DEEP = (0, 150, 199)
REC_RED = (255, 23, 68)        # the "NOW" head, #ff1744
BONE = (242, 240, 235)         # EION Polymer Bone, the one inherited value


def polar(cx, cy, deg, r):
    """Screen-space polar: 0 deg is 3 o'clock, angles grow clockwise (y is down)."""
    a = math.radians(deg)
    return (cx + r * math.cos(a), cy + r * math.sin(a))


def vertical_gradient(size, top, bottom):
    grad = Image.new("RGB", (1, size))
    for y in range(size):
        t = y / (size - 1)
        grad.putpixel((0, y), tuple(round(top[i] + (bottom[i] - top[i]) * t) for i in range(3)))
    return grad.resize((size, size), Image.BILINEAR)


def draw_icon(size=SS):
    s = size
    cx = cy = s / 2.0

    icon = Image.new("RGBA", (s, s), (0, 0, 0, 0))

    # --- panel -------------------------------------------------------------
    mask = Image.new("L", (s, s), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, s - 1, s - 1], radius=0.225 * s, fill=255)
    icon.paste(vertical_gradient(s, BG_TOP, BG_BOTTOM).convert("RGBA"), (0, 0), mask)

    # hairline so the tile reads as an object on a light desktop too
    edge = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    ImageDraw.Draw(edge).rounded_rectangle(
        [0.006 * s, 0.006 * s, s - 1 - 0.006 * s, s - 1 - 0.006 * s],
        radius=0.219 * s, outline=(255, 255, 255, 26), width=int(0.007 * s))
    icon.alpha_composite(edge)

    d = ImageDraw.Draw(icon)

    # --- the roll ----------------------------------------------------------
    r_out = 0.372 * s
    t = 0.077 * s
    r_mid = r_out - t / 2.0
    bbox = [cx - r_out, cy - r_out, cx + r_out, cy + r_out]

    start, end = 22.0, 300.0   # gap on the right, centred near 1 o'clock
    d.arc(bbox, start=start, end=end, fill=CYAN + (255,), width=int(t))

    # a cooler inner lip gives the ring some dimension at large sizes
    d.arc([cx - r_out + t * 0.78, cy - r_out + t * 0.78,
           cx + r_out - t * 0.78, cy + r_out - t * 0.78],
          start=start, end=end, fill=CYAN_DEEP + (110,), width=int(t * 0.20))

    # arrowhead: the ring is turning clockwise, into the gap
    tip = polar(cx, cy, end + 12.5, r_mid)
    d.polygon([tip,
               polar(cx, cy, end - 1.5, r_mid + t * 1.16),
               polar(cx, cy, end - 1.5, r_mid - t * 1.16)],
              fill=CYAN + (255,))

    # record head: caps the other end of the arc, sitting on the ring path
    hx, hy = polar(cx, cy, start, r_mid)
    rh = t * 0.86
    d.ellipse([hx - rh * 1.55, hy - rh * 1.55, hx + rh * 1.55, hy + rh * 1.55],
              fill=REC_RED + (60,))
    d.ellipse([hx - rh, hy - rh, hx + rh, hy + rh], fill=REC_RED + (255,))

    # --- captured waveform -------------------------------------------------
    # Five bars, symmetric about the centre line: what the buffer is holding.
    heights = [0.30, 0.62, 1.00, 0.62, 0.30]
    bar_w = 0.062 * s
    gap = 0.044 * s
    span = len(heights) * bar_w + (len(heights) - 1) * gap
    x = cx - span / 2.0
    for i, h in enumerate(heights):
        half = h * 0.185 * s
        colour = BONE if i == 2 else CYAN
        d.rounded_rectangle([x, cy - half, x + bar_w, cy + half],
                            radius=bar_w / 2.0, fill=colour + (255,))
        x += bar_w + gap

    return icon


def main():
    os.makedirs(OUT, exist_ok=True)
    master = draw_icon()
    for px in (1024, 512, 256, 128):
        img = master.resize((px, px), Image.LANCZOS)
        img.save(os.path.join(OUT, f"RecRoll_Icon_{px}.png"))
        print("wrote", f"RecRoll_Icon_{px}.png")

    master.resize((256, 256), Image.LANCZOS).save(
        os.path.join(OUT, "RecRoll_Icon.ico"),
        sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    print("wrote RecRoll_Icon.ico")


if __name__ == "__main__":
    main()
