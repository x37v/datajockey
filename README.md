There is a lot of development going on right now.  I would love help if you're
interested.  Contact me on irc [xnor in #lad on freenode] or via email [alex at
x37v.info]

# Dependencies: 

## all:
 * qt >= 5
 * jack
   * jackd
   * libjack
 * cmake
 * Queen Mary Beat Analysis plugins: http://isophonics.net/QMVampPlugins
 * libraries:
   * boost
   * libsndfile
   * libmad
   * libvorbisfile
   * liblo
   * libtag

## linux:
 * lilv and its dependencies
 * vamp:
   * headers are in vamp-plugin-sdk
   * you'll also want libvamp-hostsdk3 installed

## Ubuntu Quick Start:
 * sudo apt-get install qt5-default qttools5-dev-tools qtcreator liblilv-dev vamp-plugin-sdk libvamp-hostsdk3v5 libsndfile-dev libmad0-dev libvorbis-dev liblo-dev libtag1-dev cmake libjack-jackd2-dev libboost-dev jackd2
 * download the gziped tar for linux [choose your bit-flavor]: http://isophonics.net/QMVampPlugins
 * install the qm vamp plugins, read the INSTALL_linux.txt file
 * start up qtcreator and create a project for, *I use all default settings*:
	 * ext/yaml-cpp/CMakeLists.txt
	 * app/app.pro
	   * make depend on yaml-cpp
	 * importer/importer.pro
	   * make depend on yaml-cpp
 * Build!

# Notes:

## osx:
debugging with xcode 6, you must select 'run in terminal' in the 'run' settings of the project

