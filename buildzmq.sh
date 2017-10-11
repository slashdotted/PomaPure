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
SYSTEM_DEPS="autoconf automake libtool"
sudo mkdir -p /usr/local/share/doc
sudo mkdir -p /usr/local/lib

if ! ldconfig -v 2>/dev/null | grep "/usr/local/lib" > /dev/null; then
	sudo echo  "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/usrlocallib.conf
	sudo ldconfig
fi

echo "Installing system dependencies"
sudo apt-get install --assume-yes ${SYSTEM_DEPS} || die "Failed to install system dependencies"

if ! check_package poma-libzmq; then
	echo "Building and installing libzmq"
	wget https://github.com/zeromq/libzmq/releases/download/v4.2.2/zeromq-4.2.2.tar.gz || die "Failed to download zeromq 4.2.2"
	tar xvfz zeromq-4.2.2.tar.gz || die "Failed to uncompress zeromq 4.2.2"
	cd zeromq-4.2.2/ 
	./autogen.sh || die "Failed to autogen"
	./configure || die "Failed to configure"
	make || die "Failed to make"
	sudo checkinstall -y --pkgname poma-libzmq --pkgversion 1 --pkgrelease 1
	sudo ldconfig
fi
