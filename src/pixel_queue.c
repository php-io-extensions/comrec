/*
 * pixel_queue: a frame of drawing commands in C. PixelQueue records shapes
 * under a transform and a clip, keeps the last frame, and answers which pixels
 * changed between the two.
 */

#include "runtime.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "../stubs/pixel_queue_arginfo.h"
#include "../stubs/PixelQueueException_arginfo.h"

zend_class_entry *phppq_ce_PixelQueueException;

void phppq_register_PixelQueueException(void)
{
	phppq_ce_PixelQueueException = register_class_PixelQueueException(zend_ce_exception);
}

PHP_MINIT_FUNCTION(pixel_queue)
{
	(void) type;

	register_pixel_queue_symbols(module_number);

	phppq_register_PixelQueueException();
	phppq_register_PixelQueue();

	return SUCCESS;
}

PHP_RINIT_FUNCTION(pixel_queue)
{
	(void) type;
	(void) module_number;
#if defined(COMPILE_DL_PIXEL_QUEUE) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}

PHP_MINFO_FUNCTION(pixel_queue)
{
	(void) zend_module;

	php_info_print_table_start();
	php_info_print_table_row(2, "pixel_queue support", "enabled");
	php_info_print_table_row(2, "Version", PHP_PIXEL_QUEUE_VERSION);
	php_info_print_table_end();
}

zend_module_entry pixel_queue_module_entry = {
	STANDARD_MODULE_HEADER,
	"pixel_queue",
	NULL,
	PHP_MINIT(pixel_queue),
	NULL,
	PHP_RINIT(pixel_queue),
	NULL,
	PHP_MINFO(pixel_queue),
	PHP_PIXEL_QUEUE_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PIXEL_QUEUE
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(pixel_queue)
#endif
