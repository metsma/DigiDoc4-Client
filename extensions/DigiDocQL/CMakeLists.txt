set(RESOURCES ../../client/mac/Resources/asic.icns)
add_custom_command(OUTPUT PreviewViewController.nib
    COMMAND ibtool --errors --warnings --notices --output-format human-readable-text
        --compile PreviewViewController.nib ${CMAKE_CURRENT_SOURCE_DIR}/Base.lproj/PreviewViewController.xib
)
set_source_files_properties(PreviewViewController.nib PROPERTIES MACOSX_PACKAGE_LOCATION Resources/Base.lproj)
add_executable(DigiDocQL MACOSX_BUNDLE
    ${RESOURCES}
    PreviewViewController.nib
    PreviewViewController.mm
)
set_target_properties(DigiDocQL PROPERTIES
    BUNDLE YES
    BUNDLE_EXTENSION appex
    RESOURCE ${RESOURCES}
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.cmake
    AUTOMOC OFF
    INCLUDE_DIRECTORIES ${LIBDIGIDOCPP_INCLUDE_DIR}
    COMPILE_FLAGS "-Wno-unused-parameter -fobjc-arc"
    LINK_LIBRARIES "-framework digidocpp;-framework AppKit;-framework WebKit;-framework Quartz;-framework UniformTypeIdentifiers"
    LINK_FLAGS "-F/Library/Frameworks -fobjc-arc -fobjc-link-runtime -e _NSExtensionMain -fapplication-extension"
    BUILD_WITH_INSTALL_RPATH YES
    INSTALL_RPATH "@loader_path/../../../../Frameworks"
    XCODE_ATTRIBUTE_WRAPPER_EXTENSION appex
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
)
target_link_libraries(DigiDocQL digidocpp::digidocpp)
