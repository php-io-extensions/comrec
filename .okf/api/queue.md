---
type: API
title: Queue
description: Constants, PixelQueue: every method, the commands, the arithmetic of each shape, how a box is found and how two frames are compared, and what is refused.
resource: stubs/
tags: [queue, display-list, damage, transform, c]
status: draft
generated: { by: claude-opus/5.5, at: 2026-11-19T06:00:00Z }
sources:
  - id: stubs
    resource: stubs/
    title: comrec.stub.php, PixelQueue.stub.php, PixelQueueException.stub.php
  - id: class
    resource: src/PixelQueue.c
    title: The class
  - id: boxes
    resource: src/boxes.c
    title: Rects, a piece's box, the merge
---

# Overview

`PixelQueue` records a frame of drawing commands under a transform and a clip, keeps the last two finished frames, and answers where a frame writes and where two frames differ.[^stubs] It draws nothing.

Coordinates are numbers in pixels of a surface with whole numbers on pixel edges. A colour is an integer the queue never looks at.

Two kinds of value belong to the caller and pass through untouched: the **clip value** (given to `begin()`, `clip()` and `enter()`, put last in every command recorded under it) and the two **rule values** (given to the constructor, put in `path` commands). The queue keeps the clip's rect beside its value for its own arithmetic.

# Frame

| Method | What it does |
|---|---|
| `__construct(nonZero = 0, evenOdd = 1)` | The two rule values. |
| `begin(width, height, clip)` | Opens a frame: no commands, the identity transform, nothing pushed, clip rect `0, 0, width, height`. Sides 1..2^24. `Error` when one is open. |
| `recording()` | Whether a frame is open. |
| `finish()` | Returns the commands. The frame becomes *last*; the old last becomes *before*; a layer still entered is dropped. `Error` with no frame. |
| `abandon()` | Drops the open frame, its layers and its pushes. Last and before stay. |

# State

| Method | What it does |
|---|---|
| `push()` / `pop()` | Keep / bring back the transform, the clip value and the clip rect. `pop()` is an `Error` when nothing is pushed in the frame or layer being recorded. |
| `depth()` | Pushes waiting in the frame or layer being recorded. |
| `concat(a, b, c, d, e, f)` | Current × given: `a' = a·ga + c·gb`, `b' = b·ga + d·gb`, `c' = a·gc + c·gd`, `d' = b·gc + d·gd`, `e' = a·ge + c·gf + e`, `f' = b·ge + d·gf + f`. A component that is not finite throws `PIXEL_QUEUE_BAD_MATRIX` with `$value` the first such, and the transform is unchanged. |
| `matrix()` | `[a, b, c, d, e, f]`. |
| `clip(value, x, y, width, height)` / `clipValue()` | Set the clip; read the value. A width or height of 0 or less is an empty clip: shapes record nothing under it. |
| `enter(value, x, y, width, height)` / `leave()` | A layer: commands go to a list of their own under this clip with nothing pushed; `leave()` returns them and brings back the commands, the clip and the pushes of before. The transform is not kept or brought back. |

# Shapes

Every shape first checks that a frame is open, then that each argument is finite, in the order of the arguments. A size, radius or stroke of 0 or less then records nothing. A point goes through the transform as `a·x + c·y + e`, `b·x + d·y + f` and each coordinate must be finite and at most `PIXEL_QUEUE_LIMIT` in magnitude, x first. Nothing is recorded under an empty clip, but the checks still run.[^class]

| Method | Records |
|---|---|
| `clear(rgba)` | `['clear', rgba]`, whatever the clip. |
| `fillRect(x, y, w, h, rgba)` | `['path', [[tl, tr, br, bl]], nonZero, rgba, clip]`. |
| `fillTriangle(...)` | `['path', [[p0, p1, p2]], nonZero, rgba, clip]`. |
| `fillPath(ops, evenOdd, rgba)` | `['path', contours, rule, rgba, clip]`: every subpath of the ops `['M', x, y]`, `['L', x, y]`, `['Q', cx, cy, x, y]`, `['C', c1x, c1y, c2x, c2y, x, y]`, `['Z']`, through the transform, curves flattened (see *Flattening a curve*); `rule` is `evenOdd` or `nonZero` as given to the constructor. A segment after `Z` starts a new subpath where the closed one began. Reach: the points, one pixel out. Nothing for no points. Every number is checked in order, finite then within the limit once transformed, named `x` or `y`; ops of the wrong form, or not starting with `M`, are a `ValueError`. |
| `strokePath(ops, rgba, stroke)` | A `['polyline', points, stroke, closed, rgba, clip]` for every subpath of two points or more, flattened as `fillPath()`'s, `closed` where a `Z` closes it, the stroke scaled as `line()`'s. The ops are checked before the stroke. |
| `strokeRect(x, y, w, h, rgba, stroke)` | `['polyline', [tl, tr, br, bl], stroke', true, rgba, clip]`. |
| `line(x0, y0, x1, y1, rgba, stroke)` | `['polyline', [p0, p1], stroke', false, rgba, clip]`. |
| `fillEllipse(cx, cy, rx, ry, rgba)` | Axes kept (`b == 0` and `c == 0`): `['ellipse', cx', cy', rx·|a|, ry·|d|, rgba, clip]`. Otherwise a `path` of the flattened ellipse. |
| `strokeEllipse(..., stroke)` | Axes kept: `['ring', cx', cy', rx', ry', stroke', rgba, clip]`. Otherwise a closed `polyline` of the flattened ellipse. |
| `roundRect(x, y, w, h, radii, fill, border, borderWidth)` | Below. |
| `shadow(x, y, w, h, radius, blur, rgba, dx, dy)` | `['shadow', x0, y0, x1, y1, radius·a, blur·a, rgba, clip]`, corners `(x + dx, y + dy)` and `(x + w + dx, y + h + dy)` transformed; radius held to `0..min(w, h) / 2`, blur to 0 or more. Needs an even transform. |
| `glyphs(runs, x, y, ink, rgba)` | `['glyphs', runs, [a, b, c, d, e′, f′], rgba, [x0, y0, x1, y1], clip]`: `runs` carried as given; `e′, f′` is `(x, y)` through the transform; `[x0 .. y1]` the box of `ink`'s four corners, each moved by `(x, y)` and transformed, one pixel out. `x`, `y` checked and transformed first; nothing recorded for a null `ink` or an empty clip. Refusals name the call `text`; a malformed `ink` is a `ValueError`. |

**Stroke.** `stroke' = stroke · sqrt(|a·d − b·c|)`; past the limit it throws with `$what` `stroke`. A stroke' of 0 or less records nothing.

**Flattening.** A circle of radius r pixels gets `sides = ceil(π · sqrt(r / (2 · 0.1)) · 1.01)`, held to 12..1024. For an ellipse, `r = max(rx, ry) · stretch` with `stretch = sqrt(max(a² + b², c² + d²))`, and point i is `(cx + rx·cos(θ), cy + ry·sin(θ))` transformed, `θ = 2π·i / sides`. A rounded corner of radius r gets `steps = ceil(sides(r · stretch) / 4)` and `steps + 1` points from its start angle (π, 1.5π, 0, 0.5π for top-left, top-right, bottom-right, bottom-left) in steps of `0.5π / steps`; a corner of radius 0 is one point.

**Rounded box.** `radii` must be a list of four integers or floats, else `PIXEL_QUEUE_BAD_RADII`; each must be finite (`$what` `radius 0`..`radius 3`) and is held to `0..min(w, h) / 2`. `bw` is 0 with no border, else `borderWidth` held to 0 or more. Inner radii are `max(0, r − bw)`. The box is hollow when `w > 2·bw` and `h > 2·bw`.

* Even transform (`b == 0`, `c == 0`, `a == d`, `a > 0`), with `(x0, y0)` and `(x1, y1)` its corners transformed and `s = a`:
  * `bw <= 0`: `['roundrect', x0, y0, x1, y1, radii·s, fill, clip]`.
  * else, when hollow: `['roundrect', x0 + bw·s, y0 + bw·s, x1 − bw·s, y1 − bw·s, inner·s, fill, clip]`; then `['roundring', x0, y0, x1, y1, radii·s, bw·s, border, clip]`.
* Any other transform: the outline flattened. `bw <= 0`: one `path` of it, non-zero, in `fill`. Else, when hollow, a `path` of the inner outline in `fill`; then a `path` of the outer and (when hollow) inner outlines, even-odd, in `border`.

# Commands made elsewhere

`append(command, how, grow = 0.0, pieces = null, restless = false, reads = null)` records `command` as it is. `how` says where it writes:

| `how` | Where it writes |
|---|---|
| `PIXEL_QUEUE_WHOLE` | The whole surface. |
| `PIXEL_QUEUE_CONTOURS` | Around the points of `command[1]`, a list of lists of `[x, y]`, `grow` out, inside the current clip. |
| `PIXEL_QUEUE_POINTS` | Around the points of `command[1]`, a list of `[x, y]`, `grow` out, inside the current clip. |
| `PIXEL_QUEUE_SPANS` | The pixels of `command[1]`, a string of 7-byte spans (y, x, length as uint16 LE, coverage), inside the current clip. |
| `PIXEL_QUEUE_PIECES` | `pieces`: 8 numbers (`x0, y0, x1, y1` then a clip's `x, y, width, height`), or 16 for two; the box is the two boxes joined. |

For contours, points and spans the entry is read when a box is asked for, not when the command is appended. An entry that is not what `how` says gives the command no box.

`restless` marks a command whose pixels can change with nothing in it changing. `reads` is `[x, y, width, height, reach]`: the command reads the pixels under that rect and `reach` around it.

# Lowering

`lower(command, form)` gives a recorded command in a simpler form, for a drawer whose library cannot draw it as it is. It reads nothing but the command and the two rule values the queue was made with, and records nothing. The steps are `src/lower.c`, plain C; topside's `Surface\Rendering\CommandRecorders\Lowering` is the same steps in PHP and gives the same numbers bit for bit.

- `'polygon'`: `['path', contours, rule, paint, clip]`. A path comes back as it is. An ellipse is its circle's sides (the shapes' rule, from the larger radius) from angle 0; a rounded box its outline clockwise from the top-left corner, a corner of radius `r` in `(sides(r) + 3) / 4` steps; a rounded ring that outline and, when the inset leaves a hole, the inset outline with radii `max(0, r − inset)`, under the even-odd value. A stroke (a polyline, a ring's ellipse) is the left side going forward then the left side of the way back: one contour open, two closed, butt ends, non-zero. A side's left normal is `(−dy, dx) / length · width / 2`. A corner turning away from the side is mitered at the corner plus `(n0 + n1) · 1 / (2 cos²)`, `cos = |n0 + n1| / width`, while `1 / cos ≤ 4`, else beveled; a corner turning towards it, or a way straight back, goes through the corner's point.
- `'trapezoids'`: `['trapezoids', [[top, bottom, left at top, left at bottom, right at top, right at bottom], ...], paint, clip]`. Edges (no flat ones) sweep down between the sorted heights of their ends. A band's edges are ordered by their x halfway down, then by index (an insertion sort from the last band's order, which it is near). Where two neighbours' order differs at the top or bottom, the band is cut at the earliest such crossing more than 1e-9 inside it, top part first, except under non-zero inside a *sealed group*: the edges between two places where the edges left of them are the same at the band's top and at its bottom (ordered there by x, then x halfway, then index) reorder only among themselves, and if the winding left of the group plus all their downs is above 0, or plus all their ups below 0, no order of them empties the area between them, so their crossings change nothing and are not cut. In a band with no cut, the rule's spans (winding ≠ 0, or odd) are found left to right, each by its left and right edge. A trapezoid stays open while a span with the same two edges follows it; one that does not continue ends at the band's top, its bottom corners worked out from its own edges (`x0 + (y - y0) · ((x1 - x0) / (y1 - y0))`), and is given then, in the order the open ones were found; the rest end at the last height.
- `'triangles'`: `['triangles', [[x0, y0, x1, y1, x2, y2], ...], paint, clip]`: each trapezoid as (left top, right top, right bottom) and (left top, right bottom, left bottom), without one whose top or bottom has no width.

Each form lowers from the one before: a `'trapezoids'` command as `lower()` gives it lowers to `'triangles'` without being cut again (its numbers at 0 to 5, ints or floats, read as floats), and to no other form. Velvet hands each form to the next.

A third argument, `packing`, gives a `'trapezoids'` or `'triangles'` command's items as packed strings in place of lists: a list of 1 to 8 `[encoding, fields]` pairs, one string each. `encoding` is `'f32'` (little-endian float32) or `'fixed16'` (little-endian int32, each number times 65536 rounded half away from zero, held to int32); `fields` are the places in an item (0 to 5) each struct field holds, in order, repeats allowed. Each string is every item's fields in turn. A drawer states the layout; the queue knows no library. A `'polygon'` is always lists, and a packed command lowers no further.

null for a command with no outline (`'clear'`, `'shadow'`, `'glyphs'`) or one it cannot read; a `ValueError` for any other form, or a packing that is not as described.

# Boxes

A command's **reach** is `x0, y0, x1, y1` in pixels: a path's points one pixel out; a polyline's `2·stroke + 1` out; an ellipse's `c ± r ± 1`; a ring's `c ± r ± stroke / 2 ± 1`; a rounded box's corners one out; a shadow's corners `blur + 1` out; text's ink box, transformed, one out. Its **box** on a surface `sw` by `sh` is `floor(x0)` held to 0 at least, `floor(y0)` likewise, `ceil(x1)` held to `sw` at most, `ceil(y1)` to `sh`, then cut to its clip rect; no box when that is no pixel.[^boxes]

A **grain** is `unitWidth, unitHeight, limitWidth, limitHeight`. Snapping a rect grows its left and top down to a multiple of the unit, its right and bottom up to one, and holds the right and bottom to the limits.

**Merging** a list, in order: snap the rect; join it with every kept rect it overlaps (a rect that only touches does not overlap), snapping after each join, pass after pass until it overlaps none; put it last. Kept rects stay in the order they were put.

| Method | What it answers |
|---|---|
| `drawn(sw, sh, uw, uh, lw, lh, also = [])` | The boxes of the last frame's commands, then `also`, merged. |
| `changed(sw, sh, uw, uh, lw, lh, also = [])` | Below. |

Both return `x, y, width, height` four numbers a rect. `also` is rects in the same form.

**changed()**, for place i up to the longer frame: when both frames have a command there, the new one is not restless, and the two are equal by PHP's `==`, nothing; otherwise the old command's box, then the new one's. Those are merged. The result and `also` are merged again. Then each command of the last frame that reads gives an **outer**: its rect grown by `ceil(reach)` each side and cut to the surface. Pass after pass over the outers still out, in order: one that overlaps a rect is merged in (the rects merged again from nothing, then the outer) and leaves; until a pass joins none.

# Refusals

`PixelQueueException` (extends `Exception`) has `$what` and `$value`; its code:

| Code | When | `$what`, `$value` |
|---|---|---|
| `PIXEL_QUEUE_NOT_RECORDING` | A shape with no frame open | the method |
| `PIXEL_QUEUE_NOT_FINITE` | An argument is not finite | its name |
| `PIXEL_QUEUE_PAST_LIMIT` | A coordinate, a radius or a stroke is past the limit once transformed | its name, what it became |
| `PIXEL_QUEUE_BAD_RADII` | Radii are not a list of four numbers | |
| `PIXEL_QUEUE_UNEVEN` | `shadow()` under a transform that is not even | the method |
| `PIXEL_QUEUE_BAD_MATRIX` | `concat()` would make a component that is not finite | `''`, the component |

Argument names are the stub's: `x`, `y`, `width`, `height`, `stroke`, `x0`..`y2`, `cx`, `cy`, `rx`, `ry`, `radius`, `blur`, `dx`, `dy`, `borderWidth`. A transformed point reports `x` or `y` (a line's and a triangle's report `x0`..`y2`, an ellipse's `cx` or `cy`).

# Constants

`PIXEL_QUEUE_LIMIT` 16777216.0; `PIXEL_QUEUE_FLATNESS` 0.1; the five `how` values; the six codes.

[^stubs]: The stubs are the declaration: every method and constant, with its types.
[^class]: `src/PixelQueue.c`. The kind strings are interned in `MINIT`.
[^boxes]: `src/boxes.c`, plain C with no Zend.


## Flattening a curve

A path's curves are flattened after the transform: its control points go through it first, then the curve is cut into `n` straight pieces at even steps of `t`, so a curve drawn larger gets more pieces.

* Pieces: a quadratic's `n = ceil(sqrt(d / (4 × 0.25)))` with `d = |p0 − 2p1 + p2|`; a cubic's `n = ceil(sqrt(3d / (4 × 0.25)))` with `d` the larger of `|p0 − 2p1 + p2|` and `|p1 − 2p2 + p3|`. `|v|` is `sqrt(x·x + y·y)`. `n` is at least 1 and at most 1024. The 0.25 is the most a piece strays from the curve, in pixels.
* Points: for `i` = 1 to `n − 1`, `t = i / n`, `mt = 1 − t`. A quadratic's `x = (a·x0 + b·x1) + d·x2` with `a = mt·mt`, `b = 2·mt·t`, `d = t·t`; a cubic's `x = ((a·x0 + b·x1) + e·x2) + d·x3` with `a = mt·mt·mt`, `b = 3·mt·mt·t`, `e = 3·mt·t·t`, `d = t·t·t`; each product left to right, `y` the same. The last point is the curve's end as given.


## Paints

`fillRect`, `strokeRect`, `line`, `fillTriangle`, `fillEllipse`, `strokeEllipse`, `roundRect` (fill and border), `fillPath` and `strokePath` take a paint where they took an `rgba`: an int colour, `0xRRGGBBAA`, or a gradient, a list of four `[kind, geometry, stops, spread]`. An int goes into the command as it is. A gradient goes in as given with the transform it is drawn through appended, `[a, b, c, d, e, f]`: a list of five. The queue reads nothing inside a gradient: what kind, geometry, stops and spread mean is the caller's and the drawer's. A paint that is neither an int nor a list of four is a `ValueError`, checked before anything else. `clear`, `shadow` and `glyphs` keep an int colour.
