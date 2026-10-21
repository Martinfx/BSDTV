find_package(Qt6 REQUIRED COMPONENTS Test)

function(bsdtv_test name)
    qt_add_executable(${name} ${name}.cpp testutil.h uitest.h fakejellyfin.h)
    target_link_libraries(${name} PRIVATE bsdtvcore bsdtvcoreplugin Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    # Headless: no display needed.
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LC_ALL=C.UTF-8")
endfunction()

bsdtv_test(tst_config)
bsdtv_test(tst_apps)
bsdtv_test(tst_systemtools)
bsdtv_test(tst_cec)
bsdtv_test(tst_remote)
bsdtv_test(tst_jellyfin)
bsdtv_test(tst_ui)
bsdtv_test(tst_media)
