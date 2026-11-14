<?php

/** @generate-class-entries */

/**
 * A shape a PixelQueue refused. getCode() is one of the PIXEL_QUEUE_NOT_RECORDING ..
 * PIXEL_QUEUE_BAD_MATRIX constants.
 */
class PixelQueueException extends Exception
{
    /** The argument, or the method, the refusal is about; '' when it is about neither. */
    public string $what = '';

    /** The number that was refused, when the refusal is about one; 0.0 otherwise. */
    public float $value = 0.0;
}
