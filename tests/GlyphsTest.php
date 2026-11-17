<?php

declare(strict_types=1);

// glyphs(): text recorded as one command, its origin and its ink's box through the transform.

it('records text with the transform\'s origin moved to the text, and its ink\'s box a pixel out as its reach', function (): void {
    $runs = ['run a', 'run b'];

    expect(frame(function (PixelQueue $q) use ($runs): void {
        $q->concat(2, 0, 0, 2, 10, 20);
        $q->glyphs($runs, 3, 4, [1, -5, 9, 2], RED);
    }))->toBe([[
        'glyphs', $runs,
        // e = 2·3 + 10, f = 2·4 + 20
        [2.0, 0.0, 0.0, 2.0, 16.0, 28.0],
        RED,
        // corners (3+1, 4-5) and (3+9, 4+2) through the transform: (18, 18) and (34, 32); then a pixel out
        [17.0, 17.0, 35.0, 33.0],
        'clip',
    ]]);
});

it('reaches as far as the four corners of the ink\'s box once turned', function (): void {
    [$command] = frame(function (PixelQueue $q): void {
        // an eighth turn, scaled by the square root of 2: (x, y) lands at (x - y, x + y), then moved by (50, 0)
        $q->concat(1, 1, -1, 1, 50, 0);
        $q->glyphs([], 0, 0, [0, 0, 10, 4], RED);
    });

    // corners (0,0) (10,0) (10,4) (0,4) land at (50,0) (60,10) (56,14) (46,4): each is the furthest one way
    expect($command[2])->toBe([1.0, 1.0, -1.0, 1.0, 50.0, 0.0])
        ->and($command[4])->toBe([45.0, -1.0, 61.0, 15.0]);
});

it('records nothing for text with no ink, or under an empty clip', function (): void {
    expect(frame(function (PixelQueue $q): void {
        $q->glyphs(['run'], 1, 1, null, RED);
        $q->clip('none', 5, 5, 0, 9);
        $q->glyphs(['run'], 1, 1, [0, 0, 5, 5], RED);
    }))->toBe([]);
});

it('checks the origin even when nothing would be recorded', function (): void {
    $queue = opened();

    expect(refusal(fn () => $queue->glyphs([], NAN, 0, null, RED)))->toBe([PIXEL_QUEUE_NOT_FINITE, 'x', 0.0]);
});

it('answers a text\'s box from its reach, and finds equal text unchanged', function (): void {
    $queue = finished(fn (PixelQueue $q) => $q->glyphs(['run'], 10.5, 10, [0, 0, 5, 3], RED));

    // reach [9.5, 9, 16.5, 14]: rounded out
    expect(drawn($queue))->toBe([[9, 9, 8, 5]]);

    finished(fn (PixelQueue $q) => $q->glyphs(['run'], 10.5, 10, [0, 0, 5, 3], RED), $queue);
    expect(changed($queue))->toBe([]);

    finished(fn (PixelQueue $q) => $q->glyphs(['other run'], 10.5, 10, [0, 0, 5, 3], RED), $queue);
    expect(changed($queue))->toBe([[9, 9, 8, 5]]);
});

it('refuses text, naming the call \'text\' and the number', function (callable $call, array $refusal): void {
    $queue = opened();

    expect(refusal(fn () => $call($queue)))->toBe($refusal)
        ->and($queue->finish())->toBe([]);
})->with([
    'a y that is not finite' => [fn (PixelQueue $q) => $q->glyphs([], 0, INF, [0, 0, 1, 1], RED), [PIXEL_QUEUE_NOT_FINITE, 'y', 0.0]],
    'an origin past the limit' => [fn (PixelQueue $q) => $q->glyphs([], 2.0e7, 0, [0, 0, 1, 1], RED), [PIXEL_QUEUE_PAST_LIMIT, 'x', 2.0e7]],
    'a corner of the ink past the limit once transformed' => [fn (PixelQueue $q) => [$q->concat(1, 0, 0, 100, 0, 0), $q->glyphs([], 0, 0, [0, 0, 1, 2.0e5], RED)], [PIXEL_QUEUE_PAST_LIMIT, 'y', 2.0e7]],
]);

it('refuses text when no frame is open', function (): void {
    expect(refusal(fn () => (new PixelQueue())->glyphs([], 0, 0, null, RED)))->toBe([PIXEL_QUEUE_NOT_RECORDING, 'text', 0.0]);
});

it('refuses an ink box that is not four finite numbers', function (array $ink): void {
    expect(fn () => opened()->glyphs([], 0, 0, $ink, RED))->toThrow(ValueError::class);
})->with([[[0, 0, 1]], [[0, 0, 1, NAN]], [['a' => 0, 0, 1, 1]], [[0, '0', 1, 1]]]);
