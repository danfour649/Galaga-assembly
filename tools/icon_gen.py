"""Scale /tmp/icon48.bmp (from tools/icon_gen.c) into web/icons/ (needs Pillow)."""
import os
from PIL import Image

out = os.path.join(os.path.dirname(__file__), '..', 'web', 'icons')
src = Image.open('/tmp/icon48.bmp').convert('RGB')
bg = (10, 10, 24)
for size in (192, 512):
    src.resize((size, size), Image.NEAREST).save(os.path.join(out, f'icon-{size}.png'), optimize=True)
    m = Image.new('RGB', (size, size), bg)          # maskable: art in the 75% safe zone
    inner = int(size * 0.75) // 48 * 48 or int(size * 0.75)
    m.paste(src.resize((inner, inner), Image.NEAREST), ((size - inner) // 2, (size - inner) // 2))
    m.save(os.path.join(out, f'icon-maskable-{size}.png'), optimize=True)
src.resize((180, 180), Image.NEAREST).save(os.path.join(out, 'apple-touch-icon.png'), optimize=True)
src.resize((32, 32), Image.LANCZOS).save(os.path.join(out, 'favicon-32.png'), optimize=True)
