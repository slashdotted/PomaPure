# Poma Pipeline

This tool enables you to easily create modular pipelines for processing data. Multiple threads and/or distributed execution (on multiple machines) is natively supported. Pipelines are defined through a JSON file: each module is compiled into a separate library which gets loaded dynamically. If necessary pipeline descriptions can be embedded into the code of your application.
A description of the base modules shipped with this tool is in the Documentation directory.

To compile you need to satisfy the following dependencies:

- Boost >= 1.63.0
- (for distributed processing) 0MQ

## Install instructions

These are the instructions to compile the framework on a Ubuntu/Debian machine. To keep you system clean we are going to package additional software into deb files.

### Install CMake
We are going to install CMake from https://cmake.org/files/: before continuing ensure that you have no CMake package installed on your system.

First install the required dependencies `sudo apt-get install --assume-yes build-essential checkinstall fakeroot git pkg-config unzip libcurl4-openssl-dev`

Create the required target directories with  `sudo mkdir -p /usr/local/share/doc` and `sudo mkdir -p /usr/local/lib`
Configure the library path with `sudo echo  "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/usrlocallib.conf && sudo ldconfig.

Download the latest version of CMake from https://cmake.org/download/ and uncompress it on your computer. In a terminal, move to the directory where CMake was extracted and execute ./bootstrap --system-curl --prefix=/usr/local`. Then you can proceed with `make`. To create a package for CMake use `sudo checkinstall -y --pkgname poma-cmake --pkgversion 1 --pkgrelease`. Don't forget to add /usr/local/bin to your PATH before continuing!

### Install Boost (>= 1.63.0)

Download Boost from https://sourceforge.net/projects/boost/files/boost/1.63.0/boost_1_63_0.tar.bz2
Uncompress the archive and run `./bootstrap.sh --with-libraries=filesystem,program_options,system,chrono,thread --exec-prefix=/usr/local`. When building is completed, run `sudo checkinstall -y --pkgname poma-boost --pkgversion 1 --pkgrelease 1  ./b2 install --prefix=/usr/local` to install and package Boost

### Compile Poma

Download a snapshot or clone the git repository at https://github.com/slashdotted/PomaPure
From the root directory, run `mkdir build && cd build && cmake ..`; if everything goes well you can compile the program with `make`.

Move to the Modules directory and run `
for i in */; do cd $i && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make && cd ../..; done`






