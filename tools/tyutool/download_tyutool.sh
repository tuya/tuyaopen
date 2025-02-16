#!/usr/bin/env bash
##
# @file download_tyutool.sh
# @brief
# @author Tuya
# @version 1.0.0
# @date 2024-12-26
#/

HOST="https://images.tuyacn.com/smart/embed/package/vscode/data/ide_serial"
TYUTOOL="tyutool_cli"
INSTALL_DIR=$1

if [ x"$INSTALL_DIR" = x"" ]; then
    echo "Error: Need install path."
    exit 1
fi

mkdir -p $INSTALL_DIR
cd $INSTALL_DIR

if [ -x $TYUTOOL ]; then
    exit 0
fi

SYSTEM=$(uname -s)
MACHINE=$(uname -m)

if [ $SYSTEM = "Linux" ]; then
    PACKAGE="tyutool_cli.tar.gz"
elif [ $SYSTEM = "Darwin" ]; then
    PACKAGE="darwin_arm64_tyutool_cli.tar.gz"
    if [[ $MACHINE = "x86_64" ]]; then
        PACKAGE="darwin_x86_tyutool_cli.tar.gz"
    fi
else
    PACKAGE="win_tyutool_cli.tar.gz"
fi

DOWNLOAD_URL=${HOST}/${PACKAGE}

wget -c $DOWNLOAD_URL
tar -xzf $PACKAGE

if [ ! -x $TYUTOOL ]; then
    exit 1
fi

exit 0
