# ==============================================================================
# src/gui/CMakeLists.txt  —  GUI 可执行文件构建规则
# ==============================================================================

set(GUI_SOURCES
    main.cpp
    MainWindow.cpp
    panels/ConfigPanel.cpp
    panels/ProblemTreePanel.cpp
    panels/SolverPanel.cpp
    panels/OutputPanel.cpp
    workers/SolverWorker.cpp)

set(GUI_HEADERS
    MainWindow.h panels/ConfigPanel.h panels/ProblemTreePanel.h
    panels/SolverPanel.h panels/OutputPanel.h workers/SolverWorker.h)

add_executable(qep_gui ${GUI_SOURCES} ${GUI_HEADERS})

target_include_directories(qep_gui PRIVATE ${PROJECT_SOURCE_DIR}/include
                                           ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(qep_gui PRIVATE QEP Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(qep_gui PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                         ${PROJECT_SOURCE_DIR}/bin AUTOMOC ON)
