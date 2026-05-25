cmake_minimum_required(VERSION 3.20)
project(
  QEP
  VERSION 1.0.0
  LANGUAGES CXX)

# ========== 基础设置 ==========
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
  add_compile_options(/wd4244)
endif()
add_compile_definitions(NOMINMAX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

option(QEP_USE_MKL "Use Intel MKL for linear algebra" ON)
option(QEP_USE_AVX2 "Enable AVX2 instruction set" ON)
option(QEP_USE_OPENMP "Enable OpenMP parallelization" ON)
option(QEP_BUILD_CLI "Build CLI solver executable" ON)
option(QEP_BUILD_EXAMPLES "Build example programs" OFF)
option(QEP_BUILD_GUI "Build Qt-based graphical interface" ON)
option(QEP_INSTALL "Generate installation rules" ON)

set(QEP_PROBLEM_BASE_PATH
    "../Problems"
    CACHE PATH "Base path for problem data files")
set(THIRD_PARTY_DIR
    ${PROJECT_SOURCE_DIR}/third_party
    CACHE PATH "Third party libraries directory")

message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message(STATUS "third_party path: ${THIRD_PARTY_DIR}")

# ---- nlohmann/json ----
if(EXISTS "${THIRD_PARTY_DIR}/nlohmann/json.hpp")
  # NLOHMANN_INCLUDE_DIR points to third_party/ root so #include
  # <nlohmann/json.hpp> works
  set(NLOHMANN_INCLUDE_DIR "${THIRD_PARTY_DIR}")
  message(STATUS "Found nlohmann/json: ${THIRD_PARTY_DIR}/nlohmann/json.hpp")
else()
  find_path(
    NLOHMANN_INCLUDE_DIR
    NAMES nlohmann/json.hpp
    DOC "nlohmann/json include directory")
  if(NOT NLOHMANN_INCLUDE_DIR)
    message(
      FATAL_ERROR
        "nlohmann/json not found. Please place nlohmann/json.hpp in third_party/nlohmann/"
    )
  endif()
  message(STATUS "Found nlohmann/json (system): ${NLOHMANN_INCLUDE_DIR}")
endif()

# ---- Eigen3 ----
find_path(
  EIGEN_INCLUDE_DIR
  NAMES Eigen/Core
  PATHS ${THIRD_PARTY_DIR}/eigen ${THIRD_PARTY_DIR}/Eigen
  NO_DEFAULT_PATH
  DOC "Eigen include directory")
if(NOT EIGEN_INCLUDE_DIR)
  find_path(
    EIGEN_INCLUDE_DIR
    NAMES Eigen/Core
    DOC "Eigen include directory")
endif()
if(NOT EIGEN_INCLUDE_DIR)
  message(
    FATAL_ERROR
      "Eigen not found. Please install Eigen or set EIGEN_INCLUDE_DIR.")
endif()
message(STATUS "Found Eigen : ${EIGEN_INCLUDE_DIR}")
add_library(Eigen3::Eigen INTERFACE IMPORTED)
set_target_properties(Eigen3::Eigen PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                               ${EIGEN_INCLUDE_DIR})

# ---- Spectra  ----
find_path(
  SPECTRA_INCLUDE_DIR
  NAMES Spectra/GenEigsSolver.h
  PATHS ${THIRD_PARTY_DIR}/spectra/spectra ${THIRD_PARTY_DIR}/spectra ${THIRD_PARTY_DIR}/Spectra
  NO_DEFAULT_PATH
  DOC "Spectra include directory")
if(NOT SPECTRA_INCLUDE_DIR)
  find_path(
    SPECTRA_INCLUDE_DIR
    NAMES Spectra/GenEigsSolver.h
    DOC "Spectra include directory")
endif()
if(NOT SPECTRA_INCLUDE_DIR)
  message(
    FATAL_ERROR
      "Spectra not found. Please place Spectra headers in ${THIRD_PARTY_DIR}/spectra"
  )
endif()
message(STATUS "Found Spectra: ${SPECTRA_INCLUDE_DIR}")
add_library(Spectra::Spectra INTERFACE IMPORTED)
set_target_properties(Spectra::Spectra PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                  ${SPECTRA_INCLUDE_DIR})

# ---- MKL  ----
if(QEP_USE_MKL)
  set(MKL_ROOT
      ""
      CACHE PATH "Intel MKL installation root")
  if(NOT MKL_ROOT)
    if(DEFINED ENV{ONEAPI_ROOT})
      set(MKL_ROOT "$ENV{ONEAPI_ROOT}/mkl/latest")
    elseif(WIN32)
      set(MKL_ROOT "C:/Program Files (x86)/Intel/oneAPI/mkl/latest")
    elseif(UNIX)
      set(MKL_ROOT "/opt/intel/oneapi/mkl/latest")
    endif()
  endif()
  set(MKL_INCLUDE_DIR "${MKL_ROOT}/include")
  find_library(
    MKL_RT_LIB
    NAMES mkl_rt
    PATHS "${MKL_ROOT}/lib"
    NO_DEFAULT_PATH)
  if(MKL_RT_LIB AND EXISTS "${MKL_INCLUDE_DIR}/mkl.h")
    set(MKL_FOUND TRUE)
    set(MKL_LIBRARIES ${MKL_RT_LIB})
    if(MSVC)
      find_library(
        IOMP5_LIB
        NAMES libiomp5md
        PATHS "${MKL_ROOT}/../compiler/latest/lib" "${MKL_ROOT}/lib"
        NO_DEFAULT_PATH)
      if(NOT IOMP5_LIB)
        find_library(
          IOMP5_LIB
          NAMES libiomp5md)
      endif()
      if(IOMP5_LIB)
        list(APPEND MKL_LIBRARIES ${IOMP5_LIB})
      endif()
    endif()
    message(STATUS "Found MKL: ${MKL_RT_LIB}")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
      message(FATAL_ERROR "MKL requires 64-bit compilation. Use a 64-bit generator.")
    endif()
  else()
    set(MKL_FOUND FALSE)
    message(WARNING "MKL not found. Building without MKL acceleration.")
    set(QEP_USE_MKL OFF)
  endif()
else()
  set(MKL_FOUND FALSE)
  message(STATUS "MKL disabled by user.")
endif()

# ========== 公共编译选项和依赖（接口库） ==========
add_library(qep_common INTERFACE)

target_include_directories(
  qep_common
  INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${NLOHMANN_INCLUDE_DIR}>)

target_link_libraries(qep_common INTERFACE Eigen3::Eigen Spectra::Spectra)

target_compile_definitions(
  qep_common INTERFACE QEP_PROBLEM_BASE_PATH="${QEP_PROBLEM_BASE_PATH}")

target_compile_options(
  qep_common
  INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/utf-8
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:Debug>:/Od
            /Zi
            /RTC1>>
            $<$<CXX_COMPILER_ID:GNU,Clang>:$<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Debug>:-O1
            -g
            -Wall
            -Wextra>>)

if(QEP_USE_AVX2)
  if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(AMD64)|(i.86)")
    message(WARNING "AVX2 requires x86-64 CPU (detected: ${CMAKE_SYSTEM_PROCESSOR}). Disabling AVX2.")
    set(QEP_USE_AVX2 OFF)
  else()
    target_compile_options(
      qep_common INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>
                           $<$<CXX_COMPILER_ID:GNU,Clang>:-mavx2>)
  endif()
endif()

if(QEP_USE_OPENMP)
  if(MSVC)
    target_compile_options(qep_common INTERFACE /openmp)
  else()
    find_package(OpenMP QUIET)
    if(OpenMP_CXX_FOUND)
      target_compile_options(qep_common INTERFACE ${OpenMP_CXX_FLAGS})
      target_link_libraries(qep_common INTERFACE OpenMP::OpenMP_CXX)
      message(STATUS "OpenMP enabled: ${OpenMP_CXX_FLAGS}")
    else()
      message(WARNING "OpenMP not found. Disabling OpenMP.")
    endif()
  endif()
endif()

if(MKL_FOUND)
  target_compile_definitions(qep_common INTERFACE EIGEN_USE_MKL_ALL
                                                  EIGEN_PARDISO_SUPPORT)
  target_include_directories(qep_common INTERFACE ${MKL_INCLUDE_DIR})
  target_link_libraries(qep_common INTERFACE ${MKL_LIBRARIES})
  message(STATUS "Eigen MKL+PARDISO support enabled (LP64 mode)")
else()
  message(STATUS "Eigen MKL+PARDISO support disabled (no MKL)")
endif()

# ========== 核心库 ==========
add_subdirectory(src)

# ========== 可选子目录 ==========
if(QEP_BUILD_CLI)
  add_subdirectory(src/cli)
endif()
if(QEP_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
if(QEP_BUILD_GUI)
  find_package(Qt6 QUIET COMPONENTS Widgets)
  if(NOT Qt6_FOUND)
    find_package(Qt5 QUIET COMPONENTS Widgets)
    if(NOT Qt5_FOUND)
      message(FATAL_ERROR "Qt5/Qt6 not found. Install Qt or disable GUI: -DQEP_BUILD_GUI=OFF")
    endif()
    set(QT_VERSION_MAJOR 5)
  else()
    set(QT_VERSION_MAJOR 6)
  endif()
  message(STATUS "Qt${QT_VERSION_MAJOR} found, building GUI")
  add_subdirectory(src/gui)
endif()

# ========== 安装规则 ==========
if(QEP_INSTALL)
  include(GNUInstallDirs)
  install(
    TARGETS QEP qep_common
    EXPORT QEPTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(DIRECTORY include/qep include/config
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(
    EXPORT QEPTargets
    FILE QEPTargets.cmake
    NAMESPACE QEP::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QEP)
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/QEPConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/QEPConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QEP)
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/QEPConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/QEPConfig.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/QEPConfigVersion.cmake
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QEP)
endif()
