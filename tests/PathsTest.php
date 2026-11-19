<?php

declare(strict_types=1);

it('records every subpath of a path through the transform, with the rule value given for it', function (): void {
    // x -> 2x + 10, y -> 2y
    expect(frame(function (PixelQueue $q): void {
        $q->concat(2, 0, 0, 2, 10, 0);
        $q->fillPath([['M', 0, 0], ['L', 10, 0], ['L', 5, 5], ['Z'], ['M', 1, 1], ['L', 2, 1], ['L', 2, 2]], true, RED);
        $q->fillPath([['M', 0, 0], ['L', 1, 0], ['L', 0, 1]], false, BLUE);
    }))->toBe([
        ['path', [[[10.0, 0.0], [30.0, 0.0], [20.0, 10.0]], [[12.0, 2.0], [14.0, 2.0], [14.0, 4.0]]], 'eo', RED, 'clip'],
        ['path', [[[10.0, 0.0], [12.0, 0.0], [10.0, 2.0]]], 'nz', BLUE, 'clip'],
    ]);
});

it('flattens a quadratic into ceil(sqrt(|p0 - 2p1 + p2|)) pieces at even steps of t', function (): void {
    // |(0, 0) - 2(4, 8) + (8, 0)| = 16: 4 pieces, t = 1/4, 1/2, 3/4; x = 8t, y = 2t(1 - t) 8 = 16t(1 - t)
    expect(frame(fn (PixelQueue $q) => $q->fillPath([['M', 0, 0], ['Q', 4, 8, 8, 0]], false, RED)))
        ->toBe([['path', [[[0.0, 0.0], [2.0, 3.0], [4.0, 4.0], [6.0, 3.0], [8.0, 0.0]]], 'nz', RED, 'clip']]);
});

it('flattens a cubic into ceil(sqrt(3 x its largest second difference)) pieces', function (): void {
    // second differences |(0,0) - 2(0,12) + (12,12)| = |(12, -12)| and |(0,12) - 2(12,12) + (12,0)| = |(-12, -12)|,
    // each 12 sqrt 2 = 16.97; sqrt(3 x 16.97) = 7.14: 8 pieces. At t = 1/2: x = 3/8 0 + 3/8 12 + 1/8 12 = 6, y = 3/8 12 + 3/8 12 = 9
    $path = frame(fn (PixelQueue $q) => $q->fillPath([['M', 0, 0], ['C', 0, 12, 12, 12, 12, 0]], false, RED))[0][1][0];

    expect(count($path))->toBe(9)
        ->and($path[4])->toBe([6.0, 9.0])
        ->and($path[8])->toBe([12.0, 0.0]);
});

it('keeps a straight curve one piece', function (): void {
    expect(frame(fn (PixelQueue $q) => $q->fillPath([['M', 0, 0], ['Q', 5, 0, 10, 0], ['L', 10, 5]], false, RED)))
        ->toBe([['path', [[[0.0, 0.0], [10.0, 0.0], [10.0, 5.0]]], 'nz', RED, 'clip']]);
});

it('flattens a curve after the transform, so a scaled curve gets more pieces', function (): void {
    // under scale 4 the quadratic's second difference is 64: 8 pieces
    expect(count(frame(function (PixelQueue $q): void {
        $q->concat(4, 0, 0, 4, 0, 0);
        $q->fillPath([['M', 0, 0], ['Q', 4, 8, 8, 0]], false, RED);
    })[0][1][0]))->toBe(9);
});

it('writes a path within its points and a pixel round them', function (): void {
    // points x 2..6, y 3..8; a pixel out: x 1..7, y 2..9
    expect(drawn(finished(fn (PixelQueue $q) => $q->fillPath([['M', 2, 3], ['L', 6, 3], ['L', 4, 8]], false, RED))))
        ->toBe([[1, 2, 6, 7]]);
});

it('records a polyline a subpath, closed where a Z closes it, its stroke scaled as a line\'s', function (): void {
    // x, y -> 3x, 3y; the stroke by sqrt(9) = 3. After a Z, a segment starts a new subpath where the closed one began.
    expect(frame(function (PixelQueue $q): void {
        $q->concat(3, 0, 0, 3, 0, 0);
        $q->strokePath([['M', 1, 1], ['L', 5, 1], ['L', 5, 4], ['Z'], ['L', 1, 4]], RED, 1);
    }))->toBe([
        ['polyline', [[3.0, 3.0], [15.0, 3.0], [15.0, 12.0]], 3.0, true, RED, 'clip'],
        ['polyline', [[3.0, 3.0], [3.0, 12.0]], 3.0, false, RED, 'clip'],
    ]);
});

it('writes a stroked path within twice its stroke and a pixel round its points', function (): void {
    // points x 10..20, y 10; 2 x 2 + 1 = 5 out: x 5..25, y 5..15
    expect(drawn(finished(fn (PixelQueue $q) => $q->strokePath([['M', 10, 10], ['L', 20, 10]], RED, 2))))
        ->toBe([[5, 5, 20, 10]]);
});

it('records nothing for no ops, a subpath of one point stroked, a stroke of no width, or under an empty clip', function (): void {
    expect(frame(function (PixelQueue $q): void {
        $q->fillPath([], false, RED);
        $q->strokePath([], RED, 1);
        $q->strokePath([['M', 1, 1]], RED, 1);
        $q->strokePath([['M', 1, 1], ['L', 2, 2]], RED, 0);
        $q->clip('none', 5, 5, 0, 9);
        $q->fillPath([['M', 0, 0], ['L', 5, 0], ['L', 0, 5]], false, RED);
        $q->strokePath([['M', 0, 0], ['L', 5, 5]], RED, 1);
    }))->toBe([]);
});

it('refuses a path, saying which number and why', function (callable $call, array $refusal): void {
    $queue = opened();

    expect(refusal(fn () => $call($queue)))->toBe($refusal)
        ->and($queue->finish())->toBe([]);
})->with([
    'an x that is not finite' => [fn (PixelQueue $q) => $q->fillPath([['M', 0, 0], ['L', NAN, 1]], false, RED), [PIXEL_QUEUE_NOT_FINITE, 'x', 0.0]],
    'a control point that is not finite' => [fn (PixelQueue $q) => $q->strokePath([['M', 0, 0], ['C', 1, 1, 2, -INF, 3, 3]], RED, 1), [PIXEL_QUEUE_NOT_FINITE, 'y', 0.0]],
    'the ops before the stroke' => [fn (PixelQueue $q) => $q->strokePath([['M', 0, NAN], ['L', 1, 1]], RED, INF), [PIXEL_QUEUE_NOT_FINITE, 'y', 0.0]],
    'a stroke that is not finite' => [fn (PixelQueue $q) => $q->strokePath([['M', 0, 0], ['L', 1, 1]], RED, NAN), [PIXEL_QUEUE_NOT_FINITE, 'stroke', 0.0]],
    'a point past the limit once transformed' => [fn (PixelQueue $q) => [$q->concat(1, 0, 0, 100, 0, 0), $q->fillPath([['M', 0, 0], ['Q', 1, 2.0e5, 2, 0]], false, RED)], [PIXEL_QUEUE_PAST_LIMIT, 'y', 2.0e7]],
    'a stroke past the limit once scaled' => [fn (PixelQueue $q) => [$q->concat(1.0e4, 0, 0, 1.0e4, 0, 0), $q->strokePath([['M', 0, 0], ['L', 1, 1]], RED, 1.0e4)], [PIXEL_QUEUE_PAST_LIMIT, 'stroke', 1.0e8]],
]);

it('refuses ops of the wrong form', function (callable $call): void {
    $queue = opened();

    expect(fn () => $call($queue))->toThrow(ValueError::class)
        ->and($queue->finish())->toBe([]);
})->with([
    'ops with keys' => [fn (PixelQueue $q) => $q->fillPath(['a' => ['M', 0, 0]], false, RED)],
    'an op that is not a list' => [fn (PixelQueue $q) => $q->fillPath([5], false, RED)],
    'a letter it does not know' => [fn (PixelQueue $q) => $q->fillPath([['M', 0, 0], ['A', 1, 1]], false, RED)],
    'too few numbers' => [fn (PixelQueue $q) => $q->fillPath([['M', 0, 0], ['Q', 1, 1]], false, RED)],
    'a number that is not one' => [fn (PixelQueue $q) => $q->strokePath([['M', 0, 0], ['L', 1, '2']], RED, 1)],
    'no M first' => [fn (PixelQueue $q) => $q->strokePath([['L', 1, 1]], RED, 1)],
]);

it('refuses a path when no frame is open, naming the method', function (callable $call, string $method): void {
    expect(refusal(fn () => $call(new PixelQueue())))->toBe([PIXEL_QUEUE_NOT_RECORDING, $method, 0.0]);
})->with([
    [fn (PixelQueue $q) => $q->fillPath([['M', 0, 0], ['L', 1, 0], ['L', 0, 1]], false, RED), 'fillPath'],
    [fn (PixelQueue $q) => $q->strokePath([['M', 0, 0], ['L', 1, 1]], RED, 1), 'strokePath'],
]);
