cmake_minimum_required(VERSION 3.15)
project(
  QEP
  VERSION 1.0.0
  LANGUAGES CXX)

# ========== 基础设置 ==========
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
  add_compile_options(/wd4244) # 消除 C4244 警告
endif()
add_compile_definitions(NOMINMAX)

# 默认构建类型为 Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 64 位检查（MKL 需要）
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(
    FATAL_ERROR "MKL requires 64-bit compilation. Use a 64-bit generator.")
endif()

# ========== 用户可配置选项 ==========
option(QEP_USE_MKL "Use Intel MKL for linear algebra" ON)
option(QEP_USE_AVX2 "Enable AVX2 instruction set" ON)
option(QEP_USE_OPENMP "Enable OpenMP parallelization" ON)
option(QEP_BUILD_TEST "Build test executable" ON) # 编译src/test（已完善）
option(QEP_BUILD_EXAMPLES "Build example programs" OFF) # 编译examples(还未完善)
option(QEP_INSTALL "Generate installation rules" ON)

# ========== 第三方依赖路径（可自定义） ==========
set(THIRD_PARTY_DIR
    ${PROJECT_SOURCE_DIR}/third_party
    CACHE PATH "Third party libraries directory")

message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message(STATUS "third_party path: ${THIRD_PARTY_DIR}")

# ---- Eigen3 ----
# 默认优先选用QEP内部third_party
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
# 创建接口目标以统一使用
add_library(Eigen3::Eigen INTERFACE IMPORTED)
set_target_properties(Eigen3::Eigen PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                               ${EIGEN_INCLUDE_DIR})

# ---- Spectra  ----
# 默认优先选用QEP内部third_party
find_path(
  SPECTRA_INCLUDE_DIR
  NAMES Spectra/GenEigsSolver.h
  PATHS ${THIRD_PARTY_DIR}/spectra ${THIRD_PARTY_DIR}/Spectra
  NO_DEFAULT_PATH
  DOC "Spectra include directory")
if(NOT SPECTRA_INCLUDE_DIR)
  find_path(
    SPECTRA_INCLUDE_DIR
    NAMES Spectra/GenEigsSolver.h
    NO_DEFAULT_PATH
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
  # 如果用户未指定，尝试常见路径
  if(NOT MKL_ROOT)
    if(WIN32)
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
    # 查找 OpenMP 运行时（MSVC 下需要 libiomp5md）
    if(MSVC)
      find_library(
        IOMP5_LIB
        NAMES libiomp5md
        PATHS "${MKL_ROOT}/../compiler/latest/lib" "${MKL_ROOT}/lib")
      if(IOMP5_LIB)
        list(APPEND MKL_LIBRARIES ${IOMP5_LIB})
      endif()
    endif()
    message(STATUS "Found MKL: ${MKL_RT_LIB}")
  else()
    set(MKL_FOUND FALSE)
    message(WARNING "MKL not found. Building without MKL acceleration.")
    set(QEP_USE_MKL OFF)
  endif()
endif()

# ========== 公共编译选项和依赖（接口库） ==========
add_library(qep_common INTERFACE)
target_include_directories(
  qep_common INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                       $<INSTALL_INTERFACE:include>)

# 链接 Eigen 和 Spectra
target_link_libraries(qep_common INTERFACE Eigen3::Eigen Spectra::Spectra)

# 编译选项
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
  target_compile_options(
    qep_common INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>
                         $<$<CXX_COMPILER_ID:GNU,Clang>:-march=native>)
endif()

if(QEP_USE_OPENMP) #
  target_compile_options(
    qep_common INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/openmp>
                         $<$<CXX_COMPILER_ID:GNU,Clang>:-fopenmp>)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL
                                             "Clang")
    find_package(OpenMP REQUIRED)
    target_link_libraries(qep_common INTERFACE OpenMP::OpenMP_CXX)
  elseif(MSVC AND MKL_FOUND)
    # MSVC 下 OpenMP 由 MKL 的 libiomp5md 提供，已经包含在 MKL_LIBRARIES 中
  endif()
endif()

if(MKL_FOUND)
  target_compile_definitions(qep_common INTERFACE EIGEN_USE_MKL_ALL
                                                  EIGEN_PARDISO_SUPPORT)
  target_include_directories(qep_common INTERFACE ${MKL_INCLUDE_DIR})
  target_link_libraries(qep_common INTERFACE ${MKL_LIBRARIES})
  message(STATUS "Eigen MKL+PARDISO support enabled (LP64 mode)")
endif()

# ========== 核心库 ==========
add_subdirectory(src)

# ========== 可选子目录 ==========
if(QEP_BUILD_TEST)
  add_subdirectory(src/test)
endif()
if(QEP_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# ========== 安装规则 ==========
if(QEP_INSTALL)
  include(GNUInstallDirs)

  # 安装库
  install(
    TARGETS QEP qep_common
    EXPORT QEPTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  # 安装公共头文件
  install(DIRECTORY include/qep include/config
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  # 导出 CMake 目标
  install(
    EXPORT QEPTargets
    FILE QEPTargets.cmake
    NAMESPACE QEP::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QEP)

  # 生成并安装 Config 文件
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
