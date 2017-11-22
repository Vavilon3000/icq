cmake_minimum_required(VERSION 3.4)

project(updater)


if(NOT UNIX OR APPLE)
    message(FATAL_ERROR "Only Linux is supported")
endif()

get_filename_component(ICQ_ROOT "${CMAKE_SOURCE_DIR}/.." ABSOLUTE)
set(ICQ_EXTERNAL "${ICQ_ROOT}/external")
set(ICQ_QT_ROOT "${ICQ_EXTERNAL}/linux/Qt")

if(LINUX_ARCH EQUAL 64)
    set(LINUX64 ON)
    set(ARCH_FLAGS "-m64")
    set(ICQ_PLATFORM linux/x64)
    message(STATUS "Linux x64")
else()
    set(LINUX32 ON)
    set(ARCH_FLAGS "-m32")
    set(ICQ_PLATFORM linux)
    message(STATUS "Linux x32")
endif()

set(ICQ_EXTERNAL "${ICQ_ROOT}/external")
set(ICQ_EXTERNAL_PLATFORM "${ICQ_EXTERNAL}/${ICQ_PLATFORM}")

if(LINUX32)
    set(ICQ_QT_LIB_DIR "${ICQ_QT_ROOT}/lib")
    set(UPDATER_QRC "${CMAKE_SOURCE_DIR}/resource32.qrc")
    set(UPDATER_BIN_DIR "${CMAKE_SOURCE_DIR}/bin/Release32")
else()
    set(ICQ_QT_LIB_DIR "${ICQ_QT_ROOT}/x64/lib")
    set(UPDATER_QRC "${CMAKE_SOURCE_DIR}/resource64.qrc")
    set(UPDATER_BIN_DIR "${CMAKE_SOURCE_DIR}/bin/Release64")
endif()

set(ICQ_QT_INCLUDE_DIR "${ICQ_QT_ROOT}/include")

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ARCH_FLAGS} -static-libgcc -static-libstdc++")

set(QT_LIBRARIES ${ICQ_QT_LIB_DIR}/libQt5Core.a ${ICQ_QT_LIB_DIR}/libqtpcre.a)

set(SYSTEM_LIBRARIES -Wl,-Bdynamic -ldl -lpthread)

include_directories("${ICQ_QT_INCLUDE_DIR}")
include_directories("${ICQ_QT_INCLUDE_DIR}/QtCore")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${UPDATER_BIN_DIR})

get_filename_component(ICQ_RCC_EXE "${ICQ_QT_ROOT}/bin/rcc" ABSOLUTE)

add_custom_command(OUTPUT qresource.cpp
                   COMMAND ${ICQ_RCC_EXE} ${UPDATER_QRC} -pass 1 -o qresource.cpp
                   DEPENDS ${UPDATER_QRC})

add_custom_command(OUTPUT qresource.tmp.o
                   COMMAND ${CMAKE_CXX_COMPILER} ${ARCH_FLAGS} -c qresource.cpp -o qresource.tmp.o
                   DEPENDS qresource.cpp)

add_custom_command(OUTPUT qresource_final.o
                   COMMAND ${ICQ_RCC_EXE} ${UPDATER_QRC} -pass 2 -temp qresource.tmp.o -o qresource_final.o
                   DEPENDS qresource.tmp.o)

add_executable(updater main.cpp qresource_final.o)

target_link_libraries(updater ${QT_LIBRARIES} ${SYSTEM_LIBRARIES})
