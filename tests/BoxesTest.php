<?php

declare(strict_types=1);

it('answers a shape\'s box: its reach a pixel out, rounded out to whole pixels', function (callable $draw, array $box): void {
    expect(drawn(finished($draw)))->toBe([$box]);
})->with([
    // 10.5..15.5 a pixel out is 9.5..16.5: pixels 9 to 16
    'a rect' => [fn (PixelQueue $q) => $q->fillRect(10.5, 20.5, 5, 5, RED), [9, 19, 8, 8]],
    'a rect on whole pixels' => [fn (PixelQueue $q) => $q->fillRect(10, 20, 5, 5, RED), [9, 19, 7, 7]],
    'held to the surface' => [fn (PixelQueue $q) => $q->fillRect(-5, -5, 10, 10, RED), [0, 0, 6, 6]],
    'held to its clip' => [fn (PixelQueue $q) => [$q->clip('c', 2, 3, 3, 2), $q->fillRect(-5, -5, 30, 30, RED)], [2, 3, 3, 2]],
    // a stroke reaches 2 x stroke + 1 past its points
    'a line' => [fn (PixelQueue $q) => $q->line(10, 10, 20, 10, RED, 2), [5, 5, 20, 10]],
    'an ellipse' => [fn (PixelQueue $q) => $q->fillEllipse(50, 40, 10, 5, RED), [39, 34, 22, 12]],
    'a ring, half its stroke further' => [fn (PixelQueue $q) => $q->strokeEllipse(50, 40, 10, 5, RED, 4), [37, 32, 26, 16]],
    'a rounded box' => [fn (PixelQueue $q) => $q->roundRect(10, 10, 40, 20, [4, 4, 4, 4], RED, null, 0), [9, 9, 42, 22]],
    'a shadow, its blur further' => [fn (PixelQueue $q) => $q->shadow(10, 10, 20, 10, 0, 4, RED, 0, 0), [5, 5, 30, 20]],
    'a clear: the whole surface' => [fn (PixelQueue $q) => $q->clear(RED), [0, 0, 100, 80]],
]);

it('answers no box for what is off the surface or outside its clip', function (): void {
    expect(drawn(finished(function (PixelQueue $q): void {
        $q->fillRect(200, 10, 5, 5, RED);
        $q->fillRect(10, -50, 5, 5, RED);
        $q->clip('c', 60, 60, 5, 5);
        $q->fillRect(10, 10, 5, 5, RED);
    })))->toBe([]);
});

it('joins boxes that overlap, and keeps apart those that do not, in the order they were drawn', function (): void {
    expect(drawn(finished(function (PixelQueue $q): void {
        $q->fillRect(50, 50, 5, 5, RED);
        $q->fillRect(10, 10, 5, 5, RED);
        $q->fillRect(14, 12, 5, 5, RED);
        // touches the first two only once they are joined to it
        $q->fillRect(18, 10, 40, 42, RED);
    })))->toBe([[9, 9, 50, 47]]);

    expect(drawn(finished(function (PixelQueue $q): void {
        $q->fillRect(50, 50, 5, 5, RED);
        $q->fillRect(10, 10, 5, 5, RED);
        $q->fillRect(14, 12, 5, 5, RED);
    })))->toBe([[49, 49, 7, 7], [9, 9, 11, 9]]);
});

it('grows each box to whole units, and holds its right and bottom to the limits', function (): void {
    $queue = finished(function (PixelQueue $q): void {
        $q->fillRect(10.5, 20.5, 5, 5, RED);
        $q->fillRect(91, 60, 5, 5, RED);
    });

    // [9, 19, 8, 8] in units of 16 by 8 is 0..32 by 16..32; [90, 59, 7, 7] is 80..112 by 56..72, held to 100
    expect(rects($queue->drawn(100, 80, 16, 8, 100, 80)))->toBe([[0, 16, 32, 16], [80, 56, 20, 16]])
        ->and(rects($queue->drawn(100, 80, 100, 80, 100, 80)))->toBe([[0, 0, 100, 80]]);
});

it('counts the rects it is also given as drawn', function (): void {
    $queue = finished(fn (PixelQueue $q) => $q->fillRect(10, 10, 5, 5, RED));

    expect(drawn($queue, [60, 60, 4, 4, 12, 12, 10, 10]))->toBe([[60, 60, 4, 4], [9, 9, 13, 13]])
        ->and(drawn(new PixelQueue(), [1, 2, 3, 4]))->toBe([[1, 2, 3, 4]]);
});

it('finds where a command made elsewhere writes, by how it was appended', function (array $command, array $how, array $box): void {
    expect(drawn(finished(fn (PixelQueue $q) => $q->append($command, ...$how))))->toBe([$box]);
})->with([
    'contours, grown' => [['path', [[[1, 1], [5, 1], [5, 5]], [[20.5, 2], [22, 2], [22, 9.25]]], 0, RED, 'c'], [PIXEL_QUEUE_CONTOURS, 1.0], [0, 0, 23, 11]],
    'points, grown' => [['polyline', [[10, 10], [20, 10]], 1.0, false, RED, 'c'], [PIXEL_QUEUE_POINTS, 3.0], [7, 7, 16, 6]],
    // two spans: row 3 from x 4, five long; row 7 from x 2, three long
    'spans' => [['spans', pack('vvvC', 3, 4, 5, 255).pack('vvvC', 7, 2, 3, 255), RED, 'c'], [PIXEL_QUEUE_SPANS], [2, 3, 7, 5]],
    'one piece' => [['image'], [PIXEL_QUEUE_PIECES, 0.0, [10.2, 10.2, 20.7, 15.1, 0, 0, 100, 80]], [10, 10, 11, 6]],
    'one piece inside its own clip' => [['image'], [PIXEL_QUEUE_PIECES, 0.0, [10.2, 10.2, 20.7, 15.1, 12, 0, 4, 80]], [12, 10, 4, 6]],
    'two pieces, joined' => [['glass'], [PIXEL_QUEUE_PIECES, 0.0, [10, 10, 20, 20, 0, 0, 100, 80, 40, 30, 50, 35, 0, 0, 100, 80]], [10, 10, 40, 25]],
    'two pieces, one of them off the surface' => [['glass'], [PIXEL_QUEUE_PIECES, 0.0, [200, 10, 220, 20, 0, 0, 100, 80, 40, 30, 50, 35, 0, 0, 100, 80]], [40, 30, 10, 5]],
    'the whole surface' => [['clear', RED], [PIXEL_QUEUE_WHOLE], [0, 0, 100, 80]],
]);

it('keeps a command made elsewhere as it was given', function (): void {
    $object = new stdClass();
    $command = ['layer', $object, [['path', [[[1, 1], [2, 1], [2, 2]]], 0, RED, 'c']], 'c'];

    expect(frame(fn (PixelQueue $q) => $q->append($command, PIXEL_QUEUE_PIECES, 0.0, [0, 0, 1, 1, 0, 0, 1, 1])))->toBe([$command]);
});

it('answers no box for a command whose points or spans are not there', function (array $command, int $how): void {
    expect(drawn(finished(fn (PixelQueue $q) => $q->append($command, $how, 1.0))))->toBe([]);
})->with([
    'no contours' => [['path', [], 0, RED, 'c'], PIXEL_QUEUE_CONTOURS],
    'a contour that is not points' => [['path', [[1, 2, 3]], 0, RED, 'c'], PIXEL_QUEUE_CONTOURS],
    'no second entry' => [['path'], PIXEL_QUEUE_POINTS],
    'a point of one number' => [['polyline', [[1]], 1.0, false, RED, 'c'], PIXEL_QUEUE_POINTS],
    'spans shorter than one' => [['spans', 'abc', RED, 'c'], PIXEL_QUEUE_SPANS],
    'spans that are not bytes' => [['spans', [1, 2], RED, 'c'], PIXEL_QUEUE_SPANS],
]);

it('finds nothing changed between two frames of equal commands', function (): void {
    $draw = function (PixelQueue $q): void {
        $q->clear(BLUE);
        $q->fillRect(10, 10, 5, 5, RED);
        $q->strokeEllipse(40, 40, 6, 6, RED, 2);
    };
    $queue = finished($draw);
    finished($draw, $queue);

    expect(changed($queue))->toBe([])
        ->and(changed($queue, [1, 1, 2, 2]))->toBe([[1, 1, 2, 2]]);
});

it('marks a changed command\'s box and the box of the command it replaced', function (): void {
    $queue = finished(function (PixelQueue $q): void {
        $q->clear(BLUE);
        $q->fillRect(10, 10, 5, 5, RED);
        $q->fillRect(70, 60, 5, 5, RED);
    });
    finished(function (PixelQueue $q): void {
        $q->clear(BLUE);
        $q->fillRect(40, 10, 5, 5, RED);
        $q->fillRect(70, 60, 5, 5, RED);
    }, $queue);

    expect(changed($queue))->toBe([[9, 9, 7, 7], [39, 9, 7, 7]]);
});

it('marks a command that has none at its place in the other frame', function (): void {
    $two = function (PixelQueue $q): void {
        $q->clear(BLUE);
        $q->fillRect(10, 10, 5, 5, RED);
    };
    $three = function (PixelQueue $q) use ($two): void {
        $two($q);
        $q->fillRect(60, 50, 5, 5, RED);
    };

    $more = finished($two);
    finished($three, $more);
    $fewer = finished($three);
    finished($two, $fewer);

    expect(changed($more))->toBe([[59, 49, 7, 7]])
        ->and(changed($fewer))->toBe([[59, 49, 7, 7]]);
});

it('compares by position: a command put first makes every one after it differ', function (): void {
    $queue = finished(function (PixelQueue $q): void {
        $q->fillRect(10, 10, 5, 5, RED);
        $q->fillRect(70, 60, 5, 5, RED);
    });
    finished(function (PixelQueue $q): void {
        $q->fillRect(40, 30, 2, 2, RED);
        $q->fillRect(10, 10, 5, 5, RED);
        $q->fillRect(70, 60, 5, 5, RED);
    }, $queue);

    // place 0: the old first rect and the new one; place 1: the old second and the first again; place 2: the second again
    expect(changed($queue))->toBe([[39, 29, 4, 4], [9, 9, 7, 7], [69, 59, 7, 7]]);
});

it('marks everything in the first frame there is', function (): void {
    expect(changed(finished(fn (PixelQueue $q) => $q->fillRect(10, 10, 5, 5, RED))))->toBe([[9, 9, 7, 7]]);
});

it('never finds a restless command equal', function (): void {
    $draw = fn (bool $restless) => function (PixelQueue $q) use ($restless): void {
        $q->clear(BLUE);
        $q->append(['image', 'source'], PIXEL_QUEUE_PIECES, 0.0, [20, 20, 30, 25, 0, 0, 100, 80], $restless);
    };
    $still = finished($draw(false));
    finished($draw(false), $still);
    $restless = finished($draw(true));
    finished($draw(true), $restless);

    expect(changed($still))->toBe([])
        ->and(changed($restless))->toBe([[20, 20, 10, 5]]);
});

it('redraws a command that reads under itself, whole and with its reach, when a change touches what it reads', function (): void {
    $draw = fn (float $x) => function (PixelQueue $q) use ($x): void {
        $q->clear(BLUE);
        $q->fillRect($x, 44, 5, 5, RED);
        // a pane over 50..60 that reads 4 pixels around itself: 46..64
        $q->append(['glass'], PIXEL_QUEUE_PIECES, 0.0, [50, 50, 60, 60, 0, 0, 100, 80], false, [50, 50, 10, 10, 4]);
    };

    // 10 to 12: the change is far from what the pane reads
    $far = finished($draw(10));
    finished($draw(12), $far);
    // 10 to 44: the new rect's box, 43..50, reaches what the pane reads
    $near = finished($draw(10));
    finished($draw(44), $near);

    expect(changed($far))->toBe([[9, 43, 9, 7]])
        ->and(changed($near))->toBe([[9, 43, 7, 7], [43, 43, 21, 21]]);
});

it('lets one command that reads reach the next', function (): void {
    $draw = fn (int $x) => function (PixelQueue $q) use ($x): void {
        $q->clear(BLUE);
        $q->fillRect($x, 10, 2, 2, RED);
        // the second pane is only reached through the first: the first is found on the second pass
        $q->append(['glass'], PIXEL_QUEUE_PIECES, 0.0, [40, 10, 50, 20, 0, 0, 100, 80], false, [40, 10, 10, 10, 2]);
        $q->append(['glass'], PIXEL_QUEUE_PIECES, 0.0, [20, 10, 30, 20, 0, 0, 100, 80], false, [20, 10, 10, 10, 10]);
    };
    $queue = finished($draw(8));
    finished($draw(9), $queue);

    // the rect's change, 7..12, reaches what the second pane reads (10..40 by 0..30); joined, 7..40 reaches what the first reads (38..52)
    expect(changed($queue))->toBe([[7, 0, 45, 30]]);
});

it('refuses sizes, rects and pieces that are not what it asks for', function (callable $call): void {
    $queue = finished(fn (PixelQueue $q) => $q->fillRect(1, 1, 1, 1, RED));
    $queue->begin(100, 80, 'clip');

    expect(fn () => $call($queue))->toThrow(ValueError::class);
})->with([
    'a surface with no width' => [fn (PixelQueue $q) => $q->drawn(0, 80, 1, 1, 100, 80)],
    'a unit of 0' => [fn (PixelQueue $q) => $q->changed(100, 80, 1, 0, 100, 80)],
    'a limit past the limit' => [fn (PixelQueue $q) => $q->drawn(100, 80, 1, 1, 1 << 30, 80)],
    'three numbers of a rect' => [fn (PixelQueue $q) => $q->drawn(100, 80, 1, 1, 100, 80, [1, 2, 3])],
    'a rect of floats' => [fn (PixelQueue $q) => $q->changed(100, 80, 1, 1, 100, 80, [1.0, 2.0, 3.0, 4.0])],
    'a how that is not one' => [fn (PixelQueue $q) => $q->append(['x'], 99)],
    'a grow that is not finite' => [fn (PixelQueue $q) => $q->append(['path', []], PIXEL_QUEUE_CONTOURS, NAN)],
    'pieces not given' => [fn (PixelQueue $q) => $q->append(['x'], PIXEL_QUEUE_PIECES)],
    'seven numbers of a piece' => [fn (PixelQueue $q) => $q->append(['x'], PIXEL_QUEUE_PIECES, 0.0, [1, 2, 3, 4, 5, 6, 7])],
    'a piece with a number that is not finite' => [fn (PixelQueue $q) => $q->append(['x'], PIXEL_QUEUE_PIECES, 0.0, [1, 2, INF, 4, 0, 0, 1, 1])],
    'a piece whose clip is not whole' => [fn (PixelQueue $q) => $q->append(['x'], PIXEL_QUEUE_PIECES, 0.0, [1, 2, 3, 4, 0.5, 0, 1, 1])],
    'reads of four numbers' => [fn (PixelQueue $q) => $q->append(['x'], PIXEL_QUEUE_WHOLE, 0.0, null, false, [1, 2, 3, 4])],
    'a reach under 0' => [fn (PixelQueue $q) => $q->append(['x'], PIXEL_QUEUE_WHOLE, 0.0, null, false, [1, 2, 3, 4, -1])],
]);
