/*
 * comrec: a frame of drawing commands in C. PixelQueue records shapes
 * under a transform and a clip, keeps the last frame, and answers which pixels
 * changed between the two.
 */

#include "runtime.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "../stubs/comrec_arginfo.h"
#include "../stubs/PixelQueueException_arginfo.h"

zend_class_entry *phppq_ce_PixelQueueException;

void phppq_register_PixelQueueException(void)
{
	phppq_ce_PixelQueueException = register_class_PixelQueueException(zend_ce_exception);
}

PHP_MINIT_FUNCTION(comrec)
{
	(void) type;

	register_comrec_symbols(module_number);

	phppq_register_PixelQueueException();
	phppq_register_PixelQueue();

	return SUCCESS;
}

PHP_RINIT_FUNCTION(comrec)
{
	(void) type;
	(void) module_number;
#if defined(COMPILE_DL_COMREC) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}

PHP_MINFO_FUNCTION(comrec)
{
	(void) zend_module;

	php_info_print_table_start();
	php_info_print_table_row(2, "comrec support", "enabled");
	php_info_print_table_row(2, "Version", PHP_COMREC_VERSION);
	php_info_print_table_end();
}

zend_module_entry comrec_module_entry = {
	STANDARD_MODULE_HEADER,
	"comrec",
	NULL,
	PHP_MINIT(comrec),
	NULL,
	PHP_RINIT(comrec),
	NULL,
	PHP_MINFO(comrec),
	PHP_COMREC_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_COMREC
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(comrec)
#endif
