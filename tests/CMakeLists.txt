find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

function(qtsm_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE qtsm_core Qt::Test)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

qtsm_add_test(tst_parser)
qtsm_add_test(tst_logger)
qtsm_add_test(tst_filereader)
qtsm_add_test(tst_utf8decoder)
qtsm_add_test(tst_receivepipeline)

# Integration test driving the real main window.
add_executable(tst_mainwindow tst_mainwindow.cpp)
target_link_libraries(tst_mainwindow PRIVATE qtsm_gui Qt::Test)
add_test(NAME tst_mainwindow COMMAND tst_mainwindow)
set_tests_properties(tst_mainwindow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
