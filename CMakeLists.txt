# Valve Project Creator is a tool to generate build systems for Visual Studio, Xbox 360, PlayStation 3, Xcode and Make.
#
# See https://developer.valvesoftware.com/wiki/Valve_Project_Creator

cmake_minimum_required(VERSION 3.10)

# Default build type is Release unless overridden.
# On Windows default is Debug, so need to unify.
# See https://discourse.cmake.org/t/how-to-deal-with-ninja-setting-cmake-build-type-to-debug/281/3
set(CMAKE_BUILD_TYPE "Release" CACHE STRING
  "Choose the type of build.")
set(SE_VPC_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
  "CMake build type used to build ${PACKAGE_NAME}.")
# Export compile commands for tools like clang-tidy.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL
 "CMake should export compile commmands as tools use it.")

project(vpc LANGUAGES CXX)

# Use static MSVC runtime.
option(SE_VPC_ENABLE_STATIC_MSVC_RUNTIME "Build with static MSVC runtime." ON)

# Use portable debug database files in Release.
option(SE_VPC_ENABLE_PDB_IN_RELEASE "Build with PDB in Release." ON)

# Use Address Sanitizer.
option(SE_VPC_ENABLE_ASAN "Build with Address Sanitizer." OFF)

# Compiler id for Apple Clang is now AppleClang.
if (POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif()

# Setup root directory of the vpc project.
set(SE_VPC_ROOT_DIR "${CMAKE_SOURCE_DIR}" CACHE STRING
  "Directory root of the vpc project.")

## Package information.
set(PACKAGE_NAME   "vpc")

# C++ standard can be overridden when this is used as a sub-project.
if(NOT CMAKE_CXX_STANDARD)
  # This project requires C++17.
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif(NOT CMAKE_CXX_STANDARD)

# Git is required to build software version via Git.
find_package(Git)

if (NOT DEFINED PACKAGE_VERSION)
  if (NOT Git_FOUND)
    message(FATAL "[common]: Unable to extract Git software version - no Git.")
  endif()

  message(STATUS "[common]: Extracting Git software version.")

  if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    execute_process(COMMAND cmd /c "git rev-list --branches HEAD | find /v " " /c" OUTPUT_VARIABLE SE_VPC_GIT_VERSION WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  else()
    execute_process(COMMAND bash "-c" "git rev-list --branches HEAD | wc -l | tr -d ' ' | tr -d '\n'" OUTPUT_VARIABLE SE_VPC_GIT_VERSION WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  set(PACKAGE_VER_PATCH 2)
  math(EXPR PACKAGE_VER_TWEAK ${SE_VPC_GIT_VERSION})
  message(STATUS
    "[common]: Build version from Git (patch.tweak): ${PACKAGE_VER_PATCH}.${PACKAGE_VER_TWEAK}.")

  set(PACKAGE_VER_MAJOR 1)
  set(PACKAGE_VER_MINOR 5)

  set(PACKAGE_VERSION
    "${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}.${PACKAGE_VER_PATCH}.${PACKAGE_VER_TWEAK}")
endif()
set(PACKAGE_STRING     "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME    "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT  "https://github.com/Source-Authors/vpc/issues")

project(${PACKAGE_NAME}
  VERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}.${PACKAGE_VER_PATCH}.${PACKAGE_VER_TWEAK}
  LANGUAGES CXX)

message(STATUS "[common]: CMake version: ${CMAKE_VERSION}.")
message(STATUS "[common]: ${PACKAGE_NAME} version: ${PACKAGE_VERSION}.")

## Common variables.

set(SE_VPC_BINARY_DIR "${CMAKE_BINARY_DIR}" CACHE STRING
  "Directory of the vpc binaries output.")
set(SE_VPC_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}" CACHE STRING
  "Name of the vpc build system.")

set(SE_VPC_ENABLE_STD_ONLY OFF)

if (${SE_VPC_SYSTEM_NAME} STREQUAL "Darwin")
  set(SE_VPC_OS_MACOS ON CACHE BOOL "MacOS is build system.")
  set(SE_VPC_ENABLE_STD_ONLY ON)
else()
  set(SE_VPC_OS_MACOS OFF CACHE BOOL "MacOS is NOT build system.")
endif()

if (${SE_VPC_SYSTEM_NAME} STREQUAL "Linux")
  set(SE_VPC_OS_LINUX ON CACHE BOOL "Linux distro is build system.")
  set(SE_VPC_ENABLE_STD_ONLY ON)
else()
  set(SE_VPC_OS_LINUX OFF CACHE BOOL "Linux distro is NOT build system.")
endif()

if (${SE_VPC_SYSTEM_NAME} STREQUAL "Windows")
  set(SE_VPC_OS_WIN ON CACHE BOOL "Windows is build system.")
else()
  set(SE_VPC_OS_WIN OFF CACHE BOOL "Windows is NOT build system.")
endif()

if (SE_VPC_OS_MACOS OR SE_VPC_OS_LINUX)
  set(SE_VPC_OS_POSIX ON CACHE BOOL "POSIX compatible os is build system.")
else()
  set(SE_VPC_OS_POSIX OFF CACHE BOOL "POSIX compatible os is NOT build system.")
endif()

set(SE_VPC_PRODUCT_NAME "vpc" CACHE STRING
  "Name of the vpc product.")

set(SE_VPC_CMAKE_GENERATOR "${CMAKE_GENERATOR}" CACHE STRING
  "CMake generator used to generate build tree." FORCE)
set(SE_VPC_CXX_COMPILER "${CMAKE_CXX_COMPILER}" CACHE STRING
  "Compiler used to build ${PACKAGE_NAME}.")
set(SE_VPC_CXX_COMPILER_ID "${CMAKE_CXX_COMPILER_ID}" CACHE STRING
  "Compiler id used to build ${PACKAGE_NAME}.")
set(SE_VPC_CXX_COMPILER_FRONTEND_VARIANT "${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}"
  CACHE STRING "Compiler frontend used to build ${PACKAGE_NAME}.")
set(SE_VPC_EXPORT_COMPILE_COMMANDS ${CMAKE_EXPORT_COMPILE_COMMANDS} CACHE BOOL
  "Enable/Disable output of compile commands during generation.")

if (SE_VPC_CXX_COMPILER_ID MATCHES "Clang")
  set(SE_VPC_COMPILER_CLANG ON CACHE BOOL "Compiler is Clang.")
else()
  set(SE_VPC_COMPILER_CLANG OFF CACHE BOOL "Compiler is NOT Clang.")
endif()

if (SE_VPC_CXX_COMPILER_ID STREQUAL "GNU")
  set(SE_VPC_COMPILER_GCC ON CACHE BOOL "Compiler is GCC.")
else()
  set(SE_VPC_COMPILER_GCC OFF CACHE BOOL "Compiler is NOT GCC.")
endif()

if (SE_VPC_CXX_COMPILER_ID STREQUAL "MSVC")
  set(SE_VPC_COMPILER_MSVC ON CACHE BOOL "Compiler is MSVC.")
else()
  set(SE_VPC_COMPILER_MSVC OFF CACHE BOOL "Compiler is NOT MSVC.")
endif()

## Common dependencies.

if (Git_FOUND)
  message(STATUS
    "[common]: Git version ${GIT_VERSION_STRING} found at '${GIT_EXECUTABLE}'.")

  # Get the current working branch.
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${SE_VPC_ROOT_DIR}
    OUTPUT_VARIABLE SE_VPC_GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Get the latest commit hash.
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
    WORKING_DIRECTORY ${SE_VPC_ROOT_DIR}
    OUTPUT_VARIABLE SE_VPC_GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  # Get the subject of the commit.
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" log -1 --format=%s
    WORKING_DIRECTORY "${SE_VPC_ROOT_DIR}"
    OUTPUT_VARIABLE SE_VPC_GIT_COMMIT_SUBJECT
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

  message(STATUS
    "[common]: Git using branch '${SE_VPC_GIT_BRANCH}', "
    "commit ${SE_VPC_GIT_COMMIT_HASH} | ${SE_VPC_GIT_COMMIT_SUBJECT}.")
endif()

message(STATUS
  "[common]: ${PACKAGE_NAME} using CMake generator: ${SE_VPC_CMAKE_GENERATOR}.")

# Select correct threading library.
if (NOT SE_VPC_OS_WIN)
  set(CMAKE_THREAD_PREFER_PTHREAD ON)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()
find_package(Threads REQUIRED)
list(APPEND CMAKE_REQUIRED_LIBRARIES Threads::Threads)
if (NOT CMAKE_THREAD_LIBS_INIT)
  set(SE_VPC_THREADS_LIBRARY "system default" CACHE STRING
    "Threads library used to build ${PACKAGE_NAME}.")
else()
  set(SE_VPC_THREADS_LIBRARY "${CMAKE_THREAD_LIBS_INIT}" CACHE STRING
    "Threads library used to build ${PACKAGE_NAME}.")
endif()
message(STATUS
  "[common]: ${PACKAGE_NAME} using threading library: ${SE_VPC_THREADS_LIBRARY}.")

if (XCODE_VERSION)
  message(STATUS
    "[common]: ${PACKAGE_NAME} using XCode version: ${XCODE_VERSION}.")
endif()

message(STATUS "[common]: ${PACKAGE_NAME} cxx compiler: ${SE_VPC_CXX_COMPILER}.")
message(STATUS "[common]: ${PACKAGE_NAME} build type: ${SE_VPC_BUILD_TYPE}.")

if (SE_VPC_OS_WIN)
  set(SE_PRODUCT_NAME "Valve Project Creator")

  # Generate common product version info.
  configure_file(
    ${SE_VPC_ROOT_DIR}/build/product_version_config.h.cmake.in
    ${SE_VPC_BINARY_DIR}/build/product_version_config.h
  )

  # Generate app version info.
  configure_file(
    ${SE_VPC_ROOT_DIR}/build/app_version_config.h.cmake.in
    ${SE_VPC_BINARY_DIR}/build/app_version_config.h
  )

  # Generate visual styles manifest info.
  configure_file(
    ${SE_VPC_ROOT_DIR}/build/win/manifests/enable-visual-styles.manifest.cmake.in
    ${SE_VPC_BINARY_DIR}/manifests/enable-visual-styles.manifest
  )
endif()

# https://github.com/izenecloud/cmake/blob/master/SetCompilerWarningAll.cmake
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use the highest warning level for Visual Studio.
  set(CMAKE_CXX_WARNING_LEVEL 4)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")

  # Treat compiler warnings as errors.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")

  # Enable Secure Development Lifecycle checks.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /sdl")

  # Ignore not so interesting warnings.
  # C4127: conditional expression is constant
  # C4201: nonstandard extension used : nameless struct/union
  # C4324: structure was padded due to alignment specifier
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127 /wd4201 /wd4324")

  # Enable C++ + SEH exceptions.
  string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa")

  # Enable multiprocessor compilation.
  string(REGEX REPLACE "/MP" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

  # Enable intrinsic functions.
  string(REGEX REPLACE "/Oi" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Oi")

  # Enable Control Flow Guard.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /guard:cf")
  # Enable Control Flow Guard Continuation Metadata.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /guard:ehcont")

  # Use stricter C++ standard behavior.
  string(REGEX REPLACE "/permissive-*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")

  # Enforce type conversion rules.
  string(REGEX REPLACE "/Zc:rvalueCast" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:rvalueCast")

  # Treat linker warnings as errors.
  string(REGEX REPLACE "/WX\:NO" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "/WX" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")

  if (SE_VPC_ENABLE_ASAN)
    message(STATUS "[options]: ASAN enabled.")

    # Use AddressSanitizer.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
  endif (SE_VPC_ENABLE_ASAN)

  if (SE_VPC_ENABLE_STATIC_MSVC_RUNTIME)
    # Use static runtime.
    foreach(flag_var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      if (${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      endif()
    endforeach()
  endif (SE_VPC_ENABLE_STATIC_MSVC_RUNTIME)
else(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use -Wall for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")

  # Use -Wextra for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")

  # Use -Werror.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

  # Ignore not so interesting warnings.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-array-bounds -Wno-overloaded-virtual -Wno-invalid-offsetof")

  # For any Clang disable uninteresting warnings.
  # See https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-null-pointer-arithmetic")
  endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang$")

  # Enable C++ exceptions.
  string(REGEX REPLACE "-fexceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

  if (SE_VPC_ENABLE_ASAN)
    message(STATUS "[options]: ASAN enabled.")

    # Use AddressSanitizer.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  endif (SE_VPC_ENABLE_ASAN)
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

add_executable(${PACKAGE_NAME} "")

target_include_directories(${PACKAGE_NAME}
  PRIVATE
    ${PROJECT_SOURCE_DIR}
    public/
    ${SE_VPC_BINARY_DIR}/build
)

target_sources(${PACKAGE_NAME}
  PRIVATE
    interfaces/interfaces.cpp
    tier0/assert_dialog.cpp
    tier0/commandline.cpp
    tier0/cpu.cpp
    tier0/cputopology.cpp
    tier0/dbg.cpp
    tier0/dlmalloc/malloc.cpp
    tier0/etwprof.cpp
    tier0/fasttimer.cpp
    tier0/logging.cpp
    tier0/mem.cpp
    tier0/memdbg.cpp
    tier0/memstd.cpp
    tier0/mem_helpers.cpp
    tier0/minidump.cpp
    tier0/pch_tier0.cpp
    tier0/pmelib.cpp
    tier0/stacktools.cpp
    tier0/threadtools.cpp
    tier0/tier0_strtools.cpp
    tier0/valobject.cpp
    tier0/vprof.cpp
    tier0/win32consoleio.cpp
    tier1/characterset.cpp
    tier1/checksum_crc.cpp
    tier1/checksum_md5.cpp
    tier1/convar.cpp
    tier1/exprevaluator.cpp
    tier1/generichash.cpp
    tier1/interface.cpp
    tier1/keyvalues.cpp
    tier1/mempool.cpp
    tier1/memstack.cpp
    tier1/splitstring.cpp
    tier1/stringpool.cpp
    tier1/strtools.cpp
    tier1/tier1.cpp
    tier1/utlbuffer.cpp
    tier1/utlstring.cpp
    tier1/utlsymbol.cpp
    utils/vpccrccheck/crccheck_shared.cpp
    utils/vpc/baseprojectdatacollector.cpp
    utils/vpc/conditionals.cpp
    utils/vpc/configuration.cpp
    utils/vpc/dependencies.cpp
    utils/vpc/exprsimplifier.cpp
    utils/vpc/generatordefinition.cpp
    utils/vpc/groupscript.cpp
    utils/vpc/macros.cpp
    utils/vpc/main.cpp
    utils/vpc/memory_reservation_x64.cpp
    utils/vpc/p4sln.cpp
    utils/vpc/projectgenerator_codelite.cpp
    utils/vpc/projectgenerator_makefile.cpp
    utils/vpc/projectgenerator_ps3.cpp
    utils/vpc/projectgenerator_vcproj.cpp
    utils/vpc/projectscript.cpp
    utils/vpc/scriptsource.cpp
    utils/vpc/solutiongenerator_codelite.cpp
    utils/vpc/solutiongenerator_makefile.cpp
    utils/vpc/solutiongenerator_xcode.cpp
    utils/vpc/sys_utils.cpp
    utils/vpc/vpc.cpp
    vstdlib/cvar.cpp
    vstdlib/keyvaluessystem.cpp
    vstdlib/random.cpp
    vstdlib/vstrtools.cpp

    public/appframework/iappsystem.h
    public/color.h
    public/datamap.h
    public/filesystem.h
    public/filesystem_passthru.h
    public/icvar.h
    public/ilaunchabledll.h
    public/interfaces/interfaces.h
    public/mathlib/fltx4.h
    public/mathlib/mathlib.h
    public/mathlib/math_pfns.h
    public/mathlib/vector.h
    public/mathlib/vector2d.h
    public/p4lib/ip4.h
    public/tier0/annotations.h
    public/tier0/basetypes.h
    public/tier0/commonmacros.h
    public/tier0/dbg.h
    public/tier0/dbgflag.h
    public/tier0/etwprof.h
    public/tier0/eventmasks.h
    public/tier0/eventmodes.h
    public/tier0/fasttimer.h
    public/tier0/ia32detect.h
    public/tier0/icommandline.h
    public/tier0/ioctlcodes.h
    public/tier0/k8performancecounters.h
    public/tier0/l2cache.h
    public/tier0/logging.h
    public/tier0/mem.h
    public/tier0/memalloc.h
    public/tier0/memdbgoff.h
    public/tier0/memdbgon.h
    public/tier0/memvirt.h
    public/tier0/minidump.h
    public/tier0/p4performancecounters.h
    public/tier0/p5p6performancecounters.h
    public/tier0/platform.h
    public/tier0/pmelib.h
    public/tier0/stackstats.h
    public/tier0/stacktools.h
    public/tier0/threadtools.h
    public/tier0/tslist.h
    public/tier0/validator.h
    public/tier0/valobject.h
    public/tier0/valve_off.h
    public/tier0/valve_on.h
    public/tier0/vprof.h
    public/tier0/vprof_sn.h
    public/tier0/wchartypes.h
    public/tier0/win32consoleio.h
    public/tier0/xbox_codeline_defines.h
    public/tier1/byteswap.h
    public/tier1/characterset.h
    public/tier1/checksum_crc.h
    public/tier1/checksum_md5.h
    public/tier1/convar.h
    public/tier1/convar_serverbounded.h
    public/tier1/exprevaluator.h
    public/tier1/fmtstr.h
    public/tier1/functors.h
    public/tier1/generichash.h
    public/tier1/iconvar.h
    public/tier1/interface.h
    public/tier1/keyvalues.h
    public/tier1/mempool.h
    public/tier1/memstack.h
    public/tier1/netadr.h
    public/tier1/refcount.h
    public/tier1/stringpool.h
    public/tier1/strtools.h
    public/tier1/tier1.h
    public/tier1/utlblockmemory.h
    public/tier1/utlbuffer.h
    public/tier1/utldict.h
    public/tier1/utlenvelope.h
    public/tier1/utlfixedmemory.h
    public/tier1/utlgraph.h
    public/tier1/utlhash.h
    public/tier1/utllinkedlist.h
    public/tier1/utlmap.h
    public/tier1/utlmemory.h
    public/tier1/utlmultilist.h
    public/tier1/utlqueue.h
    public/tier1/utlrbtree.h
    public/tier1/utlsortvector.h
    public/tier1/utlstack.h
    public/tier1/utlstring.h
    public/tier1/utlsymbol.h
    public/tier1/utlvector.h
    public/tier2/tier2.h
    public/unitlib/unitlib.h
    public/vstdlib/cvar.h
    public/vstdlib/ikeyvaluessystem.h
    public/vstdlib/pch_vstdlib.h
    public/vstdlib/random.h
    public/vstdlib/vstdlib.h
    public/vstdlib/vstrtools.h
    public/winlite.h
    tier0/cputopology.h
    tier0/memstd.h
    tier0/mem_helpers.h
    tier0/mem_impl_type.h
    tier0/pch_tier0.h
    tier0/resource.h
    tier0/tier0_strtools.h
    tier0/ValveETWProviderEvents.h
    utils/vpccrccheck/crccheck_shared.h
    utils/vpc/app_version_config.h
    utils/vpc/baseprojectdatacollector.h
    utils/vpc/dependencies.h
    utils/vpc/generatordefinition.h
    utils/vpc/ibaseprojectgenerator.h
    utils/vpc/ibasesolutiongenerator.h
    utils/vpc/memory_reservation_x64.h
    utils/vpc/p4sln.h
    utils/vpc/product_version_config.h
    utils/vpc/projectgenerator_codelite.h
    utils/vpc/projectgenerator_ps3.h
    utils/vpc/projectgenerator_vcproj.h
    utils/vpc/scriptsource.h
    utils/vpc/sys_utils.h
    utils/vpc/vpc.h
    vstdlib/concommandhash.h
)

if (SE_VPC_OS_WIN)
  target_sources(${PACKAGE_NAME}
    PRIVATE
      tier0/pme.cpp
      tier0/platform.cpp

      utils/vpc/projectgenerator_win32.cpp
      utils/vpc/projectgenerator_win32_2010.cpp
      utils/vpc/projectgenerator_xbox360.cpp
      utils/vpc/projectgenerator_xbox360_2010.cpp
      utils/vpc/solutiongenerator_win32.cpp
      utils/vpc/projectgenerator_win32.h
      utils/vpc/projectgenerator_win32_2010.h
      utils/vpc/projectgenerator_xbox360.h
      utils/vpc/projectgenerator_xbox360_2010.h
  )
endif (SE_VPC_OS_WIN)

target_compile_definitions(${PACKAGE_NAME}
  PRIVATE
    NO_PERFORCE
    STANDALONE_VPC=1
    STATIC_TIER0=1
    STATIC_VSTDLIB=1
    "$<$<CONFIG:DEBUG>:_DEBUG>"
    "$<$<CONFIG:DEBUG>:DEBUG>"
)

if (NOT SE_VPC_COMPILER_MSVC OR SE_VPC_CXX_COMPILER MATCHES "x64")
  target_compile_definitions(${PACKAGE_NAME}
    PRIVATE
      PLATFORM_64BITS=1
      X64BITS=1
  )
endif (NOT SE_VPC_COMPILER_MSVC OR SE_VPC_CXX_COMPILER MATCHES "x64")

if (SE_VPC_COMPILER_MSVC)
  target_compile_definitions(${PACKAGE_NAME}
    PRIVATE
      COMPILER_MSVC=1
      COMPILER_MSVC64=1
  )

  if (SE_VPC_ENABLE_PDB_IN_RELEASE)
    target_compile_options(${PACKAGE_NAME} PRIVATE "$<$<CONFIG:Release>:/Zi>")
    target_link_options(${PACKAGE_NAME} PRIVATE "$<$<CONFIG:Release>:/DEBUG>")
    target_link_options(${PACKAGE_NAME} PRIVATE "$<$<CONFIG:Release>:/OPT:REF>")
    target_link_options(${PACKAGE_NAME} PRIVATE "$<$<CONFIG:Release>:/OPT:ICF>")
  endif()
elseif (SE_VPC_COMPILER_GCC)
  target_compile_definitions(${PACKAGE_NAME}
    PRIVATE
      COMPILER_GCC=1
  )
elseif (SE_VPC_COMPILER_CLANG)
  target_compile_definitions(${PACKAGE_NAME}
    PRIVATE
      COMPILER_CLANG=1
  )
endif (SE_VPC_COMPILER_MSVC)

if (SE_VPC_OS_LINUX)
  target_compile_definitions(${PACKAGE_NAME}
    PRIVATE
      LINUX=1
      _LINUX=1
  )
endif (SE_VPC_OS_LINUX)

if (SE_VPC_OS_MACOS)
  target_compile_definitions(${PACKAGE_NAME}
    PRIVATE
      OSX=1
      _OSX=1
  )
endif (SE_VPC_OS_MACOS)

if (SE_VPC_OS_POSIX)
  target_sources(${PACKAGE_NAME}
    PRIVATE
      tier0/cpu_posix.cpp
      tier0/platform_posix.cpp
      tier0/pme_posix.cpp
  )

  target_compile_definitions(${PACKAGE_NAME}
    PRIVATE
      POSIX=1
      _POSIX=1
      HAVE_USR_INCLUDE_MALLOC_H=1
  )
endif (SE_VPC_OS_POSIX)

set_target_properties(${PACKAGE_NAME}
  PROPERTIES
    VERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}
    SOVERSION ${PACKAGE_VER_MAJOR}.${PACKAGE_VER_MINOR}
)

if (SE_VPC_OS_WIN)
  target_sources(${PACKAGE_NAME}
    PRIVATE
      resource.h
      ${SE_VPC_BINARY_DIR}/manifests/enable-visual-styles.manifest
      build/win/manifests/dpi-aware.manifest
      build/win/manifests/heap-type.manifest
      build/win/manifests/supported-os.manifest
      build/win/manifests/ultra-high-scroll-resolution.manifest
      build/win/manifests/utf-8-code-page.manifest
      build/win/resource_scripts/windows_app_base.rc
  )

  target_compile_definitions(${PACKAGE_NAME}
    PRIVATE
      STRICT=1
      NOMINMAX=1
      _CRT_SECURE_NO_WARNINGS=1
      SE_PRODUCT_INTERNAL_NAME_STRING="${PACKAGE_NAME}"
      SE_PRODUCT_ORIGINAL_NAME_STRING="${PACKAGE_NAME}.exe"
  )
endif (SE_VPC_OS_WIN)
