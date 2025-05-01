# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\DataAcquisitionTest1_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\DataAcquisitionTest1_autogen.dir\\ParseCache.txt"
  "DataAcquisitionTest1_autogen"
  )
endif()
