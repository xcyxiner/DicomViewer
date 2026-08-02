install(
    TARGETS DicomViewer_exe
    RUNTIME_DEPENDENCY_SET runtime_deps
    RUNTIME DESTINATION bin
)

install(RUNTIME_DEPENDENCY_SET runtime_deps
    DESTINATION bin
    COMPONENT DicomViewer_Runtime   # 可选：和 exe 归到同一个 component
)
# 在已有的 install() 规则后面加
if(AUTO_DEPLOY_FONTS AND UNIX)
    # 安装字体到 prefix/bin/fonts/
    install(DIRECTORY "${QT_FONT_DEST}"
        DESTINATION bin/fonts
        FILES_MATCHING PATTERN "*.ttf"
    )
endif()

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
