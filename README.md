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

# Notes:

## osx:
debugging with xcode 6, you must select 'run in terminal' in the 'run' settings of the project

## Ubuntu 17.10:
```
sudo apt-get install qt5-default libjack-jackd2-dev cmake libboost-dev libsndfile1-dev libmad0-dev libvorbisfile3 liblo-dev libtagc0-dev liblilv-dev vamp-plugin-sdk libvamp-hostsdk3v5 qtcreator
```

