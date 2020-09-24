# Install script for directory: D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Assimp")
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

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/build/code/Debug/assimp-vc141-mtd.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/build/code/Release/assimp-vc141-mt.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/build/code/MinSizeRel/assimp-vc141-mt.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/build/code/RelWithDebInfo/assimp-vc141-mt.lib")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/build/code/Debug/assimp-vc141-mtd.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/build/code/Release/assimp-vc141-mt.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/build/code/MinSizeRel/assimp-vc141-mt.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/build/code/RelWithDebInfo/assimp-vc141-mt.dll")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xassimp-devx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp" TYPE FILE FILES
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/anim.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/aabb.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/ai_assert.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/camera.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/color4.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/color4.inl"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/build/code/../include/assimp/config.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/defs.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Defines.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/cfileio.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/light.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/material.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/material.inl"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/matrix3x3.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/matrix3x3.inl"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/matrix4x4.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/matrix4x4.inl"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/mesh.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/pbrmaterial.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/postprocess.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/quaternion.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/quaternion.inl"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/scene.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/metadata.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/texture.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/types.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/vector2.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/vector2.inl"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/vector3.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/vector3.inl"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/version.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/cimport.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/importerdesc.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Importer.hpp"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/DefaultLogger.hpp"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/ProgressHandler.hpp"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/IOStream.hpp"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/IOSystem.hpp"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Logger.hpp"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/LogStream.hpp"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/NullLogger.hpp"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/cexport.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Exporter.hpp"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/DefaultIOStream.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/DefaultIOSystem.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/ZipArchiveIOSystem.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/SceneCombiner.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/fast_atof.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/qnan.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/BaseImporter.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Hash.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/MemoryIOWrapper.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/ParsingUtils.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/StreamReader.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/StreamWriter.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/StringComparison.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/StringUtils.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/SGSpatialSort.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/GenericProperty.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/SpatialSort.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/SkeletonMeshBuilder.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/SmoothingGroups.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/SmoothingGroups.inl"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/StandardShapes.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/RemoveComments.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Subdivision.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Vertex.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/LineSplitter.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/TinyFormatter.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Profiler.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/LogAux.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Bitmap.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/XMLTools.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/IOStreamBuffer.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/CreateAnimMesh.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/irrXMLWrapper.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/BlobIOSystem.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/MathFunctions.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Macros.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Exceptional.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/ByteSwapper.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xassimp-devx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/assimp/Compiler" TYPE FILE FILES
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Compiler/pushpack1.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Compiler/poppack1.h"
    "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/code/../include/assimp/Compiler/pstdint.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/build/code/Debug/assimp-vc141-mtd.pdb")
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "D:/VisualStudioProjects/ActoryEngine/libs/assimp-master/build/code/RelWithDebInfo/assimp-vc141-mt.pdb")
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
endif()

