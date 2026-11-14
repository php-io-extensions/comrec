---
type: API
title: Queue
description: Constants, PixelQueue: every method, the commands, the arithmetic of each shape, how a box is found and how two frames are compared, and what is refused.
resource: stubs/
tags: [queue, display-list, damage, transform, c]
status: draft
generated: { by: claude-fable/5.2, at: 2026-11-13T12:00:00Z }
sources:
  - id: stubs
    resource: stubs/
    title: pixel_queue.stub.php, PixelQueue.stub.php, PixelQueueException.stub.php
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
| `strokeRect(x, y, w, h, rgba, stroke)` | `['polyline', [tl, tr, br, bl], stroke', true, rgba, clip]`. |
| `line(x0, y0, x1, y1, rgba, stroke)` | `['polyline', [p0, p1], stroke', false, rgba, clip]`. |
| `fillEllipse(cx, cy, rx, ry, rgba)` | Axes kept (`b == 0` and `c == 0`): `['ellipse', cx', cy', rx·|a|, ry·|d|, rgba, clip]`. Otherwise a `path` of the flattened ellipse. |
| `strokeEllipse(..., stroke)` | Axes kept: `['ring', cx', cy', rx', ry', stroke', rgba, clip]`. Otherwise a closed `polyline` of the flattened ellipse. |
| `roundRect(x, y, w, h, radii, fill, border, borderWidth)` | Below. |
| `shadow(x, y, w, h, radius, blur, rgba, dx, dy)` | `['shadow', x0, y0, x1, y1, radius·a, blur·a, rgba, clip]`, corners `(x + dx, y + dy)` and `(x + w + dx, y + h + dy)` transformed; radius held to `0..min(w, h) / 2`, blur to 0 or more. Needs an even transform. |

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

# Boxes

A command's **reach** is `x0, y0, x1, y1` in pixels: a path's points one pixel out; a polyline's `2·stroke + 1` out; an ellipse's `c ± r ± 1`; a ring's `c ± r ± stroke / 2 ± 1`; a rounded box's corners one out; a shadow's corners `blur + 1` out. Its **box** on a surface `sw` by `sh` is `floor(x0)` held to 0 at least, `floor(y0)` likewise, `ceil(x1)` held to `sw` at most, `ceil(y1)` to `sh`, then cut to its clip rect; no box when that is no pixel.[^boxes]

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
