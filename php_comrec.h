#ifndef PHP_COMREC_H
#define PHP_COMREC_H

extern zend_module_entry comrec_module_entry;
#define phpext_comrec_ptr &comrec_module_entry

#define PHP_COMREC_VERSION "0.11.0"

#if defined(ZTS) && defined(COMPILE_DL_COMREC)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif
