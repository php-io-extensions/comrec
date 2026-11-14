<?php

declare(strict_types=1);

it('records a rect as a path of its four corners', function (): void {
    expect(frame(fn (PixelQueue $q) => $q->fillRect(1.5, 2, 3, 4, RED)))
        ->toBe([['path', [[[1.5, 2.0], [4.5, 2.0], [4.5, 6.0], [1.5, 6.0]]], 'nz', RED, 'clip']]);
});

it('records nothing for a size, a radius or a stroke of zero or less', function (): void {
    expect(frame(function (PixelQueue $q): void {
        $q->fillRect(1, 1, 0, 5, RED);
        $q->fillRect(1, 1, 5, -1, RED);
        $q->strokeRect(1, 1, 5, 5, RED, 0);
        $q->line(1, 1, 5, 5, RED, -2);
        $q->fillEllipse(5, 5, 0, 3, RED);
        $q->strokeEllipse(5, 5, 3, 3, RED, 0);
        $q->roundRect(1, 1, 0, 5, [1, 1, 1, 1], RED, null, 0);
        $q->shadow(1, 1, 5, 0, 1, 1, RED, 0, 0);
    }))->toBe([]);
});

it('records nothing under an empty clip', function (): void {
    expect(frame(function (PixelQueue $q): void {
        $q->clip('none', 5, 5, 0, 9);
        $q->fillRect(1, 1, 5, 5, RED);
        $q->line(1, 1, 5, 5, RED, 1);
        $q->fillEllipse(5, 5, 2, 2, RED);
        $q->roundRect(1, 1, 5, 5, [1, 1, 1, 1], RED, BLUE, 1);
        $q->shadow(1, 1, 5, 5, 1, 1, RED, 0, 0);
    }))->toBe([]);
});

it('records a clear whatever the clip', function (): void {
    expect(frame(function (PixelQueue $q): void {
        $q->clip('none', 0, 0, 0, 0);
        $q->clear(BLUE);
    }))->toBe([['clear', BLUE]]);
});

it('scales a stroke by the square root of the area the transform scales by', function (): void {
    expect(frame(function (PixelQueue $q): void {
        $q->concat(3, 0, 0, 3, 0, 0);
        $q->strokeRect(0, 0, 10, 5, RED, 2);
        $q->line(1, 1, 5, 1, BLUE, 1);
    }))->toBe([
        ['polyline', [[0.0, 0.0], [30.0, 0.0], [30.0, 15.0], [0.0, 15.0]], 6.0, true, RED, 'clip'],
        ['polyline', [[3.0, 3.0], [15.0, 3.0]], 3.0, false, BLUE, 'clip'],
    ]);
});

it('records a triangle as a path of its three points', function (): void {
    expect(frame(fn (PixelQueue $q) => $q->fillTriangle(0, 0, 4, 0, 2, 3, RED)))
        ->toBe([['path', [[[0.0, 0.0], [4.0, 0.0], [2.0, 3.0]]], 'nz', RED, 'clip']]);
});

it('keeps an ellipse an ellipse under a transform that keeps its axes', function (): void {
    expect(frame(function (PixelQueue $q): void {
        $q->concat(2, 0, 0, -3, 0, 30);
        $q->fillEllipse(5, 5, 2, 1, RED);
        $q->strokeEllipse(5, 5, 2, 1, BLUE, 2);
    }))->toBe([
        ['ellipse', 10.0, 15.0, 4.0, 3.0, RED, 'clip'],
        // the stroke: 2 times the square root of |2 x -3|
        ['ring', 10.0, 15.0, 4.0, 3.0, 2 * sqrt(6.0), BLUE, 'clip'],
    ]);
});

it('turns an ellipse into straight sides under a transform that turns or shears it', function (float $radius, int $sides): void {
    $commands = frame(function (PixelQueue $q) use ($radius): void {
        $q->concat(1, 0.5, 0, 1, 0, 0);
        $q->fillEllipse(50, 20, $radius, $radius / 2, RED);
        $q->strokeEllipse(50, 20, $radius, $radius / 2, BLUE, 1);
    });

    // the first point is the ellipse's right end, sheared: (x, 0.5x + y)
    expect($commands[0][0])->toBe('path')
        ->and(count($commands[0][1][0]))->toBe($sides)
        ->and($commands[0][1][0][0])->toBe([50.0 + $radius, 0.5 * (50.0 + $radius) + 20.0])
        ->and($commands[1][0])->toBe('polyline')
        ->and($commands[1][3])->toBeTrue()
        ->and($commands[1][1])->toBe($commands[0][1][0]);
})->with([
    // sides = ceil(pi x sqrt(radius x sqrt(1.25) / 0.2) x 1.01), held to 12..1024
    'a small one keeps 12 sides' => [0.5, 12],
    'radius 5' => [5.0, 17],
    'radius 40' => [40.0, 48],
    'a huge one stops at 1024' => [1.0e6, 1024],
]);

it('holds each radius of a rounded box to half its shorter side', function (): void {
    expect(frame(fn (PixelQueue $q) => $q->roundRect(10, 10, 40, 20, [4, 50, -3, 10.5], RED, null, 0)))
        ->toBe([['roundrect', 10.0, 10.0, 50.0, 30.0, [4.0, 10.0, 0.0, 10.0], RED, 'clip']]);
});

it('records a border as a ring inside the edge, with the fill inside it', function (): void {
    expect(frame(function (PixelQueue $q): void {
        $q->roundRect(10, 10, 40, 20, [4, 4, 4, 4], RED, BLUE, 2);
        // a border as wide as half the box leaves no inside to fill
        $q->roundRect(10, 10, 40, 20, [4, 4, 4, 4], RED, BLUE, 10);
        // a border that is not given, or not wider than 0, is no border
        $q->roundRect(10, 10, 40, 20, [4, 4, 4, 4], RED, BLUE, -1);
    }))->toBe([
        ['roundrect', 12.0, 12.0, 48.0, 28.0, [2.0, 2.0, 2.0, 2.0], RED, 'clip'],
        ['roundring', 10.0, 10.0, 50.0, 30.0, [4.0, 4.0, 4.0, 4.0], 2.0, BLUE, 'clip'],
        ['roundring', 10.0, 10.0, 50.0, 30.0, [4.0, 4.0, 4.0, 4.0], 10.0, BLUE, 'clip'],
        ['roundrect', 10.0, 10.0, 50.0, 30.0, [4.0, 4.0, 4.0, 4.0], RED, 'clip'],
    ]);
});

it('scales a rounded box, its radii and its border with an even transform', function (): void {
    expect(frame(function (PixelQueue $q): void {
        $q->concat(2, 0, 0, 2, 1, 1);
        $q->roundRect(10, 10, 40, 20, [4, 4, 4, 4], RED, BLUE, 2);
    }))->toBe([
        ['roundrect', 25.0, 25.0, 97.0, 57.0, [4.0, 4.0, 4.0, 4.0], RED, 'clip'],
        ['roundring', 21.0, 21.0, 101.0, 61.0, [8.0, 8.0, 8.0, 8.0], 4.0, BLUE, 'clip'],
    ]);
});

it('turns a rounded box into paths under a transform that is not even', function (): void {
    $commands = frame(function (PixelQueue $q): void {
        $q->concat(2, 0, 0, 1, 0, 0);
        $q->roundRect(10, 10, 40, 20, [0, 0, 0, 0], RED, null, 0);
        $q->roundRect(10, 10, 40, 20, [0, 0, 0, 0], RED, BLUE, 2);
    });

    expect($commands)->toBe([
        // square corners: one point each
        ['path', [[[20.0, 10.0], [100.0, 10.0], [100.0, 30.0], [20.0, 30.0]]], 'nz', RED, 'clip'],
        ['path', [[[24.0, 12.0], [96.0, 12.0], [96.0, 28.0], [24.0, 28.0]]], 'nz', RED, 'clip'],
        ['path', [[[20.0, 10.0], [100.0, 10.0], [100.0, 30.0], [20.0, 30.0]], [[24.0, 12.0], [96.0, 12.0], [96.0, 28.0], [24.0, 28.0]]], 'eo', BLUE, 'clip'],
    ]);
});

it('gives a turned rounded corner a quarter of its circle\'s sides', function (): void {
    $commands = frame(function (PixelQueue $q): void {
        $q->concat(2, 0, 0, 1, 0, 0);
        $q->roundRect(0, 0, 40, 40, [10, 0, 0, 0], RED, null, 0);
    });

    // a circle of radius 10 stretched by 2: ceil(pi x sqrt(20 / 0.2) x 1.01) = 32 sides, 8 steps a corner, 9 points
    expect(count($commands[0][1][0]))->toBe(9 + 3)
        ->and($commands[0][1][0][0])->toBe([2 * (10 + 10 * cos(M_PI)), 10 + 10 * sin(M_PI)])
        ->and($commands[0][1][0][9])->toBe([80.0, 0.0]);
});

it('records a shadow moved by its offset, its radius held and its blur never under 0', function (): void {
    expect(frame(function (PixelQueue $q): void {
        $q->shadow(10, 10, 40, 20, 30, 8, RED, 2, 3);
        $q->shadow(10, 10, 40, 20, -1, -5, RED, 0, 0);
        $q->concat(2, 0, 0, 2, 0, 0);
        $q->shadow(10, 10, 40, 20, 4, 8, BLUE, 0, 0);
    }))->toBe([
        ['shadow', 12.0, 13.0, 52.0, 33.0, 10.0, 8.0, RED, 'clip'],
        ['shadow', 10.0, 10.0, 50.0, 30.0, 0.0, 0.0, RED, 'clip'],
        ['shadow', 20.0, 20.0, 100.0, 60.0, 8.0, 16.0, BLUE, 'clip'],
    ]);
});

it('refuses a shape, saying which number and why', function (callable $call, array $refusal): void {
    $queue = opened();

    expect(refusal(fn () => $call($queue)))->toBe($refusal)
        ->and($queue->finish())->toBe([]);
})->with([
    'a number that is not finite' => [fn (PixelQueue $q) => $q->fillRect(NAN, 0, 1, 1, RED), [PIXEL_QUEUE_NOT_FINITE, 'x', 0.0]],
    'the first one in the order of the arguments' => [fn (PixelQueue $q) => $q->fillRect(0, 0, INF, NAN, RED), [PIXEL_QUEUE_NOT_FINITE, 'width', 0.0]],
    'even when the shape is empty' => [fn (PixelQueue $q) => $q->strokeRect(0, 0, -1, 1, RED, INF), [PIXEL_QUEUE_NOT_FINITE, 'stroke', 0.0]],
    'a line end' => [fn (PixelQueue $q) => $q->line(0, 0, 1, -INF, RED, 1), [PIXEL_QUEUE_NOT_FINITE, 'y1', 0.0]],
    'a triangle corner' => [fn (PixelQueue $q) => $q->fillTriangle(0, 0, 1, 1, NAN, 0, RED), [PIXEL_QUEUE_NOT_FINITE, 'x2', 0.0]],
    'an ellipse radius' => [fn (PixelQueue $q) => $q->fillEllipse(0, 0, 1, INF, RED), [PIXEL_QUEUE_NOT_FINITE, 'ry', 0.0]],
    'a border width' => [fn (PixelQueue $q) => $q->roundRect(0, 0, 1, 1, [0, 0, 0, 0], RED, null, NAN), [PIXEL_QUEUE_NOT_FINITE, 'borderWidth', 0.0]],
    'a shadow offset' => [fn (PixelQueue $q) => $q->shadow(0, 0, 1, 1, 0, 0, RED, 0, NAN), [PIXEL_QUEUE_NOT_FINITE, 'dy', 0.0]],
    'a radius of a rounded box' => [fn (PixelQueue $q) => $q->roundRect(0, 0, 1, 1, [0, 0, INF, 0], RED, null, 0), [PIXEL_QUEUE_NOT_FINITE, 'radius 2', 0.0]],
    'three radii' => [fn (PixelQueue $q) => $q->roundRect(0, 0, 1, 1, [1, 2, 3], RED, null, 0), [PIXEL_QUEUE_BAD_RADII, '', 0.0]],
    'a radius that is not a number' => [fn (PixelQueue $q) => $q->roundRect(0, 0, 1, 1, [1, '2', 3, 4], RED, null, 0), [PIXEL_QUEUE_BAD_RADII, '', 0.0]],
    'radii with keys' => [fn (PixelQueue $q) => $q->roundRect(0, 0, 1, 1, ['a' => 1, 2, 3, 4], RED, null, 0), [PIXEL_QUEUE_BAD_RADII, '', 0.0]],
    'a corner past the limit' => [fn (PixelQueue $q) => $q->fillRect(2.0e7, 0, 1, 1, RED), [PIXEL_QUEUE_PAST_LIMIT, 'x', 2.0e7]],
    'a corner past the limit once transformed' => [fn (PixelQueue $q) => [$q->concat(1, 0, 0, 100, 0, 0), $q->fillRect(0, 2.0e5, 1, 1, RED)], [PIXEL_QUEUE_PAST_LIMIT, 'y', 2.0e7]],
    'a line end past the limit' => [fn (PixelQueue $q) => $q->line(0, 0, -3.0e7, 0, RED, 1), [PIXEL_QUEUE_PAST_LIMIT, 'x1', -3.0e7]],
    'a stroke past the limit once scaled' => [fn (PixelQueue $q) => [$q->concat(1.0e4, 0, 0, 1.0e4, 0, 0), $q->line(0, 0, 1, 1, RED, 1.0e4)], [PIXEL_QUEUE_PAST_LIMIT, 'stroke', 1.0e8]],
    'an ellipse centre' => [fn (PixelQueue $q) => $q->fillEllipse(0, 9.0e7, 1, 1, RED), [PIXEL_QUEUE_PAST_LIMIT, 'cy', 9.0e7]],
    'an ellipse radius once scaled' => [fn (PixelQueue $q) => [$q->concat(1.0e4, 0, 0, 1, 0, 0), $q->fillEllipse(0, 0, 1.0e4, 1, RED)], [PIXEL_QUEUE_PAST_LIMIT, 'rx', 1.0e8]],
    'a shadow under a transform that is not even' => [fn (PixelQueue $q) => [$q->concat(2, 0, 0, 1, 0, 0), $q->shadow(0, 0, 5, 5, 1, 1, RED, 0, 0)], [PIXEL_QUEUE_UNEVEN, 'shadow', 0.0]],
]);

it('refuses a shape when no frame is open, naming the method', function (callable $call, string $method): void {
    expect(refusal(fn () => $call(new PixelQueue())))->toBe([PIXEL_QUEUE_NOT_RECORDING, $method, 0.0]);
})->with([
    [fn (PixelQueue $q) => $q->clear(RED), 'clear'],
    [fn (PixelQueue $q) => $q->fillRect(NAN, 0, 1, 1, RED), 'fillRect'],
    [fn (PixelQueue $q) => $q->strokeRect(0, 0, 1, 1, RED, 1), 'strokeRect'],
    [fn (PixelQueue $q) => $q->line(0, 0, 1, 1, RED, 1), 'line'],
    [fn (PixelQueue $q) => $q->fillTriangle(0, 0, 1, 1, 2, 0, RED), 'fillTriangle'],
    [fn (PixelQueue $q) => $q->fillEllipse(0, 0, 1, 1, RED), 'fillEllipse'],
    [fn (PixelQueue $q) => $q->strokeEllipse(0, 0, 1, 1, RED, 1), 'strokeEllipse'],
    [fn (PixelQueue $q) => $q->roundRect(0, 0, 1, 1, [1], RED, null, 0), 'roundRect'],
    [fn (PixelQueue $q) => $q->shadow(0, 0, 1, 1, 0, 0, RED, 0, 0), 'shadow'],
]);
