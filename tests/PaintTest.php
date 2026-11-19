<?php

declare(strict_types=1);

const GRADIENT = ['linear', [0, 0, 10, 0], [[0.0, RED], [1.0, BLUE]], 'pad'];

it('records a gradient as given, with the transform it is drawn through appended', function (): void {
    expect(frame(function (PixelQueue $q): void {
        $q->concat(2, 0, 0, 3, 5, 7);
        $q->fillTriangle(0, 0, 1, 0, 0, 1, GRADIENT);
    }))->toBe([['path', [[[5.0, 7.0], [7.0, 7.0], [5.0, 10.0]]], 'nz', [...GRADIENT, [2.0, 0.0, 0.0, 3.0, 5.0, 7.0]], 'clip']]);
});

it('carries a gradient into every kind of shape that takes a paint', function (): void {
    $with = [...GRADIENT, [1.0, 0.0, 0.0, 1.0, 0.0, 0.0]];
    $kinds = frame(function (PixelQueue $q): void {
        $q->fillRect(0, 0, 4, 4, GRADIENT);
        $q->strokeRect(0, 0, 4, 4, GRADIENT, 1);
        $q->line(0, 0, 4, 4, GRADIENT, 1);
        $q->fillEllipse(5, 5, 2, 2, GRADIENT);
        $q->strokeEllipse(5, 5, 2, 2, GRADIENT, 1);
        $q->roundRect(0, 0, 10, 10, [2, 2, 2, 2], GRADIENT, GRADIENT, 1);
        $q->fillPath([['M', 0, 0], ['L', 4, 0], ['L', 0, 4]], false, GRADIENT);
        $q->strokePath([['M', 0, 0], ['L', 4, 0]], GRADIENT, 1);
    });

    // the paint is the entry before the clip in each
    expect(array_map(fn (array $c) => $c[count($c) - 2], $kinds))->toBe(array_fill(0, 9, $with));
});

it('keeps an int colour an int', function (): void {
    expect(frame(fn (PixelQueue $q) => $q->fillEllipse(5, 5, 2, 2, RED))[0][5])->toBe(RED);
});

it('refuses a paint that is neither an int nor a list of four', function (callable $call): void {
    $queue = opened();

    expect(fn () => $call($queue))->toThrow(ValueError::class)
        ->and($queue->finish())->toBe([]);
})->with([
    'three entries' => [fn (PixelQueue $q) => $q->fillRect(0, 0, 1, 1, ['linear', [0, 0, 1, 0], []])],
    'keys' => [fn (PixelQueue $q) => $q->fillEllipse(0, 0, 1, 1, ['a' => 1, 2, 3, 4])],
    'a border of three' => [fn (PixelQueue $q) => $q->roundRect(0, 0, 4, 4, [1, 1, 1, 1], RED, [1, 2, 3], 1)],
]);
