#!/bin/bash

# APIs

# Returns 0 iff the module has not been loaded yet.
function module_begin () {
	local mod_name="${1}"
	local i=0
	# iterate through the module list in which 
	# stores modules not loaded yet.
	for ((; i < ${#mod_load_list[@]}; i++))
	do
		if [ "$mod_name" = "${mod_load_list[$i]}" ]
		then
			return 1
		fi
	done
	return 0
}

# Gets the value of a configuration variable.
function cfg_var_get () {
	local var_name="$1"
	# get the value of a given variable name
	local value=`eval "expr \"${var}\""`
	local i=0
	for ((; i < ${#var_list[@]}; i++))
	do
		# return the value from variable that matches the name.
		if [ "$var_name" = ${var_list[$i]} ] 
		then
			# get the value from our variable-value map array.
			local map_var=\$val_map_${var_list[$i]}
			local value=`eval "expr \"${map_var}\""`
			echo "${value}"
			return 0
		fi
	done
	return 1
}

# Deletes a configuration variable.
function cfg_var_del () {
	local var_name="$1"
	# return 1 if no such variable name exists.
	cfg_var_get "$var_name" > /dev/null || return 1
	# remove the variable from variable list.
	var_list=(${var_list[@]/$var_name})
	return 0
}

# Sets the value of a configuration variable.
function cfg_var_set () {
	local var_name="$1"
	local value="$2"
	# check the name validity 
	echo "$var_name" | grep '[^a-zA-Z_0-9]' > /dev/null
	local n_valid=${PIPESTATUS[1]}
	# return 1 if not a valid name
	if [ $n_valid -eq 0 ]
	then
		return 1
	else
		# also check if there already has a same name variable,
		# if so, replace it.
		cfg_var_get "${var_name}" > /dev/null \
		                            && cfg_var_del "${var_name}"
		var_list+=("${var_name}")
		declare -g "val_map_${var_name}"="${value}"
	fi
	return 0
}

# Lists all defined configuration variables.
function cfg_var_list() {
	local i=0
	# first see if the variable list is empty
	if [ ${#var_list[@]} -eq 0 ] 
	then
		return 1
	fi
	# iterate every variable in list.
	for ((; i < ${#var_list[@]}; i++))
	do
		# get the variable name / value pair and print
		local map_var=\$val_map_${var_list[$i]}
		local value=`eval "expr \"${map_var}\""`
		echo "${var_list[$i]}: ${value}"
	done
	return 0
}

# main script

# Validate the command-line
if [ $# -gt 1 ]
then
	echo 'bad number of arguments.'
	exit
elif [ $# -eq 0 ] 
then
	echo "usage: ${0} <config_file>"
	exit
else
	conf_name="${1}"
fi

# get the absolute script path
script_dir=$(cd "$(dirname "${0}")" && pwd)

function cfg_line_process() {
	local l="$1"
	# check if it is a "variable line"
	echo "$l" | grep ':' > /dev/null
	local n_var=${PIPESTATUS[1]}
	# check if it is a "module line"
	echo "$l" | grep 'module' > /dev/null
	local n_mod=${PIPESTATUS[1]}

	if [ $n_var -eq 0 ]
	then 
		# this is a "variable line", take out the variable name
		# and its value using awk and strip spaces not significant.
		local var_name=$(echo "$l" | awk -F ":" '{print $1}' | xargs)
		local value=$(echo "$l" | awk -F ":" '{print $2}' | 
		              sed 's/^[ ]*//')
		# use our API to set the variable
		cfg_var_set "${var_name}" "${value}"
	elif [ $n_mod -eq 0 ]
	then
		# this is a "module line", get the module name using look-
		# around regular expression.
		local mod_name=$(echo "$l" | grep -oP "(?<=^module ).+")
		# add the new module into buffer list.
		mod_tmp_list+=("${mod_name}")
	fi
}

function call_modules () {
	local i=0
	# for each of the modules not loaded
	for ((; i < ${#mod_tmp_list[@]}; i++))
	do
		# set the new MODULE_NAME and MODULE_PATH variable
		local mpath="${script_dir}/modules/${mod_tmp_list[$i]}.mod.sh"
		cfg_var_set MODULE_NAME "${mod_tmp_list[$i]}"
		cfg_var_set MODULE_PATH "$mpath"
		# source the module file and add it to module list
		source "$mpath"
		mod_load_list+=("${mod_tmp_list[$i]}")
	done
}

# set MODULE_DIR variable
cfg_var_set MODULE_DIR "${script_dir}/modules"

# Load in and validate the contents of the configuration file.
while read cur_cfg_line
do
	# filter the comment lines
	echo "$cur_cfg_line" | grep '^#' > /dev/null
	if [ ${PIPESTATUS[1]} -eq 1 ]
	then
		# read configuration file line by line
		cfg_line_process "$cur_cfg_line"
	fi
done < ${script_dir}/${conf_name}

# run modules after all variables been read.
call_modules
