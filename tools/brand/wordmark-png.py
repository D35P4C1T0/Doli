"""The name for the README: Doli's magical letters (Gfx::wordmark: the gradient, the glow, a shimmer, sparkles) as PNGs with
a transparent background, one for each of GitHub's themes (the pale end of the app's gradient is lost on white).

    python tools/brand/wordmark-png.py          (needs Pillow, numpy; writes docs/wordmark-dark.png, wordmark-light.png)
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(ROOT / 'tools' / 'preview'))
from make_film import ACCENT, ACCENT2, GLOW, TXT, Word, mix   # noqa: E402

PX = 300   # for sharp letters at the 620 px wide they are shown


def wordmark(word, stops, glow_col, glow_a, shine, sparkle_col):
    """Letters over their glow, as straight-alpha RGBA."""
    a, b, c = (np.array(s, dtype=np.float32) for s in stops)
    u = word.u
    col = np.where((u < .45)[:, None], a + (b - a) * (u / .45)[:, None], b + (c - b) * ((u - .45) / .55)[:, None])[None, :, :]
    d = (u - shine) / .16
    band = (np.exp(-4.5 * d * d) * (1 - d * d) * (np.abs(d) < 1))[None, :, None].astype(np.float32)
    col = col + (255 - col) * band * .6
    la = word.a[:, :, None]
    ga = np.clip(word.glow * glow_a, 0, 1)[:, :, None]
    # the letters over the glow, on nothing
    out_a = la + ga * (1 - la)
    rgb = (col * la + np.array(glow_col, dtype=np.float32) * ga * (1 - la)) / np.maximum(out_a, 1e-4)
    im = np.concatenate([rgb, out_a * 255], 2)
    # two sparkles, after the shimmer: the app's four-pointed star
    h, w = word.a.shape
    for sx, sy, size in ((word.pad + word.w - PX * .03, word.pad + word.h * .06, PX * .17), (word.pad + word.w * .64, word.pad - PX * .03, PX * .09)):
        n = int(size * 2) | 1
        c1 = np.linspace(-1, 1, n, dtype=np.float32)
        xx, yy = np.meshgrid(c1, c1)
        m = np.clip(np.clip(1 - (np.abs(xx) ** .55 + np.abs(yy) ** .55), 0, 1) ** 1.2 * 2.2, 0, 1)[:, :, None]
        x0, y0 = int(sx - n / 2), int(sy - n / 2)
        r = im[y0:y0 + n, x0:x0 + n]
        na = m + r[:, :, 3:] / 255 * (1 - m)
        r[:, :, :3] = (np.array(sparkle_col, dtype=np.float32) * m + r[:, :, :3] * (r[:, :, 3:] / 255) * (1 - m)) / np.maximum(na, 1e-4)
        r[:, :, 3:] = na * 255
    pic = Image.fromarray(np.clip(im, 0, 255).astype(np.uint8), 'RGBA')
    return pic.crop(pic.getchannel('A').point(lambda v: 255 if v > 6 else 0).getbbox())



word = Word(PX, pad=int(PX * .34))
dark = wordmark(word, (mix(ACCENT, TXT, .12), mix(ACCENT2, TXT, .2), mix(ACCENT2, TXT, .78)), GLOW, .5, .72, (255, 244, 236))
light = wordmark(word, ((226, 62, 47), (208, 96, 44), (150, 74, 40)), ACCENT, .34, -1, (240, 150, 96))
dark.save(ROOT / 'docs' / 'wordmark-dark.png', optimize=True)
light.save(ROOT / 'docs' / 'wordmark-light.png', optimize=True)
print(dark.size, light.size)
