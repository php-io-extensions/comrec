<?php

/** @generate-class-entries */

/**
 * A frame of drawing commands, recorded under a transform and a clip, and compared with the frame
 * before it.
 *
 * Between begin() and finish() the shape methods record commands, in order, each a list whose
 * first entry is its kind and whose last entry is the clip it was recorded under:
 *
 *     ['clear', rgba]
 *     ['path', contours, rule, rgba, clip]             contours: list of lists of [x, y]
 *     ['polyline', points, stroke, closed, rgba, clip]
 *     ['ellipse', cx, cy, rx, ry, rgba, clip]
 *     ['ring', cx, cy, rx, ry, stroke, rgba, clip]
 *     ['roundrect', x0, y0, x1, y1, [tl, tr, br, bl], rgba, clip]
 *     ['roundring', x0, y0, x1, y1, [tl, tr, br, bl], inset, rgba, clip]
 *     ['shadow', x0, y0, x1, y1, radius, blur, rgba, clip]
 *     ['glyphs', runs, [a, b, c, d, e, f], rgba, [x0, y0, x1, y1], clip]
 *
 * Numbers are in pixels of the surface, already through the transform. A colour is whatever
 * integer the caller gives. The clip and the fill rule are values the caller gives and the queue
 * hands back untouched: it never looks inside them.
 *
 * A zero or negative size, radius or stroke records nothing.
 *
 * @strict-properties
 * @not-serializable
 */
final class PixelQueue
{
    /**
     * @param mixed $nonZero What a 'path' command carries as its rule when it is filled by the non-zero rule.
     * @param mixed $evenOdd What it carries when it is filled by the even-odd rule.
     */
    public function __construct(mixed $nonZero = 0, mixed $evenOdd = 1) {}

    /**
     * Open a frame: no commands, the identity transform, nothing pushed, and a clip of the whole
     * surface that commands carry as $clip.
     *
     * @throws Error When a frame is open, or a side is not 1..PIXEL_QUEUE_LIMIT.
     */
    public function begin(int $width, int $height, mixed $clip): void {}

    public function recording(): bool {}

    /** Drop the open frame, with every layer entered in it. The last frame stays the last. */
    public function abandon(): void {}

    /**
     * Close the open frame and return its commands. It becomes the last frame; the frame that was
     * the last becomes the one before it.
     *
     * @throws Error When no frame is open.
     */
    public function finish(): array {}

    /** Keep the transform and the clip. */
    public function push(): void {}

    /**
     * Bring back the transform and the clip of the last push().
     *
     * @throws Error When nothing is pushed in the frame, or the layer, being recorded.
     */
    public function pop(): void {}

    /** How many push() calls wait for their pop() in the frame, or the layer, being recorded. */
    public function depth(): int {}

    /**
     * The transform becomes the current one times [a, b, c, d, e, f]: a point goes through the
     * given matrix first. A point (x, y) lands at (a·x + c·y + e, b·x + d·y + f).
     *
     * @throws PixelQueueException When a component of the product is not finite; the transform is unchanged.
     */
    public function concat(float $a, float $b, float $c, float $d, float $e, float $f): void {}

    /** @return array{float, float, float, float, float, float} a, b, c, d, e, f */
    public function matrix(): array {}

    /**
     * The clip becomes this rect, and commands recorded from now carry $clip.
     *
     * @param mixed $clip Handed back in every command recorded under it, and by clipValue().
     */
    public function clip(mixed $clip, int $x, int $y, int $width, int $height): void {}

    /** What clip(), begin() or enter() was last given as the clip. */
    public function clipValue(): mixed {}

    /**
     * Record a command made elsewhere, as it is.
     *
     * @param array $command A list; its first entry is its kind.
     * @param int $how Where it writes: PIXEL_QUEUE_WHOLE, _CONTOURS, _POINTS, _SPANS or _PIECES.
     * @param float $grow For _CONTOURS and _POINTS: how many pixels past its points it writes.
     * @param array|null $pieces For _PIECES: 8 or 16 numbers.
     * @param bool $restless Its pixels can change with nothing in it changing: changed() never finds it equal.
     * @param array|null $reads [x, y, width, height, reach]: it reads the pixels under that rect and $reach pixels around it, so it is redrawn whole when any of them is.
     *
     * @throws Error When no frame is open.
     * @throws ValueError When $how, $pieces or $reads is not as described.
     */
    public function append(array $command, int $how, float $grow = 0.0, ?array $pieces = null, bool $restless = false, ?array $reads = null): void {}

    /**
     * Start a layer: commands are recorded in a list of their own, under this clip, with nothing
     * pushed. The transform stays.
     */
    public function enter(mixed $clip, int $x, int $y, int $width, int $height): void {}

    /**
     * End the layer and return its commands. The commands, the clip and the pushes of before
     * enter() come back; the transform stays as it is.
     *
     * @throws Error When no layer is entered.
     */
    public function leave(): array {}

    /** @throws PixelQueueException */
    public function clear(int $rgba): void {}

    /** @throws PixelQueueException */
    public function fillRect(float $x, float $y, float $width, float $height, int $rgba): void {}

    /** @throws PixelQueueException */
    public function strokeRect(float $x, float $y, float $width, float $height, int $rgba, float $stroke): void {}

    /** @throws PixelQueueException */
    public function line(float $x0, float $y0, float $x1, float $y1, int $rgba, float $stroke): void {}

    /** @throws PixelQueueException */
    public function fillTriangle(float $x0, float $y0, float $x1, float $y1, float $x2, float $y2, int $rgba): void {}

    /**
     * An 'ellipse' under a transform that keeps its axes; a 'path' of straight sides otherwise.
     *
     * @throws PixelQueueException
     */
    public function fillEllipse(float $cx, float $cy, float $rx, float $ry, int $rgba): void {}

    /**
     * A 'ring' under a transform that keeps its axes; a closed 'polyline' otherwise.
     *
     * @throws PixelQueueException
     */
    public function strokeEllipse(float $cx, float $cy, float $rx, float $ry, int $rgba, float $stroke): void {}

    /**
     * A rounded box, with a border inside its edge when $border is not null. Each radius is held
     * to 0..min(width, height) / 2. Under a transform that turns, shears or scales x and y
     * unevenly it is recorded as 'path' commands of straight sides.
     *
     * @param array $radii [tl, tr, br, bl]
     *
     * @throws PixelQueueException
     */
    public function roundRect(float $x, float $y, float $width, float $height, array $radii, int $fill, ?int $border, float $borderWidth): void {}

    /**
     * A blurred box, moved by ($dx, $dy). Needs an upright transform that scales x and y alike.
     *
     * @throws PixelQueueException
     */
    public function shadow(float $x, float $y, float $width, float $height, float $radius, float $blur, int $rgba, float $dx, float $dy): void {}

    /**
     * Text with its top-left at (x, y): ['glyphs', runs, [a, b, c, d, e, f], rgba, [x0, y0, x1, y1], clip].
     * [a .. f] is the transform with (x, y) through it as its e and f. [x0 .. y1] is the box of $ink's
     * four corners, each moved by (x, y) and put through the transform, grown by 1 on every side: it
     * is the command's reach. Nothing is recorded when $ink is null or the clip is empty; x and y are
     * checked, and put through the transform, first. Its refusals name the call 'text'.
     *
     * @param array $runs Carried into the command as it is.
     * @param array|null $ink [x0, y0, x1, y1]: where the text has ink, from its top-left; null for none.
     *
     * @throws PixelQueueException When x or y is not finite, or a point lands past the limit.
     * @throws ValueError When $ink is not null or four finite numbers.
     */
    public function glyphs(array $runs, float $x, float $y, ?array $ink, int $rgba): void {}

    /**
     * Where the last frame writes: the box of every command, then each rect of $also, merged
     * until no two overlap.
     *
     * A command's box is its reach rounded out to whole pixels, held to the surface, inside its
     * clip. Each box is grown to whole units; the right and bottom are held to the limits.
     *
     * @param array $also Rects that count as written: x, y, width, height, four numbers each.
     * @return array<int, int> x, y, width, height, four numbers a rect
     *
     * @throws ValueError When a size is under 1, or $also is not runs of four integers.
     */
    public function drawn(int $surfaceWidth, int $surfaceHeight, int $unitWidth, int $unitHeight, int $limitWidth, int $limitHeight, array $also = []): array {}

    /**
     * Where the last frame differs from the one before it. Commands are compared by position
     * with ==: one equal to the command at the same place before, and not restless, changed
     * nothing. Every other command marks its box, and the box of the command it replaced. Those
     * are merged, then merged with $also. A command that reads is then joined whole, with its
     * reach, to the rects when it overlaps one, until none does.
     *
     * @param array $also As drawn().
     * @return array<int, int> As drawn(); empty when nothing changed.
     *
     * @throws ValueError As drawn().
     */
    public function changed(int $surfaceWidth, int $surfaceHeight, int $unitWidth, int $unitHeight, int $limitWidth, int $limitHeight, array $also = []): array {}
}
