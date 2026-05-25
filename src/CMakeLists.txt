set(QEP_SOURCES
    qep/core/UnifiedSolver.cpp
    qep/io/MatrixIO.cpp
    qep/utils/ConfigManager.cpp
    qep/utils/CreateProblem.cpp
    qep/utils/MatrixProperties.cpp
    qep/utils/ResidualValidation.cpp
)

add_library(QEP STATIC ${QEP_SOURCES})
target_link_libraries(QEP PUBLIC qep_common)
target_include_directories(
  QEP
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/qep> # 允许内部包含如
                                                     # config/GlobalConfig.h
)

# 设置输出目录（可选）
set_target_properties(
  QEP PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
                 LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
