#!/bin/bash
# Append product specific configs to default kernel config file and generate .product_config
# FIXME: Check if we can borrow something from merge_config.sh and handle config dependency etc.

usage() {
cat << EOF
Usage: bash `basename $0` PRODUCT_NAME DEFAULT_CONFIG
example:
    bash `basename $0` prod_a msm_defconfig
EOF
	exit 1
}

if [ "$#" != "2" ]; then
	usage
fi

PROD_NAME=$1
DEFAULT_CONFIG=$2

echo "Kernel: Product Config: \"${PROD_NAME}\""

DEFCONFIG_SRC=${srctree}/arch/${SRCARCH}/configs/
PRDCONFIG_SRC=${srctree}/arch/${SRCARCH}/configs/products/${PROD_NAME}

# We just put .product_config in same dir as .config
DEST_CONFIG_F=$PWD/.product_config
BASE_CONFIG_F=${DEFCONFIG_SRC}/${DEFAULT_CONFIG}
NORMAL_CONFIG_F=${PRDCONFIG_SRC}/normal_config
DEBUG_CONFIG_F=${PRDCONFIG_SRC}/debug_config

# Header
echo "# WARNING: This file was automatically generated." > ${DEST_CONFIG_F}
echo "# Do not modify it manually." >> ${DEST_CONFIG_F}
echo "# $(LC_ALL=C date)" >> ${DEST_CONFIG_F}
echo "" >> ${DEST_CONFIG_F}

# Default config file
cat ${BASE_CONFIG_F} >> ${DEST_CONFIG_F}
echo "" >> ${DEST_CONFIG_F}

if [ ! -d "${PRDCONFIG_SRC}" ]; then
    echo "Kernel: No product config found for ${PROD_NAME}: ${PRDCONFIG_SRC} does not exist."
    exit 0
fi

# Product config
echo "# ======== Product Configs Start ========" >> ${DEST_CONFIG_F}
echo CONFIG_PRODUCT_CFG=y >> ${DEST_CONFIG_F}
#echo CONFIG_PRODUCT_`echo ${PROD_NAME}|tr a-z A-Z`=y >> ${DEST_CONFIG_F}
echo "" >> ${DEST_CONFIG_F}

# Normal config
if [ -f ${NORMAL_CONFIG_F} ]; then
    echo "# --- Normal Configs ---" >> ${DEST_CONFIG_F}
	cat ${NORMAL_CONFIG_F} >> ${DEST_CONFIG_F}
	echo "" >> ${DEST_CONFIG_F}
fi

# Debug config
if [ "${TARGET_BUILD_VARIANT}" == "eng" -a -f ${DEBUG_CONFIG_F} ]; then
    echo "# --- Debug Configs ---" >> ${DEST_CONFIG_F}
	cat ${DEBUG_CONFIG_F} >> ${DEST_CONFIG_F}
    echo "" >> ${DEST_CONFIG_F}
fi

# End
echo "# ======== Product Configs End ========" >> ${DEST_CONFIG_F}
