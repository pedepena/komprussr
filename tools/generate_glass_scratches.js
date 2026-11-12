// Deterministic generator for the KU-01 meter glass wear layer.
// Uses only Node.js built-ins. Run from the repository root with:
//   node tools/generate_glass_scratches.js

const fs = require ("node:fs");
const path = require ("node:path");
const zlib = require ("node:zlib");

const root = path.resolve (__dirname, "..");
const sourcePath = path.join (root, "assets/ui/originales/vu_glass_overlay.png");
const output1x = path.join (root, "assets/ui/capas/vu_glass_overlay.png");
const output2x = path.join (root, "assets/ui/capas/vu_glass_overlay@2x.png");
const seed = 0x4b553031;

let randomState = seed >>> 0;
function random ()
{
    randomState ^= randomState << 13;
    randomState ^= randomState >>> 17;
    randomState ^= randomState << 5;
    return (randomState >>> 0) / 0x100000000;
}

const crcTable = new Uint32Array (256);
for (let n = 0; n < 256; ++n)
{
    let c = n;
    for (let k = 0; k < 8; ++k) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    crcTable[n] = c >>> 0;
}
function crc32 (data)
{
    let c = 0xffffffff;
    for (const byte of data) c = crcTable[(c ^ byte) & 0xff] ^ (c >>> 8);
    return (c ^ 0xffffffff) >>> 0;
}
function chunk (name, data)
{
    const type = Buffer.from (name, "ascii"), body = Buffer.concat ([type, data]);
    const result = Buffer.alloc (12 + data.length);
    result.writeUInt32BE (data.length, 0); body.copy (result, 4);
    result.writeUInt32BE (crc32 (body), result.length - 4);
    return result;
}
function decodePng (file)
{
    const png = fs.readFileSync (file);
    if (! png.subarray (0, 8).equals (Buffer.from ([137, 80, 78, 71, 13, 10, 26, 10]))) throw new Error (`Not a PNG: ${file}`);
    let width = 0, height = 0, bitDepth = 0, colourType = 0;
    const idat = [];
    for (let p = 8; p < png.length;)
    {
        const length = png.readUInt32BE (p), type = png.toString ("ascii", p + 4, p + 8);
        const data = png.subarray (p + 8, p + 8 + length);
        if (type === "IHDR") { width = data.readUInt32BE (0); height = data.readUInt32BE (4); bitDepth = data[8]; colourType = data[9]; }
        else if (type === "IDAT") idat.push (data);
        p += 12 + length;
    }
    if (bitDepth !== 8 || colourType !== 6) throw new Error ("Expected an 8-bit RGBA PNG");
    const bpp = 4, rowBytes = width * bpp, packed = zlib.inflateSync (Buffer.concat (idat));
    const pixels = Buffer.alloc (height * rowBytes);
    const paeth = (a, b, c) =>
    {
        const p = a + b - c, pa = Math.abs (p - a), pb = Math.abs (p - b), pc = Math.abs (p - c);
        return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
    };
    for (let y = 0; y < height; ++y)
    {
        const filter = packed[y * (rowBytes + 1)], source = y * (rowBytes + 1) + 1, target = y * rowBytes;
        for (let x = 0; x < rowBytes; ++x)
        {
            const left = x >= bpp ? pixels[target + x - bpp] : 0;
            const up = y > 0 ? pixels[target + x - rowBytes] : 0;
            const upperLeft = y > 0 && x >= bpp ? pixels[target + x - rowBytes - bpp] : 0;
            const raw = packed[source + x];
            const predictor = filter === 0 ? 0 : filter === 1 ? left : filter === 2 ? up
                : filter === 3 ? Math.floor ((left + up) / 2) : filter === 4 ? paeth (left, up, upperLeft) : null;
            if (predictor === null) throw new Error (`Unsupported PNG filter ${filter}`);
            pixels[target + x] = (raw + predictor) & 0xff;
        }
    }
    return { width, height, pixels };
}
function encodePng ({ width, height, pixels })
{
    const ihdr = Buffer.alloc (13);
    ihdr.writeUInt32BE (width, 0); ihdr.writeUInt32BE (height, 4); ihdr[8] = 8; ihdr[9] = 6;
    const raw = Buffer.alloc (height * (width * 4 + 1));
    for (let y = 0; y < height; ++y) { const row = y * (width * 4 + 1); raw[row] = 0; pixels.copy (raw, row + 1, y * width * 4, (y + 1) * width * 4); }
    return Buffer.concat ([Buffer.from ([137, 80, 78, 71, 13, 10, 26, 10]), chunk ("IHDR", ihdr),
        chunk ("IDAT", zlib.deflateSync (raw, { level: 9 })), chunk ("IEND", Buffer.alloc (0))]);
}
function upscale2x (source)
{
    const width = source.width * 2, height = source.height * 2, pixels = Buffer.alloc (width * height * 4);
    for (let y = 0; y < height; ++y) for (let x = 0; x < width; ++x)
    {
        const fx = x / 2, fy = y / 2, x0 = Math.floor (fx), y0 = Math.floor (fy);
        const x1 = Math.min (source.width - 1, x0 + 1), y1 = Math.min (source.height - 1, y0 + 1), tx = fx - x0, ty = fy - y0;
            const alphaAt = (xx, yy) => source.pixels[(yy * source.width + xx) * 4 + 3] / 255;
            const alphas = [alphaAt (x0, y0), alphaAt (x1, y0), alphaAt (x0, y1), alphaAt (x1, y1)];
            const alphaTop = alphas[0] + (alphas[1] - alphas[0]) * tx;
            const alphaBottom = alphas[2] + (alphas[3] - alphas[2]) * tx;
            const alpha = alphaTop + (alphaBottom - alphaTop) * ty;
            const output = (y * width + x) * 4;
            for (let c = 0; c < 3; ++c)
            {
                const values = [[x0, y0], [x1, y0], [x0, y1], [x1, y1]].map (([xx, yy], i) =>
                    source.pixels[(yy * source.width + xx) * 4 + c] * alphas[i]);
                const top = values[0] + (values[1] - values[0]) * tx;
                const bottom = values[2] + (values[3] - values[2]) * tx;
                pixels[output + c] = alpha > 0 ? Math.round ((top + (bottom - top) * ty) / alpha) : 0;
            }
            pixels[output + 3] = Math.round (alpha * 255);
    }
    return { width, height, pixels };
}
function compositeMark (image, x, y, colour, alpha)
{
    if (x < 0 || y < 0 || x >= image.width || y >= image.height) return;
    const i = (Math.floor (y) * image.width + Math.floor (x)) * 4, sourceAlpha = image.pixels[i + 3] / 255, a = alpha / 255;
    const outAlpha = a + sourceAlpha * (1 - a);
    for (let c = 0; c < 3; ++c)
        image.pixels[i + c] = outAlpha === 0 ? 0 : Math.round ((colour[c] * a + image.pixels[i + c] * sourceAlpha * (1 - a)) / outAlpha);
    image.pixels[i + 3] = Math.round (outAlpha * 255);
}
function drawScratch (image, points, colour, alpha, width)
{
    const scale = image.width / 463, pts = points.map (p => [p[0] * scale, p[1] * scale]), radius = width * scale * 0.5;
    for (let s = 1; s < pts.length; ++s)
    {
        const [ax, ay] = pts[s - 1], [bx, by] = pts[s], minX = Math.max (0, Math.floor (Math.min (ax, bx) - radius - 1));
        const maxX = Math.min (image.width - 1, Math.ceil (Math.max (ax, bx) + radius + 1));
        const minY = Math.max (0, Math.floor (Math.min (ay, by) - radius - 1));
        const maxY = Math.min (image.height - 1, Math.ceil (Math.max (ay, by) + radius + 1));
        const dx = bx - ax, dy = by - ay, length2 = dx * dx + dy * dy;
        for (let y = minY; y <= maxY; ++y) for (let x = minX; x <= maxX; ++x)
        {
            let coverage = 0;
            for (const [ox, oy] of [[0.25, 0.25], [0.75, 0.25], [0.25, 0.75], [0.75, 0.75]])
            {
                const px = x + ox, py = y + oy, t = Math.max (0, Math.min (1, ((px - ax) * dx + (py - ay) * dy) / length2));
                if (Math.hypot (px - ax - t * dx, py - ay - t * dy) <= radius) coverage += 0.25;
            }
            if (coverage > 0) compositeMark (image, x, y, colour, alpha * coverage);
        }
    }
}
function drawSmudge (image, cx, cy, rx, ry, angle, colour, alpha)
{
    const scale = image.width / 463, cosine = Math.cos (angle), sine = Math.sin (angle);
    const extent = Math.ceil (Math.max (rx, ry) * scale * 2);
    for (let y = Math.max (0, Math.floor (cy * scale) - extent); y <= Math.min (image.height - 1, Math.ceil (cy * scale) + extent); ++y)
        for (let x = Math.max (0, Math.floor (cx * scale) - extent); x <= Math.min (image.width - 1, Math.ceil (cx * scale) + extent); ++x)
        {
            const dx = (x + 0.5) / scale - cx, dy = (y + 0.5) / scale - cy;
            const u = dx * cosine + dy * sine, v = -dx * sine + dy * cosine;
            const radius2 = (u / rx) ** 2 + (v / ry) ** 2;
            if (radius2 < 1) compositeMark (image, x, y, colour, alpha * Math.exp (-3 * radius2) * (1 - radius2) ** 2);
        }
}
function makeGlass (source, use2x)
{
    const image = use2x ? upscale2x (source) : { width: source.width, height: source.height, pixels: Buffer.from (source.pixels) };
    const paths = [
        { p: [[39, 38], [52, 35], [69, 38], [84, 44]], c: [255, 245, 224], a: 64, w: 0.54 },
        { p: [[429, 30], [420, 33], [411, 37], [401, 42]], c: [168, 147, 116], a: 39, w: 0.46 },
        { p: [[35, 169], [43, 164], [53, 165], [61, 170]], c: [255, 246, 227], a: 60, w: 0.58 },
        { p: [[433, 166], [424, 162], [415, 164], [405, 170]], c: [160, 140, 111], a: 36, w: 0.48 },
        { p: [[78, 156], [91, 153], [106, 156], [121, 162]], c: [255, 247, 231], a: 68, w: 0.56 },
        { p: [[384, 155], [371, 153], [355, 157], [340, 164]], c: [155, 136, 108], a: 38, w: 0.50 },
        { p: [[24, 132], [33, 128], [42, 130], [49, 134]], c: [255, 247, 230], a: 48, w: 0.46 },
        { p: [[438, 141], [430, 138], [422, 141], [417, 145]], c: [164, 145, 116], a: 33, w: 0.44 }
    ];
    for (const path of paths)
    {
        const jitter = (random () - 0.5) * 1.2;
        drawScratch (image, path.p.map ((p, i) => [p[0] + (i % 2 ? jitter : -jitter), p[1] + (random () - 0.5) * 0.5]),
            path.c, path.a * (0.88 + random () * 0.24), path.w);
    }
    for (let i = 0; i < 24; ++i)
    {
        const side = i % 4, t = random ();
        const x = side < 2 ? 28 + random () * 28 : 407 + random () * 28;
        const y = side % 2 === 0 ? 18 + t * 35 : 158 + t * 38;
        const length = 1.2 + random () * 4, angle = (random () - 0.5) * 0.7;
        drawScratch (image, [[x, y], [x + Math.cos (angle) * length, y + Math.sin (angle) * length]],
            [255, 246, 227], 14 + random () * 14, 0.34 + random () * 0.18);
    }
    for (let i = 0; i < 17; ++i)
    {
        const x = random () < 0.5 ? 27 + random () * 27 : 409 + random () * 27;
        const y = random () < 0.5 ? 15 + random () * 32 : 162 + random () * 33, r = 0.35 + random () * 0.5;
        for (let yy = -1; yy <= 1; ++yy) for (let xx = -1; xx <= 1; ++xx)
            if (Math.hypot (xx, yy) <= r * 2) compositeMark (image, Math.round (x * 2) + xx, Math.round (y * 2) + yy,
                [250, 242, 224], 10 + random () * 10);
    }
    drawSmudge (image, 27, 148, 10, 4.2, -0.22, [160, 142, 116], 19);
    drawSmudge (image, 436, 151, 8, 3.2, 0.18, [156, 139, 112], 16);
    drawSmudge (image, 42, 181, 6, 2.5, -0.10, [247, 238, 219], 14);
    return image;
}
const original = decodePng (sourcePath);
randomState = seed >>> 0;
const glass1x = makeGlass (original, false);
randomState = seed >>> 0;
const glass2x = makeGlass (original, true);
fs.writeFileSync (output1x, encodePng (glass1x));
fs.writeFileSync (output2x, encodePng (glass2x));
console.log (`Generated ${output1x} (${glass1x.width}x${glass1x.height})`);
console.log (`Generated ${output2x} (${glass2x.width}x${glass2x.height})`);
