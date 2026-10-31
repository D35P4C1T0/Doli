"""Draw Doli's D-and-note mark for the homebrew launcher (Pillow)."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
S = 4
im = Image.new('RGB', (256 * S, 256 * S), '#181b26')
d = ImageDraw.Draw(im)
# The same open D and note as ICON_BRAND_D / ICON_BRAND_NOTE, on a 100-unit grid.
def box(coords):
    return tuple(round(v * 2.56 * S) for v in coords)
d.arc(box((23, 17, 85, 83)), -90, 90, fill='#ee946e', width=round(12 * 2.56 * S))
d.line(box((54, 17, 23, 17, 23, 83, 54, 83)), fill='#ee946e', width=round(12 * 2.56 * S), joint='curve')
d.ellipse(box((34, 59, 52, 77)), fill='#ffddc0')
d.line(box((51, 68, 51, 39, 65, 45)), fill='#ffddc0', width=round(7 * 2.56 * S), joint='curve')
im = im.resize((256, 256), Image.Resampling.LANCZOS)
assert im.size == (256, 256) and im.mode == 'RGB'  # NRO launcher icon requirements
im.save(ROOT / 'icon.png')
im.save(ROOT / 'icon.jpg', quality=95, subsampling=0)
