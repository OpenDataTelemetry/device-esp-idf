# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/opt/esp/idf/components/bootloader/subproject"
  "/workspaces/build/bootloader"
  "/workspaces/build/bootloader-prefix"
  "/workspaces/build/bootloader-prefix/tmp"
  "/workspaces/build/bootloader-prefix/src/bootloader-stamp"
  "/workspaces/build/bootloader-prefix/src"
  "/workspaces/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/workspaces/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
