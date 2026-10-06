# Jane-Sixty icon

Original artwork for the application and plug-in icon: a dark panel tile with the front panel's cream, red and blue section bands along the top and the DCO's sawtooth in the panel's orange accent. No Roland artwork or lettering is used.

| File | Use |
|---|---|
| `jane60.svg` | Master artwork, 1024 x 1024 viewBox. Edit this one. |
| `jane60-small.svg` | Variant for 16 to 48 px: fewer, bolder teeth so the waveform still reads. |
| `jane60-1024.png` | `ICON_BIG` in `CMakeLists.txt`. JUCE renders the macOS `.icns` and the Windows exe `.ico` from it at build time. |
| `jane60-128.png` | `ICON_SMALL`, rendered from the small variant. JUCE uses it for the 16 and 32 px entries. |
| `jane60-256.png` | README and manual title image. |
| `../../installer/windows/jane60.ico` | Inno Setup wizard and setup exe icon (16 to 48 px from the small variant, 64 to 256 px from the master). |

Colours come from `src/ui/Controls.h`: panel `#1c1c1e`, bands `#e6e1d3` / `#b0372f` / `#2f5fa8`, accent `#ff9f40`.

## Re-rendering

Any SVG renderer that honours transparency works. The committed PNGs were made with headless Chromium and ImageMagick:

```sh
chrome --headless=new --no-sandbox --disable-gpu --hide-scrollbars --default-background-color=00000000 \
       --window-size=1024,1024 --screenshot=jane60-1024.png file://$PWD/jane60.svg
chrome --headless=new --no-sandbox --disable-gpu --hide-scrollbars --default-background-color=00000000 \
       --window-size=1024,1024 --screenshot=small-1024.png file://$PWD/jane60-small.svg
convert small-1024.png -resize 128x128 jane60-128.png
convert jane60-1024.png -resize 256x256 jane60-256.png
for s in 16 24 32 48; do convert small-1024.png -resize ${s}x${s} i$s.png; done
for s in 64 128 256; do convert jane60-1024.png -resize ${s}x${s} i$s.png; done
convert i16.png i24.png i32.png i48.png i64.png i128.png i256.png ../../installer/windows/jane60.ico
```
