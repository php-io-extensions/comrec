<?php

/** @generate-class-entries */

/**
 * The furthest a transformed coordinate, or a radius or a stroke once scaled, may land: 2^24.
 *
 * @var float
 * @cvalue PQ_LIMIT
 */
const PIXEL_QUEUE_LIMIT = UNKNOWN;

/**
 * A turned ellipse or rounded corner becomes a polygon that strays from it by less than this many pixels.
 *
 * @var float
 * @cvalue PQ_FLATNESS
 */
const PIXEL_QUEUE_FLATNESS = UNKNOWN;

/**
 * append(): the command writes the whole surface.
 *
 * @var int
 * @cvalue PQ_HOW_WHOLE
 */
const PIXEL_QUEUE_WHOLE = UNKNOWN;

/**
 * append(): the command's second entry is a list of contours, each a list of [x, y]; it writes
 * around them, $grow pixels out, inside the current clip.
 *
 * @var int
 * @cvalue PQ_HOW_CONTOURS
 */
const PIXEL_QUEUE_CONTOURS = UNKNOWN;

/**
 * append(): the command's second entry is a list of [x, y]; it writes around them, $grow pixels
 * out, inside the current clip.
 *
 * @var int
 * @cvalue PQ_HOW_POINTS
 */
const PIXEL_QUEUE_POINTS = UNKNOWN;

/**
 * append(): the command's second entry is a string of spans, 7 bytes each: y, x, length
 * (uint16 LE), coverage. It writes those pixels, inside the current clip.
 *
 * @var int
 * @cvalue PQ_HOW_SPANS
 */
const PIXEL_QUEUE_SPANS = UNKNOWN;

/**
 * append(): $pieces says where the command writes: one or two runs of eight numbers, x0, y0, x1,
 * y1 then the clip's x, y, width, height.
 *
 * @var int
 * @cvalue PQ_HOW_PIECES
 */
const PIXEL_QUEUE_PIECES = UNKNOWN;

/**
 * PixelQueueException's code: a shape was recorded with no frame open. $what is the method.
 *
 * @var int
 * @cvalue PQ_NOT_RECORDING
 */
const PIXEL_QUEUE_NOT_RECORDING = UNKNOWN;

/**
 * PixelQueueException's code: an argument is not a finite number. $what is its name.
 *
 * @var int
 * @cvalue PQ_NOT_FINITE
 */
const PIXEL_QUEUE_NOT_FINITE = UNKNOWN;

/**
 * PixelQueueException's code: a number is past PIXEL_QUEUE_LIMIT once transformed. $what is its
 * name, $value what it became.
 *
 * @var int
 * @cvalue PQ_PAST_LIMIT
 */
const PIXEL_QUEUE_PAST_LIMIT = UNKNOWN;

/**
 * PixelQueueException's code: radii are not a list of four numbers.
 *
 * @var int
 * @cvalue PQ_BAD_RADII
 */
const PIXEL_QUEUE_BAD_RADII = UNKNOWN;

/**
 * PixelQueueException's code: the shape needs an upright transform that scales x and y alike.
 * $what is the method.
 *
 * @var int
 * @cvalue PQ_UNEVEN
 */
const PIXEL_QUEUE_UNEVEN = UNKNOWN;

/**
 * PixelQueueException's code: concat() made a transform with a component that is not finite.
 * $value is the first such component.
 *
 * @var int
 * @cvalue PQ_BAD_MATRIX
 */
const PIXEL_QUEUE_BAD_MATRIX = UNKNOWN;
