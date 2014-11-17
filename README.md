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

## linux:
 * lilv and its dependencies
 * vamp:
   * headers are in vamp-plugin-sdk
   * you'll also want libvamp-hostsdk3 installed

# Notes:

## osx:
debugging with xcode 6, you must select 'run in terminal' in the 'run' settings of the project

