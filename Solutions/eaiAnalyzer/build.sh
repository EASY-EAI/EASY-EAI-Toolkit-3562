#!/bin/sh

set -e

SHELL_FOLDER=$(cd "$(dirname "$0")";pwd)
cd $SHELL_FOLDER

CUR_DIR_NAME=`basename "$SHELL_FOLDER"`
APP_PATH="app-${CUR_DIR_NAME}/userdata/apps/app-${CUR_DIR_NAME}"

main ()
{
	if [ "$1" = "clear" ]; then
		rm -rf build
		rm -rf Release
		rm -rf package/app-${CUR_DIR_NAME}/userdata
		exit 0
	fi


	## 编译整个工程
	if [ -d build ]; then
		cd build
	else
		mkdir build && cd build
		cmake ..
	fi
	make -j$(nproc)


	## 创建输出目录
	mkdir -p "../Release" && cp $CUR_DIR_NAME "../Release"
	cd - > /dev/null
	cp -rf config/* Release

	
	## 制作可安装deb包
	if [ "$1" = "release" ]; then
		if [ -d package/$APP_PATH ]; then
			echo "app path is ready!"
		else
			echo "make app path .."
			mkdir -p package/$APP_PATH
		fi

		cp Release/* -r package/$APP_PATH

		if [ -d package/app-${CUR_DIR_NAME} ]; then
			cd package
			dpkg -b app-${CUR_DIR_NAME} app-${CUR_DIR_NAME}.deb
			cd - > /dev/null
		fi
		chmod 755 -R package
		tree package
	fi
}

main "$@"
