#!/usr/bin/env python3
"""Generate scaled KOMPRUSSR knobs and their fixed light/shadow layers."""

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / "assets" / "ui"
LAYERS = UI / "capas"
SOURCE = UI / "knobs_nuevos"

KNOBS = [
    {
        "id": "knob_compression",
        "source": "knob_a_vintage",
        "stem": "knob_compression_vintage",
        "x": 121,
        "y": 347,
        "width": 250,
        "height": 250,
        "z": 6,
    },
    {
        "id": "knob_makeup",
        "source": "knob_c_marfil",
        "stem": "knob_makeup_ivory",
        "x": 1005,
        "y": 347,
        "width": 250,
        "height": 250,
        "z": 7,
    },
    {
        "id": "knob_tube",
        "source": "knob_b_desgastado",
        "stem": "knob_tube_worn",
        "x": 806,
        "y": 517,
        "width": 120,
        "height": 120,
        "z": 8,
    },
]


def resample_knob(source_path: Path, target_size: int) -> Image.Image:
    image = Image.open(source_path).convert("RGBA")
    return image.resize((target_size, target_size), Image.Resampling.LANCZOS)


def make_shadow(size: int) -> Image.Image:
    scale = size / 250.0
    radius = size * (752.0 / 1026.0) / 2.0
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(mask)
    cx = size / 2 + 4.5 * scale
    cy = size / 2 + 5.5 * scale
    rx = radius * 0.99
    ry = radius * 0.96
    draw.ellipse((cx - rx, cy - ry, cx + rx, cy + ry), fill=48)
    mask = mask.filter(ImageFilter.GaussianBlur(5.0 * scale))
    image.putalpha(mask)
    return image


def make_light(size: int) -> Image.Image:
    radius = size * (752.0 / 1026.0) / 2.0
    pixels = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    values = pixels.load()
    cx = cy = (size - 1) / 2.0
    for y in range(size):
        for x in range(size):
            dx = x - cx
            dy = y - cy
            distance = math.hypot(dx, dy)
            if distance >= radius:
                continue
            # Broad, low-contrast warm lift from the fixed upper-left quadrant.
            hx = (dx + radius * 0.24) / (radius * 0.54)
            hy = (dy + radius * 0.28) / (radius * 0.54)
            highlight = 13.0 * math.exp(-0.5 * (hx * hx + hy * hy))
            edge = max(0.0, min(1.0, (distance / radius - 0.72) / 0.28))
            edge = edge * edge * 8.0
            values[x, y] = (255, 239, 211, round(highlight)) if highlight >= edge else (0, 0, 0, round(edge))
    return pixels


def make_clean_global(source_path: Path, output_path: Path, centers: list[tuple[int, int, int]]) -> None:
    image = Image.open(source_path).convert("RGBA")
    alpha = image.getchannel("A")
    mask = Image.new("L", image.size, 255)
    draw = ImageDraw.Draw(mask)
    for cx, cy, radius in centers:
        draw.ellipse((cx - radius, cy - radius, cx + radius, cy + radius), fill=0)
    # Feather the cutout edge so the preserved panel shadow/light has no seam.
    cut = Image.eval(mask, lambda value: 255 - value).filter(ImageFilter.GaussianBlur(2.0))
    alpha = Image.composite(alpha, Image.new("L", image.size, 0), cut)
    image.putalpha(alpha)
    image.save(output_path, optimize=True)


def make_previews() -> None:
    preview_dir = UI / "previews" / "knob_angles"
    preview_dir.mkdir(parents=True, exist_ok=True)
    base = Image.open(LAYERS / "panel_base_off.png").convert("RGBA")
    for name, pos in (("scales_labels.png", (0, 0)), ("tubes_on.png", (460, 472)), ("lamp_on.png", (1076, 53)), ("knob_shadows_sin_knobs.png", (0, 0))):
        base.alpha_composite(Image.open(LAYERS / name).convert("RGBA"), pos)

    for knob in KNOBS:
        shadow = Image.open(LAYERS / f"{knob['id']}_shadow.png").convert("RGBA")
        base.alpha_composite(shadow, (knob["x"], knob["y"]))

    def knob_image(knob: dict, angle: float) -> Image.Image:
        image = Image.open(LAYERS / f"{knob['stem']}.png").convert("RGBA")
        return image.rotate(-angle, resample=Image.Resampling.BICUBIC, center=(image.width / 2, image.height / 2))

    requested = {
        "knob_compression": [-135, 0, 135],
        "knob_makeup": [-135, 0, 135, -45],
        "knob_tube": [-135, 0, 135],
    }
    default_angles = {"knob_compression": -135, "knob_makeup": -45, "knob_tube": 0}
    for target in KNOBS:
        for angle in requested[target["id"]]:
            frame = base.copy()
            for knob in KNOBS:
                current_angle = angle if knob["id"] == target["id"] else default_angles[knob["id"]]
                frame.alpha_composite(knob_image(knob, current_angle), (knob["x"], knob["y"]))
                light = Image.open(LAYERS / f"{knob['id']}_light.png").convert("RGBA")
                frame.alpha_composite(light, (knob["x"], knob["y"]))
            fixed_highlights = Image.open(LAYERS / "knob_highlights_overlay_sin_knobs.png").convert("RGBA")
            frame.alpha_composite(fixed_highlights, (0, 0))
            tumbler = Image.open(LAYERS / "tumbler_on.png").convert("RGBA")
            frame.alpha_composite(tumbler, (1192, 107))
            needle = Image.open(LAYERS / "vu_needle.png").convert("RGBA")
            frame.alpha_composite(needle.rotate(-28.1, resample=Image.Resampling.BICUBIC, center=(240, 240)), (459, 132))
            glass = Image.open(LAYERS / "vu_glass_overlay.png").convert("RGBA")
            frame.alpha_composite(glass, (458, 165))
            suffix = "0db_-45" if target["id"] == "knob_makeup" and angle == -45 else f"{angle:+d}"
            (preview_dir / f"{target['id']}_{suffix}.png").parent.mkdir(parents=True, exist_ok=True)
            frame.save(preview_dir / f"{target['id']}_{suffix}.png", optimize=True)


def main() -> None:
    for knob in KNOBS:
        base_size = knob["width"]
        for scale, suffix in ((1, ""), (2, "@2x")):
            size = base_size * scale
            # The provided 512px source is intended for 1x; use 1026px for @2x.
            source_size = "512" if scale == 1 else "1026"
            src = SOURCE / f"{knob['source']}_{source_size}.png"
            out = LAYERS / f"{knob['stem']}{suffix}.png"
            resample_knob(src, size).save(out, optimize=True)

            shadow = LAYERS / f"{knob['id']}_shadow{suffix}.png"
            light = LAYERS / f"{knob['id']}_light{suffix}.png"
            make_shadow(size).save(shadow, optimize=True)
            make_light(size).save(light, optimize=True)

    # The existing full-canvas layers also contain the old knob effects. Keep
    # the original PNGs intact and point the manifest at clean variants.
    make_clean_global(
        LAYERS / "knob_shadows.png",
        LAYERS / "knob_shadows_sin_knobs.png",
        [(246, 472, 101), (1130, 472, 101), (866, 577, 54)],
    )
    make_clean_global(
        LAYERS / "knob_highlights_overlay.png",
        LAYERS / "knob_highlights_overlay_sin_knobs.png",
        [(246, 472, 101), (1130, 472, 101), (866, 577, 54)],
    )
    make_previews()


if __name__ == "__main__":
    main()
