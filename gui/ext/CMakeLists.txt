#just setting it to the version I have
cmake_minimum_required(VERSION 2.8)

project(datajockey_view)

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

SET(DATAJOCKEY_AUDIO_HEADERS ../../audio/include/ ../../audio/wrapper/include/)

#SET(QTBINDINGS_INCLUDES /home/alex/.rvm/gems/ruby-1.9.2-p0/gems/qtbindings-4.6.3.2/ext/ruby/qtruby/src/ )
SET(QTBINDINGS_INCLUDES /home/alex/.rvm/gems/ruby-1.9.2-p136/gems/qtbindings-4.6.3.2/ext/ruby/qtruby/src/ )
include_directories( ${CMAKE_SOURCE_DIR}/smoke 
   ${RUBY_INCLUDE_DIRS} 
   ${CMAKE_SOURCE_DIR}/ruby/qtruby/src 
   ${QT_INCLUDES} 
   ${QTBINDINGS_INCLUDES}
   /usr/include/ ./include/ ../include/ . ${DATAJOCKEY_AUDIO_HEADERS})

#set(QTBINDINGS_LIB_DIR /home/alex/.rvm/gems/ruby-1.9.2-p0/gems/qtbindings-4.6.3.2/lib/1.9/)
set(QTBINDINGS_LIB_DIR /home/alex/.rvm/gems/ruby-1.9.2-p136/gems/qtbindings-4.6.3.2/lib/1.9/)
link_directories(${QTBINDINGS_LIB_DIR})
set(rubydatajockey_view_LIBRARY_SRC
   src/datajockey_view.cpp
   src/waveformview.cpp
   ../../audio/src/audiobuffer.cpp
   ../../audio/src/soundfile.cpp
   ../../audio/wrapper/src/audiobufferreference.cpp
   smoke/x_1.cpp
   smoke/x_2.cpp
   smoke/smokedata.cpp)
add_library(rubydatajockey_view MODULE ${rubydatajockey_view_LIBRARY_SRC})
target_link_libraries(rubydatajockey_view
    qtruby4shared
)
set_target_properties(rubydatajockey_view PROPERTIES PREFIX "" OUTPUT_NAME datajockey_view)

install(TARGETS rubydatajockey_view DESTINATION ${CUSTOM_RUBY_SITE_ARCH_DIR})
install(FILES datajockey_view_internal.rb DESTINATION ${CUSTOM_RUBY_SITE_LIB_DIR}/datajockey)

if (Q_WS_MAC)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \"${CUSTOM_RUBY_SITE_ARCH_DIR}/datajockey_view.so\"  \"${CUSTOM_RUBY_SITE_ARCH_DIR}/datajockey_view.bundle\" )" )
endif (Q_WS_MAC)
