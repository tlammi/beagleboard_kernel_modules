#!/bin/sh

ENV_SETUP_FILE="/opt/poky/2.6.1/environment-setup-armv7at2hf-neon-poky-linux-gnueabi"

print_bold(){
	echo "$(tput bold)$1 $(tput sgr0)"
	echo ""
}

if [[ "$0" == /* ]]; then
	CURDIR=$(dirname $0)
else
	CURDIR=$(dirname $(pwd)/$0)
fi

print_bold """##########################
Performing initializations
##########################"""

print_bold "Moving to ${CURDIR}"
cd ${CURDIR}


if [ "$#" -ne 0 ]; then

	MODULE_DIRS="${@:1}"
	print_bold "Got module directories \"${MODULE_DIRS}\" from command line"
else	
	print_bold "No modules specified from the command line, compiling all"
	MODULE_DIRS=$(ls -d */)
fi

print_bold "Parsed module dirs: ${MODULE_DIRS}" | tr "\n" " "

print_bold ""

print_bold "Sourcing environment from ${ENV_SETUP_FILE}"
source ${ENV_SETUP_FILE}

print_bold """#################
Building modules:
#################"""

FAIL=0

for MODULE in ${MODULE_DIRS}; do
	cd ${CURDIR}/${MODULE}
	make

	if [ $? -ne 0 ]; then
		print_bold  "WARNING: failed to build module ${MODULE}"
		FAIL=1
	fi
done


echo ""
if [ ${FAIL} -eq 1 ]; then
	print_bold \
		"""###########################
WARNING, some builds failed
###########################"""
else
	print_bold \
		"""####################
All builds succeeded
####################"""
fi
