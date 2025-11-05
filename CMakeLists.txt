cmake_minimum_required(VERSION 3.16)
# Disable Component Manager to avoid Python package incompatibility on this machine
set(IDF_COMPONENT_MANAGER OFF CACHE BOOL "Disable Component Manager")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_jig)
