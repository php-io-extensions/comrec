#ifndef PHP_PIXEL_QUEUE_H
#define PHP_PIXEL_QUEUE_H

extern zend_module_entry pixel_queue_module_entry;
#define phpext_pixel_queue_ptr &pixel_queue_module_entry

#define PHP_PIXEL_QUEUE_VERSION "0.11.0"

#if defined(ZTS) && defined(COMPILE_DL_PIXEL_QUEUE)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif
