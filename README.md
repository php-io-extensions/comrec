<div align="center" id="header">

<h1>ext-pixel-queue</h1>

<h3>A frame of drawing commands in C, and what changed since the last one, for PHP 8.4+</h3>

[![CI](https://img.shields.io/github/actions/workflow/status/php-io-extensions/pixel-queue/ci.yml?style=flat-square&label=CI)](https://github.com/php-io-extensions/pixel-queue/actions/workflows/ci.yml)
[![Packagist](https://img.shields.io/packagist/v/php-io-extensions/pixel-queue?style=flat-square)](https://packagist.org/packages/php-io-extensions/pixel-queue)
[![PHP](https://img.shields.io/packagist/dependency-v/php-io-extensions/pixel-queue/php?style=flat-square&label=PHP)](https://packagist.org/packages/php-io-extensions/pixel-queue)
[![License](https://img.shields.io/github/license/php-io-extensions/pixel-queue?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-linux_%7C_macos-blue?style=flat-square&logo=linux&logoColor=white)](#requirements)
[![OKF](https://img.shields.io/badge/OKF-0.2-6f42c1?style=flat-square&label=knowledge%20bundle)](.okf/index.md)
[![PIE](https://img.shields.io/badge/PIE-installable-4f5b93?style=flat-square&logo=php&logoColor=white)](https://github.com/php/pie)

</div>

A program that draws a frame many times a second rarely needs to repaint all of it. To repaint only what changed it has to remember the last frame, compare the new one with it, and work out which pixels the differences cover. Before that, every shape it draws has to go through the current transform and be checked. In PHP both are loops of small steps for every shape, every frame.

<b>ext-pixel-queue</b> does both in C. `PixelQueue` records shapes as commands under a transform and a clip, keeps the last frame, and answers the rects that differ between the two. It draws nothing: the commands it returns are plain PHP lists for whatever paints them. It has no dependency.

## Basic usage

```php
$queue = new PixelQueue();

$queue->begin(320, 200, 'whole');           // a frame on a 320 x 200 surface
$queue->clear(0x101820FF);
$queue->concat(1, 0, 0, 1, 20, 10);         // move everything by (20, 10)
$queue->fillRect(0, 0, 60, 40, 0xF2AA4CFF);
$queue->fillEllipse(120, 60, 30, 30, 0xFFFFFF80);
$commands = $queue->finish();

// [['clear', 0x101820FF],
//  ['path', [[[20.0, 10.0], [80.0, 10.0], [80.0, 50.0], [20.0, 50.0]]], 0, 0xF2AA4CFF, 'whole'],
//  ['ellipse', 140.0, 70.0, 30.0, 30.0, 0xFFFFFF80, 'whole']]

// ... the next frame, with the rect moved ...

$queue->changed(320, 200, 1, 1, 320, 200);  // [x, y, width, height, ...] of what differs
```

### Examples

The clip and the fill rule are yours. The queue keeps the clip's rect for its own arithmetic and hands your value back in every command recorded under it:

```php
$queue = new PixelQueue(FillRule::NonZero, FillRule::EvenOdd);
$queue->begin(320, 200, $wholeSurface);
$queue->clip($region, $region->x, $region->y, $region->width, $region->height);
$queue->fillRect(0, 0, 60, 40, 0xF2AA4CFF);     // ['path', ..., FillRule::NonZero, 0xF2AA4CFF, $region]
```

Push and pop keep the transform and the clip:

```php
$queue->push();
$queue->concat(2, 0, 0, 2, 0, 0);
$queue->roundRect(10, 10, 40, 20, [4, 4, 4, 4], 0x2E86DEFF, 0xFFFFFFFF, 1.5);
$queue->pop();
```

A command the queue has no method for is appended as it is, with how to find where it writes:

```php
$queue->append(['image', $source, $placement], PIXEL_QUEUE_PIECES, 0.0, [$x0, $y0, $x1, $y1, $clipX, $clipY, $clipW, $clipH], restless: true);
$queue->append(['path', $contours, $rule, $rgba, $clip], PIXEL_QUEUE_CONTOURS, 1.0);
```

A layer's children are recorded in a list of their own:

```php
$queue->enter($area, $area->x, $area->y, $area->width, $area->height);
$queue->fillRect(0, 0, 10, 10, 0xFF0000FF);
$children = $queue->leave();
```

## Requirements

* PHP 8.4 or later, NTS or ZTS
* Linux or macOS
* A C compiler and the PHP development headers. No system library.

## Installation

### With PHP PIE (recommended)

```bash
pie install php-io-extensions/pixel-queue
```

### Manually (install script)

```bash
./install-macos.sh            # Homebrew php@8.4 and php@8.4-zts
./install-debian-trixie.sh    # Debian trixie, Raspberry Pi OS
```

### Manually (phpize)

```bash
phpize
./configure --enable-pixel-queue
make
sudo make install
```

Then add `extension=pixel_queue` to your `php.ini`.

### Verify

```bash
php -r 'var_dump(extension_loaded("pixel_queue"));'
```

## API

The full reference, with the arithmetic of every shape and of the comparison step by step, is in [`.okf/api/queue.md`](.okf/api/queue.md).

### PixelQueue

| Method | What it does |
|---|---|
| `new PixelQueue(nonZero = 0, evenOdd = 1)` | A queue; the two values are what a `path` carries as its fill rule |
| `begin(width, height, clip)` | Open a frame: no commands, the identity transform, the whole surface as the clip |
| `finish()` | Close the frame and return its commands |
| `abandon()` | Drop the open frame |
| `recording()` | Whether a frame is open |
| `push()`, `pop()`, `depth()` | Keep and bring back the transform and the clip |
| `concat(a, b, c, d, e, f)`, `matrix()` | Multiply the transform; read it |
| `clip(value, x, y, width, height)`, `clipValue()` | Set the clip; read the value given for it |
| `enter(value, x, y, width, height)`, `leave()` | Record a layer's children in a list of their own |
| `append(command, how, ...)` | Record a command made elsewhere |
| `drawn(...)` | The rects the last frame writes |
| `changed(...)` | The rects that differ between the last frame and the one before it |

### Shapes

| Method | Command recorded |
|---|---|
| `clear(rgba)` | `['clear', rgba]` |
| `fillRect(x, y, width, height, rgba)` | `['path', contours, rule, rgba, clip]` |
| `fillTriangle(x0, y0, x1, y1, x2, y2, rgba)` | `['path', ...]` |
| `strokeRect(x, y, width, height, rgba, stroke)` | `['polyline', points, stroke, true, rgba, clip]` |
| `line(x0, y0, x1, y1, rgba, stroke)` | `['polyline', points, stroke, false, rgba, clip]` |
| `fillEllipse(cx, cy, rx, ry, rgba)` | `['ellipse', cx, cy, rx, ry, rgba, clip]`, or a `path` when turned |
| `strokeEllipse(cx, cy, rx, ry, rgba, stroke)` | `['ring', cx, cy, rx, ry, stroke, rgba, clip]`, or a `polyline` when turned |
| `roundRect(x, y, width, height, radii, fill, border, borderWidth)` | `['roundrect', x0, y0, x1, y1, radii, rgba, clip]` and, with a border, `['roundring', x0, y0, x1, y1, radii, inset, rgba, clip]`; `path` commands when turned |
| `shadow(x, y, width, height, radius, blur, rgba, dx, dy)` | `['shadow', x0, y0, x1, y1, radius, blur, rgba, clip]` |

### What is refused

A shape with a number that is not finite, or that lands past `PIXEL_QUEUE_LIMIT` (2^24) once transformed, throws `PixelQueueException`. Its code says why, `$what` names the argument and `$value` is the number. Nothing of the shape is recorded.

A call the queue cannot act on (a `finish()` with no frame, a `pop()` with nothing pushed) is an `Error`; an argument of the wrong form is a `ValueError`.

## Testing

```bash
composer install
vendor/bin/pest
```

## License

MIT. See [LICENSE](LICENSE).
