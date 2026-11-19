<?php

declare(strict_types=1);

function lowered(array $command, string $form): ?array
{
    return (new PixelQueue('nz', 'eo'))->lower($command, $form);
}

it('cuts a square into one trapezoid, and that into two triangles', function (): void {
    $square = ['path', [[[0.0, 0.0], [10.0, 0.0], [10.0, 10.0], [0.0, 10.0]]], 'nz', RED, 'clip'];

    // one band, y 0..10: the left edge x = 0 at top and bottom, the right x = 10
    expect(lowered($square, 'trapezoids'))->toBe(['trapezoids', [[0.0, 10.0, 0.0, 0.0, 10.0, 10.0]], RED, 'clip'])
        // (left top, right top, right bottom), then (left top, right bottom, left bottom)
        ->and(lowered($square, 'triangles'))->toBe(['triangles', [[0.0, 0.0, 10.0, 0.0, 10.0, 10.0], [0.0, 0.0, 10.0, 10.0, 0.0, 10.0]], RED, 'clip'])
        ->and(lowered($square, 'polygon'))->toBe($square);
});

it('keeps the hole of an even-odd path, and fills it under non-zero', function (): void {
    // a 10 square and a 6 square inside it, both the same way round
    $nested = [[[0.0, 0.0], [10.0, 0.0], [10.0, 10.0], [0.0, 10.0]], [[2.0, 2.0], [8.0, 2.0], [8.0, 8.0], [2.0, 8.0]]];

    // bands at the corners' heights: 0..2, 2..8, 8..10. Even-odd: the span x = 0..10 ends at 2, where the hole splits
    // it into 0..2 and 8..10, both of which end at 8, where 0..10 opens again; closed in that order.
    expect(lowered(['path', $nested, 'eo', RED, 'clip'], 'trapezoids')[1])->toBe([
        [0.0, 2.0, 0.0, 0.0, 10.0, 10.0],
        [2.0, 8.0, 0.0, 0.0, 2.0, 2.0],
        [2.0, 8.0, 8.0, 8.0, 10.0, 10.0],
        [8.0, 10.0, 0.0, 0.0, 10.0, 10.0],
    ])->and(lowered(['path', $nested, 'nz', RED, 'clip'], 'trapezoids')[1])->toBe([
        // non-zero: every band is bounded by the same two outer sides, so the trapezoid stays open all the way down
        [0.0, 10.0, 0.0, 0.0, 10.0, 10.0],
    ]);
});

it('lowers its own trapezoids on to triangles, and to nothing else', function (): void {
    // a trapezoid top 0..10 at y 2, bottom 3..7 at y 6 (ints read as floats): triangle one is the top side and the
    // bottom-right corner, triangle two the top-left corner and the bottom side
    $traps = ['trapezoids', [[2, 6, 0, 3, 10, 7]], RED, 'clip'];

    expect(lowered($traps, 'triangles'))->toBe(['triangles', [
        [0.0, 2.0, 10.0, 2.0, 7.0, 6.0],
        [0.0, 2.0, 7.0, 6.0, 3.0, 6.0],
    ], RED, 'clip'])
        ->and(lowered($traps, 'polygon'))->toBeNull()
        ->and(lowered($traps, 'trapezoids'))->toBeNull()
        ->and(lowered(['trapezoids', [[2, 6, 0, 3, 10]], RED, 'clip'], 'triangles'))->toBeNull()   // five numbers
        ->and(lowered(['trapezoids', [[2, 6, 0, 3, 10, 7]], RED], 'triangles'))->toBeNull();       // no clip
});

it('does not cut one shape at the corners of another shape in the same path', function (): void {
    // a tall square x 0..4, y 0..10 and a short one x 6..10, y 3..5: the short one's corners make bands 0..3, 3..5,
    // 5..10, but the tall one is bounded by the same two sides in all three. The short one closes at 5, the tall at 10.
    $apart = [[[0.0, 0.0], [4.0, 0.0], [4.0, 10.0], [0.0, 10.0]], [[6.0, 3.0], [10.0, 3.0], [10.0, 5.0], [6.0, 5.0]]];

    expect(lowered(['path', $apart, 'nz', RED, 'clip'], 'trapezoids')[1])->toBe([
        [3.0, 5.0, 6.0, 6.0, 10.0, 10.0],
        [0.0, 10.0, 0.0, 0.0, 4.0, 4.0],
    ]);
});

it('cuts a band where two edges cross', function (): void {
    // a bowtie: its diagonals cross at (5, 5), so the band 0..10 is cut there. In each half, ordered by x halfway
    // down, the left side x = 0 and the near diagonal make the left lobe, the far diagonal and x = 10 the right one.
    $bowtie = ['path', [[[0.0, 0.0], [10.0, 10.0], [10.0, 0.0], [0.0, 10.0]]], 'nz', RED, 'clip'];

    expect(lowered($bowtie, 'trapezoids')[1])->toBe([
        [0.0, 5.0, 0.0, 0.0, 0.0, 5.0],
        [0.0, 5.0, 10.0, 5.0, 10.0, 10.0],
        [5.0, 10.0, 0.0, 0.0, 5.0, 0.0],
        [5.0, 10.0, 5.0, 10.0, 10.0, 10.0],
    ]);
});

it('outlines a stroke: butt ends, a miter at an outer corner, the corner point at an inner one', function (): void {
    // (0, 0) -> (10, 0) -> (10, 10), width 2. A side's left normal is (-dy, dx) / length x 1.
    // Forward: start (0, 1); at (10, 0) the turn is towards the left, so (10, 1), the corner (10, 0), (9, 0); end (9, 10).
    // Back, along (10, 10) -> (10, 0) -> (0, 0): start (11, 10); the turn is away, so (11, 0), the miter, (10, -1);
    // end (0, -1). The miter (1 / cos 45 = 1.41, under 4) is the corner plus (1, -1) times k = 1 / (2 cos^2): in
    // doubles cos = sqrt(2) / 2 squares to 0.5000000000000001, so k is a bit under 1 and y is -0.9999999999999998.
    expect(lowered(['polyline', [[0.0, 0.0], [10.0, 0.0], [10.0, 10.0]], 2.0, false, RED, 'clip'], 'polygon'))->toBe([
        'path',
        [[[0.0, 1.0], [10.0, 1.0], [10.0, 0.0], [9.0, 0.0], [9.0, 10.0], [11.0, 10.0], [11.0, 0.0], [11.0, -0.9999999999999998], [10.0, -1.0], [0.0, -1.0]]],
        'nz', RED, 'clip',
    ]);
});

it('bevels a corner whose miter is past 4', function (): void {
    // (0, 0) -> (10, 0) -> (0, 1), width 2. Forward the corner is the inner one: (0, 1), (10, 1), the corner (10, 0),
    // then two points on the second side: 5. Back along (0, 1) -> (10, 0) -> (0, 0) it is the outer one: the normals
    // (1, 10) / sqrt 101 and (0, -1) add to a length of 0.0996, so 1 / cos = 2 / 0.0996 = 20, past 4: a bevel, the
    // two sides' points with nothing between them, (10, -1) then the end (0, -1): 4. A miter would reach x = 10 + 20.
    $outline = lowered(['polyline', [[0.0, 0.0], [10.0, 0.0], [0.0, 1.0]], 2.0, false, RED, 'clip'], 'polygon')[1][0];

    expect($outline)->toHaveCount(9)
        ->and(array_slice($outline, 0, 3))->toBe([[0.0, 1.0], [10.0, 1.0], [10.0, 0.0]])
        ->and(array_slice($outline, 7))->toBe([[10.0, -1.0], [0.0, -1.0]])
        ->and(max(array_column($outline, 0)))->toBeLessThan(10.1);
});

it('makes a closed stroke two outlines, and a ring the stroke of its ellipse', function (): void {
    // a closed square's stroke: the left side all round, then the left side of the way back
    $square = lowered(['polyline', [[0.0, 0.0], [10.0, 0.0], [10.0, 10.0], [0.0, 10.0]], 2.0, true, RED, 'clip'], 'polygon');
    // a circle of radius 10: ceil(pi sqrt(10 / 0.2) 1.01) = ceil(22.44) = 23 sides
    $ring = lowered(['ring', 50.0, 40.0, 10.0, 10.0, 2.0, RED, 'clip'], 'polygon');
    $disc = lowered(['ellipse', 50.0, 40.0, 10.0, 10.0, RED, 'clip'], 'polygon');

    expect($square[1])->toHaveCount(2)
        ->and($disc[1])->toHaveCount(1)
        ->and($disc[1][0])->toHaveCount(23)
        ->and($disc[1][0][0])->toBe([60.0, 40.0])
        ->and($disc[2])->toBe('nz')
        ->and($ring[1])->toHaveCount(2)
        ->and($ring[2])->toBe('nz');
});

it('outlines a rounded box, and a rounded ring with its hole even-odd', function (): void {
    // square corners: one point each, clockwise from the top-left
    $box = lowered(['roundrect', 0.0, 0.0, 10.0, 8.0, [0.0, 0.0, 0.0, 0.0], RED, 'clip'], 'polygon');
    // inset 2: the hole is the box 2..8 by 2..6, radii 0 - 2 held to 0
    $ring = lowered(['roundring', 0.0, 0.0, 10.0, 8.0, [0.0, 0.0, 0.0, 0.0], 2.0, BLUE, 'clip'], 'polygon');
    // an inset of 5 leaves no hole: the outline alone, non-zero
    $solid = lowered(['roundring', 0.0, 0.0, 10.0, 8.0, [0.0, 0.0, 0.0, 0.0], 5.0, BLUE, 'clip'], 'polygon');

    expect($box)->toBe(['path', [[[0.0, 0.0], [10.0, 0.0], [10.0, 8.0], [0.0, 8.0]]], 'nz', RED, 'clip'])
        ->and($ring)->toBe(['path', [[[0.0, 0.0], [10.0, 0.0], [10.0, 8.0], [0.0, 8.0]], [[2.0, 2.0], [8.0, 2.0], [8.0, 6.0], [2.0, 6.0]]], 'eo', BLUE, 'clip'])
        ->and($solid)->toBe(['path', [[[0.0, 0.0], [10.0, 0.0], [10.0, 8.0], [0.0, 8.0]]], 'nz', BLUE, 'clip']);
});

it('has no lower form for what has no outline, and refuses a form it does not know', function (): void {
    expect(lowered(['clear', RED], 'polygon'))->toBeNull()
        ->and(lowered(['shadow', 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, RED, 'clip'], 'trapezoids'))->toBeNull()
        ->and(lowered(['glyphs'], 'triangles'))->toBeNull()
        ->and(fn () => lowered(['clear', RED], 'blobs'))->toThrow(ValueError::class);
});

it('packs a form as each layout asks: fixed16 and f32, fields in the given order, repeats allowed', function (): void {
    $square = ['path', [[[0.0, 0.0], [10.0, 0.0], [10.0, 10.0], [0.0, 10.0]]], 'nz', RED, 'clip'];
    $q = new PixelQueue('nz', 'eo');

    // the one trapezoid is [top 0, bottom 10, left 0, 0, right 10, 10]. fixed16 fields 1, 4: 10 and 10, times 65536 =
    // 655360 each. f32 fields 5, 0, 5: 10, 0, 10. One string per layout, one item each.
    expect($q->lower($square, 'trapezoids', [['fixed16', [1, 4]], ['f32', [5, 0, 5]]]))
        ->toBe(['trapezoids', [pack('V*', 655360, 655360), pack('g*', 10.0, 0.0, 10.0)], RED, 'clip'])
        // triangles: (0,0, 10,0, 10,10) and (0,0, 10,10, 0,10); fixed16 field 2 is each one's second x: 10, then 10.
        ->and($q->lower($square, 'triangles', [['fixed16', [2]]])[1])->toBe([pack('V*', 655360, 655360)])
        // a top of -1.5/65536 is -1.5 in fixed16 units, rounded half away from zero to -2: 0xFFFFFFFE as an unsigned
        // int32, in both triangles (field 1 is each one's first y, the top)
        ->and($q->lower(['trapezoids', [[-1.5 / 65536, 1, 0, 0, 1, 1]], RED, null], 'triangles', [['fixed16', [1]]])[1])
        ->toBe([pack('V*', 0xFFFFFFFE, 0xFFFFFFFE)]);
});

it('refuses a packing that is not a list of [encoding, fields] pairs, and never packs a polygon', function (): void {
    $square = ['path', [[[0.0, 0.0], [10.0, 0.0], [10.0, 10.0], [0.0, 10.0]]], 'nz', RED, 'clip'];
    $q = new PixelQueue('nz', 'eo');

    expect(fn () => $q->lower($square, 'trapezoids', [['f64', [0]]]))->toThrow(ValueError::class)
        ->and(fn () => $q->lower($square, 'trapezoids', [['f32', [6]]]))->toThrow(ValueError::class)
        ->and(fn () => $q->lower($square, 'trapezoids', []))->toThrow(ValueError::class)
        ->and($q->lower($square, 'polygon', [['f32', [0]]]))->toBe($square);
});
