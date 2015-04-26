#!/bin/bash
echo -n "current runing: "
cfg_var_get "MODULE_NAME"
echo -n "path: "
cfg_var_get MODULE_PATH

module_begin test_foo && {
	echo '--variables---'
	cfg_var_list 
	echo '--------------'

	cfg_var_get null || echo 'specified variable not existing'
	cfg_var_get my_name
	cfg_var_get my_age
	
	cfg_var_set my*age 24 || echo 'variable name invalid'
	cfg_var_set my_age 24
	cfg_var_get my_age

	cfg_var_del my_name
	cfg_var_del my_name || echo 'delete item not found'
	
	cfg_var_del my_age 
	cfg_var_del MODULE_NAME 
	cfg_var_del MODULE_PATH 
	cfg_var_del MODULE_DIR
	cfg_var_del 12_times_2 
	cfg_var_del _2 
	cfg_var_list || echo 'no variable existing'
}
