#!/bin/bash
function check_package() {
	if dpkg -s "$1" >/dev/null 2>&1; then
		return 0
	else
		return 1
	fi
}

function die() {
	echo "Aborting: $1" 1>&2
	exit 1
}

mkdir .dependencies
cd .dependencies
DEP_ROOT=$(pwd)
DEP_REPOSITORY_ROOT=../Dependencies
SYSTEM_DEPS="uild-essential checkinstall fakeroot git pkg-config unzip libcurl4-openssl-dev libz-dev wget"
sudo mkdir -p /usr/local/share/doc
sudo mkdir -p /usr/local/lib

if ! ldconfig -v 2>/dev/null | grep "/usr/local/lib" > /dev/null; then
	sudo echo  "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/usrlocallib.conf
	sudo ldconfig
fi

echo "Installing system dependencies"
sudo apt-get install --assume-yes ${SYSTEM_DEPS} || die "Failed to install system dependencies"

if ! check_package poma-cmake; then
	echo "Building and installing cmake from git"
	git clone https://github.com/Kitware/CMake.git
	./bootstrap --system-curl --prefix=/usr/local || die "Cannot boostrap cmake"
	make || die "Cannot make cmake"
	sudo checkinstall -y --pkgname poma-cmake --pkgversion=3.9 --pkgrelease 1
	export PATH=/usr/local/bin:$PATH
fi
