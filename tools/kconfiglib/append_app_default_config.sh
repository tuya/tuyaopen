#!/usr/bin/env bash
##
# @file kconfiglib/append_app_default_config.sh
# @brief 
# @author Tuya
# @version 1.0.0
# @date 2025-01-08
#/

# $1: APP_DEFAULT_CONFIG
# $2: DOT_CONFIG_DIR

if [ -f $1 ]; then
    cat $1 >> $2
fi
