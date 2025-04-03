set(RESOURCES ../../client/mac/Resources/asic.icns)
add_executable(DigiDocQL MACOSX_BUNDLE
    ${RESOURCES}
    PreviewViewController.mm
)
set_target_properties(DigiDocQL PROPERTIES
    BUNDLE YES
    BUNDLE_EXTENSION appex
    RESOURCE ${RESOURCES}
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.cmake
    AUTOMOC OFF
    COMPILE_FLAGS "-Wno-unused-parameter -fobjc-arc"
    LINK_LIBRARIES "-framework digidocpp;-framework QuickLookUI;-framework UniformTypeIdentifiers"
    LINK_FLAGS "-F/Library/Frameworks -fobjc-arc -fobjc-link-runtime -e _NSExtensionMain -fapplication-extension"
    BUILD_WITH_INSTALL_RPATH YES
    INSTALL_RPATH "@loader_path/../../../Frameworks"
    XCODE_ATTRIBUTE_WRAPPER_EXTENSION appex
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
)
target_link_libraries(DigiDocQL digidocpp::digidocpp)
