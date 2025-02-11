# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/rogeriocassares/esp/v5.2.2/esp-idf/components/bootloader/subproject"
  "/Users/rogeriocassares/Git/OpenDataTelemetry/device-esp-idf-c6/build/bootloader"
  "/Users/rogeriocassares/Git/OpenDataTelemetry/device-esp-idf-c6/build/bootloader-prefix"
  "/Users/rogeriocassares/Git/OpenDataTelemetry/device-esp-idf-c6/build/bootloader-prefix/tmp"
  "/Users/rogeriocassares/Git/OpenDataTelemetry/device-esp-idf-c6/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/rogeriocassares/Git/OpenDataTelemetry/device-esp-idf-c6/build/bootloader-prefix/src"
  "/Users/rogeriocassares/Git/OpenDataTelemetry/device-esp-idf-c6/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/rogeriocassares/Git/OpenDataTelemetry/device-esp-idf-c6/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/rogeriocassares/Git/OpenDataTelemetry/device-esp-idf-c6/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
