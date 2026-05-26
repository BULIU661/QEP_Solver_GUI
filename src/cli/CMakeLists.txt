#==============================================================================
#  src/cli/CMakeLists.txt  —  CLI 可执行文件构建规则
#==============================================================================

add_executable(qep main.cpp TestHarness.cpp)
# 链接核心库
target_link_libraries(qep PRIVATE QEP)
# 如果测试代码需要访问私有头文件（如 GlobalConfig.h），可以添加包含路径 但更好的做法是测试代码只通过公共接口测试。若确实需要，可以取消注释：
# target_include_directories(qep PRIVATE ${PROJECT_SOURCE_DIR}/src) 设置输出目录
set_target_properties(qep PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                          ${PROJECT_SOURCE_DIR}/bin)
