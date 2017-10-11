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
SYSTEM_DEPS="build-essential checkinstall fakeroot git pkg-config unzip"
sudo mkdir -p /usr/local/share/doc
sudo mkdir -p /usr/local/lib

if ! ldconfig -v 2>/dev/null | grep "/usr/local/lib" > /dev/null; then
	sudo echo  "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/usrlocallib.conf
	sudo ldconfig
fi

echo "Installing system dependencies"
sudo apt-get install --assume-yes ${SYSTEM_DEPS} || die "Failed to install system dependencies"

if ! check_package poma-boost; then
	echo "Building and installing boost"
	wget https://sourceforge.net/projects/boost/files/boost/1.65.0/boost_1_65_0.tar.bz2 || die "Failed to download boost 1.65"
	tar xvjpf boost_1_65_0.tar.bz2 || die "Failed to uncompress boost 1.65"
	cd boost_1_65_0 
	#patch -p1 < ../Dependencies/boost_json_number_wo_quotes.patch || die "Failed to patch Boost"
	#cd boost_1_63_0
	./bootstrap.sh --with-libraries=filesystem,program_options,system,chrono,thread --exec-prefix=/usr/local || die "Cannot boostrap boost"
	sudo checkinstall -y --pkgname poma-boost --pkgversion 1 --pkgrelease 1  ./b2 install --prefix=/usr/local
fi
