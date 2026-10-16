cmake_minimum_required(VERSION 3.16)

# The app version comes from version.txt and must be a semantic version:
# devices install an image only when its version is newer than their own.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(secure_ota_demo)
