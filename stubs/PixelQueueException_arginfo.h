/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 4d7790b40c4a4ac55649dc273c85014156765493 */

static zend_class_entry *register_class_PixelQueueException(zend_class_entry *class_entry_Exception)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "PixelQueueException", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_Exception, 0);

	zval property_what_default_value;
	ZVAL_EMPTY_STRING(&property_what_default_value);
	zend_string *property_what_name = zend_string_init("what", sizeof("what") - 1, 1);
	zend_declare_typed_property(class_entry, property_what_name, &property_what_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release(property_what_name);

	zval property_value_default_value;
	ZVAL_DOUBLE(&property_value_default_value, 0.0);
	zend_string *property_value_name = zend_string_init("value", sizeof("value") - 1, 1);
	zend_declare_typed_property(class_entry, property_value_name, &property_value_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_DOUBLE));
	zend_string_release(property_value_name);

	return class_entry;
}
