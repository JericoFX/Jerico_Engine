# Install script for directory: F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/source-work/libexpat-e26a263aa9182d1e4552cd615857f0f61df3ba42/expat

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/JELocalExpat")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/build/upstream/expat_config.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/build/lib/libexpatMD.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/source-work/libexpat-e26a263aa9182d1e4552cd615857f0f61df3ba42/expat/lib/expat.h"
    "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/source-work/libexpat-e26a263aa9182d1e4552cd615857f0f61df3ba42/expat/lib/expat_external.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/expat" TYPE FILE FILES
    "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/source-work/libexpat-e26a263aa9182d1e4552cd615857f0f61df3ba42/expat/AUTHORS"
    "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/build/upstream/changelog"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/expat-2.4.2" TYPE FILE FILES
    "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/build/upstream/cmake/expat-config.cmake"
    "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/build/upstream/cmake/expat-config-version.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/expat-2.4.2/expat.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/expat-2.4.2/expat.cmake"
         "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/build/upstream/CMakeFiles/Export/f5f66dd1ec74b28a42c7393242053fd9/expat.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/expat-2.4.2/expat-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/expat-2.4.2/expat.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/expat-2.4.2" TYPE FILE FILES "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/build/upstream/CMakeFiles/Export/f5f66dd1ec74b28a42c7393242053fd9/expat.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/expat-2.4.2" TYPE FILE FILES "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/build/upstream/CMakeFiles/Export/f5f66dd1ec74b28a42c7393242053fd9/expat-release.cmake")
  endif()
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "F:/JericoEngine/JericoEngine/build/n0-validation/local-packages/expat-rev1-validation/expat-local-v1/build/upstream/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
