#just setting it to the version I have
cmake_minimum_required(VERSION 2.8)

project(datajockey_audio)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../../cmake/modules )
include (MacroOptionalAddBindings)

find_package(Qt4 REQUIRED)

set(COMPILE_RUBY FALSE CACHE INTERNAL "")

find_package(Ruby REQUIRED)

if (RUBY_LIBRARY AND RUBY_INCLUDE_DIRS AND RUBY_EXECUTABLE)
    set(COMPILE_RUBY TRUE CACHE INTERNAL "")
endif (RUBY_LIBRARY AND RUBY_INCLUDE_DIRS AND RUBY_EXECUTABLE)

#macro_log_feature(COMPILE_RUBY "Ruby" "Ruby interpreter and libraries" "http://www.ruby-lang.org" FALSE "" "Needed to compile the Ruby bindings")

if (NOT COMPILE_RUBY)
    return()
endif (NOT COMPILE_RUBY)

SET(CUSTOM_RUBY_SITE_ARCH_DIR ${RUBY_SITEARCH_DIR} CACHE DIR "custom installation directory for ruby binary extension" )
SET(CUSTOM_RUBY_SITE_LIB_DIR ${RUBY_SITELIB_DIR} CACHE DIR "custom installation directory for ruby extension" )

include_directories( ${CMAKE_SOURCE_DIR}/smoke ${RUBY_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/ruby/qtruby/src ${QT_INCLUDES} /home/alex/.rvm/gems/ruby-1.9.2-p0/gems/qtbindings-4.6.3.2/ext/ruby/qtruby/src/ /usr/include/ ./include/ ../include/ .)

SET(datajockey_MOC_HEADERS
   include/audiocontroller.hpp
   include/audioloaderthread.hpp
)

QT4_WRAP_CPP(datajockey_MOC_SRC ${datajockey_MOC_HEADERS})

link_directories(/home/alex/.rvm/gems/ruby-1.9.2-p0/gems/qtbindings-4.6.3.2/lib/1.9/)
set(rubydatajockey_audio_LIBRARY_SRC 
   ${datajockey_MOC_SRC}
   src/datajockey_audio.cpp 
	../src/audiobuffer.cpp
	../src/audioio.cpp
	src/audiobufferreference.cpp
	src/audiocontroller.cpp
	src/audioloaderthread.cpp
	../src/beatbuffer.cpp
	../src/command.cpp
	../src/master.cpp
	../src/player.cpp
	../src/scheduler.cpp
	../src/schedulenode.cpp
	../src/soundfile.cpp
	../src/timepoint.cpp
	../src/transport.cpp
   smoke/smokedata.cpp
   smoke/x_1.cpp smoke/x_2.cpp)
add_library(rubydatajockey_audio MODULE ${rubydatajockey_audio_LIBRARY_SRC})
target_link_libraries(rubydatajockey_audio
    qtruby4shared
    sndfile
    vorbisfile
    mad
    jack
    jackcpp
    rubberband
)
set_target_properties(rubydatajockey_audio PROPERTIES PREFIX "" OUTPUT_NAME datajockey_audio)

install(TARGETS rubydatajockey_audio DESTINATION ${CUSTOM_RUBY_SITE_ARCH_DIR})
install(FILES datajockey_audio_internal.rb DESTINATION ${CUSTOM_RUBY_SITE_LIB_DIR}/datajockey)

if (Q_WS_MAC)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \"${CUSTOM_RUBY_SITE_ARCH_DIR}/datajockey_audio.so\"  \"${CUSTOM_RUBY_SITE_ARCH_DIR}/datajockey_audio.bundle\" )" )
endif (Q_WS_MAC)
