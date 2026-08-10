#!/usr/bin/env python3
#
#  gen_sample.py
#  asciiplay
#
#  Created by Piotr Panasewicz on 10/08/2026.
#  Copyright © 2026 Codice. All rights reserved.
#
#  Licensed under the MIT licence. See LICENSE in the project root
#  for the full text.
#
"""Generate sample.png, the demo image in the README.

A small raytracer rather than a photograph, so the repository carries no
third-party imagery and anyone can reproduce the picture exactly.

The scene is picked to exercise the things the renderer is judged on: hard
silhouette edges against the sky, a checkerboard that recedes into
high-frequency detail (which is what dithering and the braille grid are for),
smooth tonal gradients across each sphere, and saturated colour.

    pip install numpy pillow
    python3 tools/gen_sample.py -o sample.png

"""

import argparse

import numpy as np
from PIL import Image

# --------------------------------------------------------------- the scene --

# (centre, radius, colour, specular, reflectivity)
SPHERES = [
    ((-1.75, 0.90, 1.40), 0.90, (0.85, 0.20, 0.22), 0.55, 0.28),
    ((0.35, 0.55, -0.55), 0.55, (0.96, 0.74, 0.16), 0.70, 0.20),
    ((1.95, 1.20, 2.10), 1.20, (0.18, 0.42, 0.80), 0.45, 0.35),
]
LIGHT = np.array([5.0, 7.0, -4.0])

# A dark sky over a bright floor.  The tonal range matters more here than
# prettiness: a mostly-pale scene survives histogram equalisation as noise,
# whereas strong silhouettes still read at one dot per pixel.
HORIZON = np.array([0.38, 0.45, 0.58])
ZENITH = np.array([0.03, 0.05, 0.12])
CHECK_A = np.array([0.94, 0.93, 0.90])
CHECK_B = np.array([0.05, 0.06, 0.08])


def unit(v):
    return v / np.linalg.norm(v, axis=-1, keepdims=True)


def sky(d):
    """Vertical gradient, brightened towards the light for a hint of glare.

    The camera sits near the ground, so it only ever sees the bottom quarter of
    the hemisphere -- the ramp has to be steep or the whole sky comes out one
    flat tone.
    """
    t = np.clip(d[:, 1:2] * 3.5, 0.0, 1.0)
    base = HORIZON * (1.0 - t) + ZENITH * t
    glow = np.clip((d @ unit(LIGHT)).reshape(-1, 1), 0.0, 1.0) ** 24
    return np.clip(base + glow * 0.45, 0.0, 1.0)


def hit_spheres(orig, d):
    """Nearest sphere hit per ray: distance and which sphere (-1 for none)."""
    best = np.full(len(d), np.inf)
    which = np.full(len(d), -1, dtype=np.int32)
    for i, (centre, radius, _, _, _) in enumerate(SPHERES):
        oc = orig - np.asarray(centre)
        b = np.einsum("ij,ij->i", oc, d)
        c = np.einsum("ij,ij->i", oc, oc) - radius * radius
        disc = b * b - c
        ok = disc > 0.0
        if not ok.any():
            continue
        root = np.sqrt(np.where(ok, disc, 0.0))
        t = -b - root
        far = -b + root
        t = np.where(t < 1e-3, far, t)          # inside the sphere
        ok &= t > 1e-3
        closer = ok & (t < best)
        best = np.where(closer, t, best)
        which = np.where(closer, i, which)
    return best, which


def hit_plane(orig, d):
    """The floor is y = 0, and only rays heading downwards can reach it."""
    down = d[:, 1] < -1e-6
    t = np.full(len(d), np.inf)
    t[down] = -orig[down, 1] / d[down, 1]
    t[t < 1e-3] = np.inf
    return t


def shadowed(p, n):
    """One shadow ray towards the light, spheres only -- the floor is flat."""
    to_light = unit(LIGHT - p)
    dist = np.linalg.norm(LIGHT - p, axis=-1)
    t, _ = hit_spheres(p + n * 1e-3, to_light)
    return t < dist


def trace(orig, d, depth=2):
    colour = np.zeros_like(d)

    ts, which = hit_spheres(orig, d)
    tp = hit_plane(orig, d)

    on_sphere = ts < tp
    on_plane = tp < ts
    missed = ~(on_sphere | on_plane)
    if missed.any():
        colour[missed] = sky(d[missed])

    for mask, is_sphere in ((on_sphere, True), (on_plane, False)):
        if not mask.any():
            continue
        o, dd = orig[mask], d[mask]
        t = (ts if is_sphere else tp)[mask]
        p = o + dd * t[:, None]

        if is_sphere:
            idx = which[mask]
            centres = np.array([s[0] for s in SPHERES])[idx]
            n = unit(p - centres)
            albedo = np.array([s[2] for s in SPHERES])[idx]
            spec = np.array([s[3] for s in SPHERES])[idx][:, None]
            refl = np.array([s[4] for s in SPHERES])[idx][:, None]
        else:
            n = np.zeros_like(p)
            n[:, 1] = 1.0
            check = (np.floor(p[:, 0]) + np.floor(p[:, 2])) % 2 == 0
            albedo = np.where(check[:, None], CHECK_A, CHECK_B)
            spec = np.full((len(p), 1), 0.25)
            # Only the light squares reflect much; it keeps the checker legible
            # instead of dissolving into mirror.
            refl = np.where(check[:, None], 0.22, 0.06)

        to_light = unit(LIGHT - p)
        lam = np.clip(np.einsum("ij,ij->i", n, to_light), 0.0, 1.0)[:, None]
        lam = np.where(shadowed(p, n)[:, None], lam * 0.12, lam)

        half = unit(to_light - dd)
        phong = np.clip(np.einsum("ij,ij->i", n, half), 0.0, 1.0)[:, None] ** 48

        shade = albedo * (0.13 + 0.87 * lam) + spec * phong * lam

        if depth > 0:
            r = dd - 2.0 * np.einsum("ij,ij->i", dd, n)[:, None] * n
            shade = shade * (1.0 - refl) + refl * trace(p + n * 1e-3, unit(r),
                                                        depth - 1)

        if not is_sphere:
            # Fade the floor into the sky so the horizon does not alias into a
            # hard line of noise.
            fog = np.clip(t[:, None] / 34.0, 0.0, 1.0) ** 0.7
            shade = shade * (1.0 - fog) + sky(dd) * fog

        colour[mask] = shade

    return colour


# ------------------------------------------------------------------- camera --

def render(width, height, samples):
    eye = np.array([0.0, 1.45, -5.0])
    target = np.array([0.0, 0.95, 0.6])
    fwd = unit(target - eye)
    right = unit(np.cross(fwd, np.array([0.0, 1.0, 0.0])))
    up = np.cross(right, fwd)
    scale = np.tan(np.radians(46.0) * 0.5)
    aspect = width / height

    w, h = width * samples, height * samples
    xs = (np.arange(w) + 0.5) / w * 2.0 - 1.0
    ys = 1.0 - (np.arange(h) + 0.5) / h * 2.0
    gx, gy = np.meshgrid(xs, ys)

    d = unit(fwd
             + right * (gx * scale * aspect).reshape(-1, 1)
             + up * (gy * scale).reshape(-1, 1))
    orig = np.repeat(eye[None, :], len(d), axis=0)

    img = trace(orig, d).reshape(h, w, 3)
    if samples > 1:                       # box-filter the supersampled grid
        img = img.reshape(height, samples, width, samples, 3).mean((1, 3))

    img = np.clip(img, 0.0, 1.0) ** (1.0 / 2.2)          # to sRGB
    return (img * 255.0 + 0.5).astype(np.uint8)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("-o", "--out", default="sample.png")
    ap.add_argument("--width", type=int, default=1280)
    ap.add_argument("--height", type=int, default=720)
    ap.add_argument("--samples", type=int, default=3,
                    help="supersampling factor per axis (default 3)")
    a = ap.parse_args()

    Image.fromarray(render(a.width, a.height, a.samples)).save(a.out,
                                                              optimize=True)
    print("wrote %s (%dx%d)" % (a.out, a.width, a.height))


if __name__ == "__main__":
    main()
