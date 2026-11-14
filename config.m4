PHP_ARG_ENABLE([pixel-queue],
  [whether to enable pixel_queue support],
  [AS_HELP_STRING([--enable-pixel-queue], [Enable the pixel_queue drawing-command queue extension])],
  [no])

if test "$PHP_PIXEL_QUEUE" != "no"; then
  dnl -ffp-contract=off: no fused multiply-add, so every platform rounds each step the same way.
  dnl -fno-builtin-sin -fno-builtin-cos: the compiler otherwise joins a sin and a cos of one angle
  dnl into one sincos call, whose last bit is not always the one sin() and cos() give alone.
  PHP_NEW_EXTENSION([pixel_queue],
    [src/pixel_queue.c src/boxes.c src/PixelQueue.c],
    [$ext_shared],, [-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -ffp-contract=off -fno-builtin-sin -fno-builtin-cos])
  PHP_ADD_BUILD_DIR([$ext_builddir/src])
fi
