cmake_minimum_required(VERSION 2.6)

project(MapCRC)
include_directories(
${MapCRC_SOURCE_DIR}
${MapCRC_SOURCE_DIR}/../../contrib/systemiv
${MapCRC_SOURCE_DIR}/../../contrib
)

add_definitions(-DTARGET_OS_LINUX -DTARGET_ARCH_AMD64)

add_executable(MapCRC
#Contrib
../../contrib/unrar/crc.h
../../contrib/unrar/crc.cpp

#SystemIV
../../contrib/systemiv/lib/filesys/binary_stream_readers.h
../../contrib/systemiv/lib/filesys/binary_stream_readers.cpp
../../contrib/systemiv/lib/debug_utils.cpp
../../contrib/systemiv/lib/math/filecrc.h
../../contrib/systemiv/lib/math/filecrc.cpp
../../contrib/systemiv/lib/filesys/filesys_utils.h
../../contrib/systemiv/lib/filesys/filesys_utils.cpp
../../contrib/systemiv/lib/tosser/llist.h
../../contrib/systemiv/lib/string_utils.cpp
../../contrib/systemiv/lib/netlib/net_mutex.h
../../contrib/systemiv/lib/netlib/net_mutex_linux.cpp

lib/universal_include.h

main.cpp
)

target_link_libraries(MapCRC
pthread
)
