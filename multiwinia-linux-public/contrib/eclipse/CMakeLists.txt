cmake_minimum_required(VERSION 2.6)

project(Eclipse)
include_directories(
${Eclipse_SOURCE_DIR}
${Eclipse_SOURCE_DIR}/../../contrib/systemiv
${Eclipse_SOURCE_DIR}/../../code
)

add_definitions(-DTARGET_OS_LINUX)

add_library(Eclipse
eclipse.h

eclbutton.h
eclwindow.h

eclipse.cpp
eclbutton.cpp
eclwindow.cpp
)

