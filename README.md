# Poma Pipeline

This tool enables you to easily create modular pipelines for processing data. Multiple threads and/or distributed execution (on multiple machines) is natively supported. Pipelines are defined through a JSON file: each module is compiled into a separate library which gets loaded dynamically. If necessary pipeline descriptions can be embedded into the code of your application.
A description of the base modules shipped with this tool is in the Documentation directory.

To compile you need to satisfy the following dependencies:

- CMake (git version > 3.9)
- Boost >= 1.65.0
- (for distributed processing) 0MQ (ZeroMQ)

## Install instructions (Ubuntu/Debian)

These are the instructions to compile the framework on a Ubuntu/Debian machine. To keep you system clean we are going to package additional software into deb files. This procedure was tested on a clean Ubuntu MATE 16.04.2 install.

### Install CMake
Since CMake releases do not yet support Boost >=1.64 (hence also 1.65) we are going to install from https://github.com/Kitware/CMake.git: before continuing ensure that you have no CMake package installed on your system.

First install the required dependencies `sudo apt-get install --assume-yes build-essential checkinstall fakeroot git pkg-config unzip libcurl4-openssl-dev libz-dev wget`

Create the required target directories with `sudo mkdir -p /usr/local/share/doc` and `sudo mkdir -p /usr/local/lib`
Configure the library path with `sudo echo  "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/usrlocallib.conf && sudo ldconfig`.

Clone the CMake git repository `git clone https://github.com/Kitware/CMake.git`, and then `cd CMake`. Execute `./bootstrap --system-curl --prefix=/usr/local`. Then you can proceed with `make`. To create a package for CMake use `sudo checkinstall -y --pkgname poma-cmake --pkgversion=3.9 --pkgrelease 1`. Don't forget to add /usr/local/bin to your PATH before continuing (on Ubuntu it should be already set)!

### Install Boost (1.65.0)
Download Boost from https://sourceforge.net/projects/boost/files/boost/1.64.0/boost_1_65_0.tar.bz2 and
uncompress the archive: `wget https://sourceforge.net/projects/boost/files/boost/1.65.0/boost_1_65_0.tar.bz2 && tar xvjpf boost_1_65_0.tar.bz2 && cd boost_1_65_0` .Run `./bootstrap.sh --with-libraries=filesystem,program_options,system,chrono,thread --exec-prefix=/usr/local`. When building is completed, run `sudo checkinstall -y --pkgname poma-boost --pkgversion=1.65 --pkgrelease 1  ./b2 install --prefix=/usr/local` to install and package Boost

### Install ZeroMQ
Install the required tools to compile ZeroMQ with `sudo apt-get install autoconf automake libtool`
Download ZeroMQ from https://github.com/zeromq/libzmq/releases/download/v4.2.2/zeromq-4.2.2.tar.gz with `wget https://github.com/zeromq/libzmq/releases/download/v4.2.2/zeromq-4.2.2.tar.gz`. Uncompress this file `tar xvfz zeromq-4.2.2.tar.gz`and move into the source directory `cd zeromq-4.2.2/`. Execute `./autogen.sh && ./configure` then compile with `make`.
Install with `sudo checkinstall -y --pkgname poma-libzmq --pkgversion 1 --pkgrelease 1` then execute `sudo ldconfig`.

### Compile Poma
Download a snapshot or clone the git repository at https://github.com/slashdotted/PomaPure
From the root directory, run `mkdir build && cd build && cmake ..`; if everything goes well you can compile the program with `make`.

Move to the Modules directory and run `
for i in */; do cd $i && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make && cd ../..; done`
