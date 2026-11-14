<?php

declare(strict_types=1);

it('records between begin() and finish()', function (): void {
    $queue = new PixelQueue();

    expect($queue->recording())->toBeFalse();
    $queue->begin(10, 10, 'clip');
    expect($queue->recording())->toBeTrue()
        ->and($queue->finish())->toBe([])
        ->and($queue->recording())->toBeFalse();
});

it('puts a point through the transform: the matrix given last applies first', function (): void {
    $queue = opened();
    $queue->concat(1, 0, 0, 1, 10, 5);
    $queue->concat(2, 0, 0, 2, 0, 0);
    $queue->fillRect(3, 4, 1, 1, RED);

    expect($queue->matrix())->toBe([2.0, 0.0, 0.0, 2.0, 10.0, 5.0])
        ->and($queue->finish())->toBe([['path', [[[16.0, 13.0], [18.0, 13.0], [18.0, 15.0], [16.0, 15.0]]], 'nz', RED, 'clip']]);
});

it('brings the transform and the clip back with pop()', function (): void {
    $queue = opened();
    $queue->concat(1, 0, 0, 1, 7, 7);
    $queue->push();
    $queue->concat(3, 0, 0, 3, 0, 0);
    $queue->clip('inner', 1, 2, 3, 4);

    expect($queue->depth())->toBe(1)->and($queue->clipValue())->toBe('inner');
    $queue->pop();
    expect($queue->depth())->toBe(0)
        ->and($queue->matrix())->toBe([1.0, 0.0, 0.0, 1.0, 7.0, 7.0])
        ->and($queue->clipValue())->toBe('clip');
});

it('hands the clip and the fill rule back as they were given, objects included', function (): void {
    $nonZero = new stdClass();
    $evenOdd = new stdClass();
    $clip = new ArrayObject();
    $queue = new PixelQueue($nonZero, $evenOdd);
    $queue->begin(100, 80, 'whole');
    $queue->clip($clip, 0, 0, 50, 50);
    $queue->fillRect(1, 1, 2, 2, RED);
    $queue->concat(1, 0.5, 0, 1, 0, 0);
    $queue->roundRect(10, 10, 20, 20, [2, 2, 2, 2], RED, BLUE, 2.0);
    $commands = $queue->finish();

    expect($commands[0][2])->toBe($nonZero)
        ->and($commands[0][4])->toBe($clip)
        ->and($commands[1][2])->toBe($nonZero)
        ->and($commands[2][2])->toBe($evenOdd)
        ->and(count($commands[2][1]))->toBe(2);
});

it('leaves the transform as it was when the product is not finite', function (): void {
    $queue = opened();
    $queue->concat(1.0e200, 0, 0, 1, 0, 0);

    expect(refusal(fn () => $queue->concat(1.0e200, 0, 0, 1, 0, 0)))->toBe([PIXEL_QUEUE_BAD_MATRIX, '', INF])
        ->and($queue->matrix())->toBe([1.0e200, 0.0, 0.0, 1.0, 0.0, 0.0]);
});

it('drops a frame it is told to abandon, and keeps the last one finished', function (): void {
    $queue = finished(fn (PixelQueue $q) => $q->fillRect(10, 10, 5, 5, RED));
    $queue->begin(100, 80, 'clip');
    $queue->push();
    $queue->enter('layer', 0, 0, 5, 5);
    $queue->fillRect(60, 60, 5, 5, RED);
    $queue->abandon();

    expect($queue->recording())->toBeFalse()
        ->and(drawn($queue))->toBe([[9, 9, 7, 7]]);
});

it('records a layer in a list of its own, and brings the clip and the pushes back after it', function (): void {
    $queue = opened();
    $queue->push();
    $queue->fillRect(0, 0, 1, 1, RED);
    $queue->enter('layer', 10, 10, 20, 20);

    expect($queue->depth())->toBe(0)->and($queue->clipValue())->toBe('layer');
    $queue->push();
    $queue->concat(1, 0, 0, 1, 5, 5);
    $queue->fillRect(0, 0, 1, 1, BLUE);
    $children = $queue->leave();

    expect($children)->toBe([['path', [[[5.0, 5.0], [6.0, 5.0], [6.0, 6.0], [5.0, 6.0]]], 'nz', BLUE, 'layer']])
        ->and($queue->depth())->toBe(1)
        ->and($queue->clipValue())->toBe('clip')
        ->and($queue->matrix())->toBe([1.0, 0.0, 0.0, 1.0, 5.0, 5.0])
        ->and(count($queue->finish()))->toBe(1);
});

it('refuses a call that has no frame, no push or no layer to act on', function (callable $call, string $error): void {
    expect(fn () => $call(new PixelQueue()))->toThrow($error);
})->with([
    'finish() with no frame' => [fn (PixelQueue $q) => $q->finish(), Error::class],
    'push() with no frame' => [fn (PixelQueue $q) => $q->push(), Error::class],
    'concat() with no frame' => [fn (PixelQueue $q) => $q->concat(1, 0, 0, 1, 0, 0), Error::class],
    'matrix() with no frame' => [fn (PixelQueue $q) => $q->matrix(), Error::class],
    'clip() with no frame' => [fn (PixelQueue $q) => $q->clip('c', 0, 0, 1, 1), Error::class],
    'append() with no frame' => [fn (PixelQueue $q) => $q->append(['clear', 0], PIXEL_QUEUE_WHOLE), Error::class],
    'begin() twice' => [fn (PixelQueue $q) => [$q->begin(1, 1, 'c'), $q->begin(1, 1, 'c')], Error::class],
    'pop() with nothing pushed' => [fn (PixelQueue $q) => [$q->begin(1, 1, 'c'), $q->pop()], Error::class],
    'pop() past the layer it is in' => [fn (PixelQueue $q) => [$q->begin(9, 9, 'c'), $q->push(), $q->enter('l', 0, 0, 1, 1), $q->pop()], Error::class],
    'leave() with no layer' => [fn (PixelQueue $q) => [$q->begin(1, 1, 'c'), $q->leave()], Error::class],
    'a surface with no pixel' => [fn (PixelQueue $q) => $q->begin(0, 5, 'c'), ValueError::class],
    'a clip past the limit' => [fn (PixelQueue $q) => [$q->begin(1, 1, 'c'), $q->clip('c', 0, 0, 1 << 30, 1)], ValueError::class],
]);

it('lets go of a queue that a value it holds points back to', function (): void {
    $queue = new PixelQueue();
    $clip = new stdClass();
    $clip->queue = $queue;
    $queue->begin(4, 4, $clip);
    $queue->fillRect(0, 0, 1, 1, RED);
    $seen = WeakReference::create($queue);
    unset($queue, $clip);
    gc_collect_cycles();

    expect($seen->get())->toBeNull();
});
