# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/rogerio/Tools/Espressif/esp/esp-idf/components/bootloader/subproject"
  "/home/rogerio/Git/OpenDataTelemetry/device-data-esp32-idf/udp_client/build/bootloader"
  "/home/rogerio/Git/OpenDataTelemetry/device-data-esp32-idf/udp_client/build/bootloader-prefix"
  "/home/rogerio/Git/OpenDataTelemetry/device-data-esp32-idf/udp_client/build/bootloader-prefix/tmp"
  "/home/rogerio/Git/OpenDataTelemetry/device-data-esp32-idf/udp_client/build/bootloader-prefix/src/bootloader-stamp"
  "/home/rogerio/Git/OpenDataTelemetry/device-data-esp32-idf/udp_client/build/bootloader-prefix/src"
  "/home/rogerio/Git/OpenDataTelemetry/device-data-esp32-idf/udp_client/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/rogerio/Git/OpenDataTelemetry/device-data-esp32-idf/udp_client/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
