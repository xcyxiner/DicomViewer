# 安装可执行文件
install(
    TARGETS DicomViewer_exe
    RUNTIME DESTINATION bin
)

# 安装 DCMTK 字典文件
if(UNIX)
    # 使用 CMakeLists.txt 中已找到的 DCM_DICT_PATH
    if(DCM_DICT_PATH)
        install(FILES "${DCM_DICT_PATH}"
            DESTINATION share/dcmtk
        )
        message(STATUS "Will install dicom.dic from: ${DCM_DICT_PATH}")
    endif()
endif()

if(UNIX)
    # 获取插件目录路径
    set(_triplet "${VCPKG_TARGET_TRIPLET}")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(QT_PLUGINS_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${_triplet}/debug/Qt6/plugins")
    else()
        set(QT_PLUGINS_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${_triplet}/Qt6/plugins")
    endif()

    #直接安装,并添加所用插件的依赖
    qt_generate_deploy_script(
        TARGET DicomViewer_exe
        OUTPUT_SCRIPT deploy_script
        CONTENT "
        qt_deploy_runtime_dependencies(
            EXECUTABLE $<TARGET_FILE:DicomViewer_exe>
            GENERATE_QT_CONF
            ADDITIONAL_MODULES
                \"${QT_PLUGINS_DIR}/platforms/libqxcb.so\"
                \"${QT_PLUGINS_DIR}/platforms/libqwayland-generic.so\"
                \"${QT_PLUGINS_DIR}/platforms/libqwayland-egl.so\"
            VERBOSE
        )
    ")

    # 安装字体文件到 lib 目录下
    install(DIRECTORY "${QT_FONT_DEST}"  # 注意这里改成了标准变量
        DESTINATION lib
        FILES_MATCHING PATTERN "*.ttf"
    )

    # 拷贝所有插件到安装目录的 plugins 目录下
    install(DIRECTORY "${QT_PLUGINS_DIR}/"
            DESTINATION plugins
            FILES_MATCHING PATTERN "*.so*")

    # 修正所有已安装插件的 RPATH,提示需要安装 patchelf 工具
    message(STATUS "Fixing RPATH for installed plugins. Ensure 'patchelf' is installed on your system.")
    install(CODE "
        file(GLOB_RECURSE plugin_files \"\${CMAKE_INSTALL_PREFIX}/plugins/*.so*\")
        foreach(plugin \${plugin_files})
            message(STATUS \"Fixing RPATH for \${plugin}\")
            execute_process(COMMAND patchelf --set-rpath \"\$ORIGIN/../../lib\" \"\${plugin}\"
                            RESULT_VARIABLE result)
            if(NOT result EQUAL 0)
                message(WARNING \"Failed to set RPATH for \${plugin}\")
            endif()
        endforeach()
    ")
else()
    # Windows 平台下，使用 qt_generate_deploy_app_script 来生成部署脚本
    qt_generate_deploy_app_script(
        TARGET DicomViewer_exe              # 指定主目标
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    # 第三方依赖直接拷贝到安装目录的 bin 目录下
    install(DIRECTORY ${CMAKE_BINARY_DIR}/Release/
            DESTINATION bin
            FILES_MATCHING PATTERN "*.dll"
    )
    message(STATUS "Installed Windows DLLs from ${CMAKE_BINARY_DIR}/Release/")
endif()

install(SCRIPT ${deploy_script})
message(STATUS "Deploy script will be generated at: ${deploy_script}") 

if(PROJECT_IS_TOP_LEVEL)
    include(CPack)
endif()
