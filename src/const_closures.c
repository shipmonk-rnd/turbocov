#include "turbocov_internal.h"

/*
 * Since PHP 8.5 a constant expression can contain a closure: a parameter default, an attribute argument,
 * the value of a constant, the default of a property. The op array of such a closure is a node of the constant AST.
 * It is not in the dynamic function definitions of an op array, and no table of the engine lists it.
 */
#if PHP_VERSION_ID >= 80500

#include "zend_ast.h"
#include "zend_attributes.h"

static void tc_process_ast(zend_ast *ast)
{
	if (!ast) {
		return;
	}

	if (ast->kind == ZEND_AST_OP_ARRAY) {
		tc_process_op_array(zend_ast_get_op_array(ast)->op_array);

		return;
	}

	if (zend_ast_is_special(ast)) {
		return;
	}

#if PHP_VERSION_ID >= 80600
	/* The arguments of a partial function application are in a member of the node, not in its children. */
	if (ast->kind == ZEND_AST_CALLABLE_CONVERT) {
		tc_process_ast(((zend_ast_fcc *) ast)->args);

		return;
	}
#endif

	if (zend_ast_is_list(ast)) {
		zend_ast_list *list = zend_ast_get_list(ast);

		for (uint32_t i = 0; i < list->children; i++) {
			tc_process_ast(list->child[i]);
		}

		return;
	}

	for (uint32_t i = 0; i < zend_ast_get_num_children(ast); i++) {
		tc_process_ast(ast->child[i]);
	}
}

static void tc_process_value(zval *value)
{
	if (Z_TYPE_P(value) == IS_CONSTANT_AST) {
		tc_process_ast(Z_ASTVAL_P(value));
	}
}

static void tc_process_attributes(HashTable *attributes)
{
	zend_attribute *attribute;

	if (!attributes) {
		return;
	}

	ZEND_HASH_FOREACH_PTR(attributes, attribute) {
		for (uint32_t i = 0; i < attribute->argc; i++) {
			tc_process_value(&attribute->args[i].value);
		}
	} ZEND_HASH_FOREACH_END();
}

void tc_process_op_array_closures(zend_op_array *op_array)
{
	for (uint32_t i = 0; i < (uint32_t) op_array->last_literal; i++) {
		tc_process_value(&op_array->literals[i]);
	}

	/* The attributes of the function and of its parameters. */
	tc_process_attributes(op_array->attributes);

	/*
	 * The attributes of a global constant are a pointer in a literal of the opcode that declares the constant.
	 * ZEND_ACC_PTR_OPS has the value of ZEND_ACC_OVERRIDE, and only the code outside of all functions can have it.
	 */
	if (!op_array->function_name && (op_array->fn_flags & ZEND_ACC_PTR_OPS)) {
		for (uint32_t i = 0; i + 1 < op_array->last; i++) {
			zend_op *opline = &op_array->opcodes[i];

			if (opline->opcode == ZEND_DECLARE_ATTRIBUTED_CONST && (opline + 1)->op1_type == IS_CONST) {
				tc_process_attributes(Z_PTR_P(RT_CONSTANT(opline + 1, (opline + 1)->op1)));
			}
		}
	}
}

void tc_process_class_closures(zend_class_entry *ce)
{
	zend_class_constant *constant;
	zend_property_info *property;

	tc_process_attributes(ce->attributes);

	ZEND_HASH_MAP_FOREACH_PTR(&ce->constants_table, constant) {
		tc_process_value(&constant->value);
		tc_process_attributes(constant->attributes);
	} ZEND_HASH_FOREACH_END();

	ZEND_HASH_MAP_FOREACH_PTR(&ce->properties_info, property) {
		tc_process_attributes(property->attributes);
	} ZEND_HASH_FOREACH_END();

	/* The type of the two counts is int before PHP 8.6 and uint32_t since PHP 8.6. */
	for (uint32_t i = 0; i < (uint32_t) ce->default_properties_count; i++) {
		tc_process_value(&ce->default_properties_table[i]);
	}

	for (uint32_t i = 0; i < (uint32_t) ce->default_static_members_count; i++) {
		tc_process_value(&ce->default_static_members_table[i]);
	}
}

#endif
