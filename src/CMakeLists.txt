# 收集所有源文件（排除 test 目录，因为 test 是单独子目录）
file(GLOB_RECURSE QEP_SOURCES CONFIGURE_DEPENDS "qep/*.cpp")
# 如果有任何不希望包含的文件，可以在这里过滤 例如：list(FILTER QEP_SOURCES EXCLUDE REGEX ".*/test/.*")

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
