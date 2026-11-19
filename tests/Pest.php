<?php

declare(strict_types=1);

if (! extension_loaded('comrec')) {
    throw new RuntimeException('The comrec extension is not loaded; run pest with -d extension=/path/to/comrec.so');
}

const RED = 0xFF0000FF;
const BLUE = 0x0000FFFF;

/** A queue with a frame open on a 100 by 80 surface, its clip carried as 'clip'. */
function opened(mixed $nonZero = 'nz', mixed $evenOdd = 'eo'): PixelQueue
{
    $queue = new PixelQueue($nonZero, $evenOdd);
    $queue->begin(100, 80, 'clip');

    return $queue;
}

/** The commands of one frame drawn by $draw. */
function frame(callable $draw): array
{
    $queue = opened();
    $draw($queue);

    return $queue->finish();
}

/** A queue whose last frame is the one $draw drew. */
function finished(callable $draw, ?PixelQueue $queue = null): PixelQueue
{
    $queue ??= new PixelQueue('nz', 'eo');
    $queue->begin(100, 80, 'clip');
    $draw($queue);
    $queue->finish();

    return $queue;
}

/** Rects as [x, y, width, height] each. */
function rects(array $flat): array
{
    return array_chunk($flat, 4);
}

/** Where the last frame writes, on the 100 by 80 surface, pixel by pixel. */
function drawn(PixelQueue $queue, array $also = []): array
{
    return rects($queue->drawn(100, 80, 1, 1, 100, 80, $also));
}

function changed(PixelQueue $queue, array $also = []): array
{
    return rects($queue->changed(100, 80, 1, 1, 100, 80, $also));
}

/** What a shape was refused with: [code, what, value], or null when it was not refused. */
function refusal(callable $call): ?array
{
    try {
        $call();
    } catch (PixelQueueException $e) {
        return [$e->getCode(), $e->what, $e->value];
    }

    return null;
}
