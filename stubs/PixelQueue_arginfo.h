/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 5af8b2a7f2936c465eb819ca2be886406b303462 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_PixelQueue___construct, 0, 0, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, nonZero, IS_MIXED, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, evenOdd, IS_MIXED, 0, "1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_begin, 0, 3, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, clip, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_recording, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_abandon, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_finish, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_PixelQueue_push arginfo_class_PixelQueue_abandon

#define arginfo_class_PixelQueue_pop arginfo_class_PixelQueue_abandon

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_depth, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_concat, 0, 6, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, a, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, b, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, c, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, d, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, e, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, f, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_PixelQueue_matrix arginfo_class_PixelQueue_finish

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_clip, 0, 5, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, clip, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_clipValue, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_append, 0, 2, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, command, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, how, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, grow, IS_DOUBLE, 0, "0.0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, pieces, IS_ARRAY, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, restless, _IS_BOOL, 0, "false")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, reads, IS_ARRAY, 1, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_PixelQueue_enter arginfo_class_PixelQueue_clip

#define arginfo_class_PixelQueue_leave arginfo_class_PixelQueue_finish

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_clear, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, rgba, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_fillRect, 0, 5, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, rgba, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_strokeRect, 0, 6, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, rgba, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, stroke, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_line, 0, 6, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, x0, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, y0, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, x1, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, y1, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, rgba, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, stroke, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_fillTriangle, 0, 7, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, x0, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, y0, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, x1, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, y1, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, x2, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, y2, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, rgba, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_fillEllipse, 0, 5, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, cx, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, cy, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, rx, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, ry, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, rgba, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_strokeEllipse, 0, 6, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, cx, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, cy, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, rx, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, ry, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, rgba, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, stroke, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_roundRect, 0, 8, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, radii, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, fill, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, border, IS_LONG, 1)
	ZEND_ARG_TYPE_INFO(0, borderWidth, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_shadow, 0, 9, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, radius, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, blur, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, rgba, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, dx, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, dy, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_glyphs, 0, 5, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, runs, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, ink, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, rgba, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_PixelQueue_drawn, 0, 6, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, surfaceWidth, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, surfaceHeight, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, unitWidth, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, unitHeight, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, limitWidth, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, limitHeight, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, also, IS_ARRAY, 0, "[]")
ZEND_END_ARG_INFO()

#define arginfo_class_PixelQueue_changed arginfo_class_PixelQueue_drawn

ZEND_METHOD(PixelQueue, __construct);
ZEND_METHOD(PixelQueue, begin);
ZEND_METHOD(PixelQueue, recording);
ZEND_METHOD(PixelQueue, abandon);
ZEND_METHOD(PixelQueue, finish);
ZEND_METHOD(PixelQueue, push);
ZEND_METHOD(PixelQueue, pop);
ZEND_METHOD(PixelQueue, depth);
ZEND_METHOD(PixelQueue, concat);
ZEND_METHOD(PixelQueue, matrix);
ZEND_METHOD(PixelQueue, clip);
ZEND_METHOD(PixelQueue, clipValue);
ZEND_METHOD(PixelQueue, append);
ZEND_METHOD(PixelQueue, enter);
ZEND_METHOD(PixelQueue, leave);
ZEND_METHOD(PixelQueue, clear);
ZEND_METHOD(PixelQueue, fillRect);
ZEND_METHOD(PixelQueue, strokeRect);
ZEND_METHOD(PixelQueue, line);
ZEND_METHOD(PixelQueue, fillTriangle);
ZEND_METHOD(PixelQueue, fillEllipse);
ZEND_METHOD(PixelQueue, strokeEllipse);
ZEND_METHOD(PixelQueue, roundRect);
ZEND_METHOD(PixelQueue, shadow);
ZEND_METHOD(PixelQueue, glyphs);
ZEND_METHOD(PixelQueue, drawn);
ZEND_METHOD(PixelQueue, changed);

static const zend_function_entry class_PixelQueue_methods[] = {
	ZEND_ME(PixelQueue, __construct, arginfo_class_PixelQueue___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, begin, arginfo_class_PixelQueue_begin, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, recording, arginfo_class_PixelQueue_recording, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, abandon, arginfo_class_PixelQueue_abandon, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, finish, arginfo_class_PixelQueue_finish, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, push, arginfo_class_PixelQueue_push, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, pop, arginfo_class_PixelQueue_pop, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, depth, arginfo_class_PixelQueue_depth, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, concat, arginfo_class_PixelQueue_concat, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, matrix, arginfo_class_PixelQueue_matrix, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, clip, arginfo_class_PixelQueue_clip, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, clipValue, arginfo_class_PixelQueue_clipValue, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, append, arginfo_class_PixelQueue_append, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, enter, arginfo_class_PixelQueue_enter, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, leave, arginfo_class_PixelQueue_leave, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, clear, arginfo_class_PixelQueue_clear, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, fillRect, arginfo_class_PixelQueue_fillRect, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, strokeRect, arginfo_class_PixelQueue_strokeRect, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, line, arginfo_class_PixelQueue_line, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, fillTriangle, arginfo_class_PixelQueue_fillTriangle, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, fillEllipse, arginfo_class_PixelQueue_fillEllipse, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, strokeEllipse, arginfo_class_PixelQueue_strokeEllipse, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, roundRect, arginfo_class_PixelQueue_roundRect, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, shadow, arginfo_class_PixelQueue_shadow, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, glyphs, arginfo_class_PixelQueue_glyphs, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, drawn, arginfo_class_PixelQueue_drawn, ZEND_ACC_PUBLIC)
	ZEND_ME(PixelQueue, changed, arginfo_class_PixelQueue_changed, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_PixelQueue(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "PixelQueue", class_PixelQueue_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES|ZEND_ACC_NOT_SERIALIZABLE);

	return class_entry;
}
