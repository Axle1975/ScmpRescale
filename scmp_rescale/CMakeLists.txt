cmake_policy(SET CMP0020 NEW)

# find the QtWidgets library
find_package(Qt5Core)
if( NOT ${Qt5Core_FOUND} )
    message( FATAL_ERROR "Can't find system install of Qt at ${CMAKE_PREFIX_PATH}, please follow docs to install." )
endif()

find_package(Qt5Widgets REQUIRED)

# add the required Qt source includes to the cmake path
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Qt5Widgets_INCLUDE_DIRS}
    )


file(GLOB source_files *.cpp *.h)


qt5_wrap_cpp(MOC_OUTFILES
    scmp_rescale_window.h
)

qt5_wrap_ui( UI_HEADERS
    scmp_rescale_window.ui
)


add_executable (scmp_rescale
    ${source_files}
    ${MOC_OUTFILES}
    ${UI_HEADERS}
    )

target_link_libraries (scmp_rescale LINK_PUBLIC 
    scmp
    nfa_gl
    ${Qt5Core_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    )

SET(plugin_dest_dir bin)
set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/scmp_rescale.exe")

install(
    TARGETS scmp_rescale
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
    )

INSTALL(CODE "
    file(GLOB_RECURSE QTPLUGINS
      \"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
    include(BundleUtilities)
    fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DIRS}\")
    " COMPONENT Runtime)

SET(DIRS ${QT_LIBRARY_DIRS})

INSTALL(CODE "
    file(GLOB_RECURSE QTPLUGINS
      \"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
    include(BundleUtilities)
    fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DIRS}\")
    " COMPONENT Runtime)

set(CPACK_GENERATOR NSIS)
set(CPACK_PACKAGE_NAME "ScmpRescale")
set(CPACK_NSIS_DISPLAY_NAME "ScmpRescale")
set(CPACK_PACKAGE_EXECUTABLES scmp_rescale;ScmpRescale)
set(CPACK_PACKAGE_VENDOR "Axle")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SCMP Rescale - a tool to rescale / import Supreme Commander .scmap map files")
set(CPACK_PACKAGE_VERSION "1.2.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ScmpRescale")
SET(CPACK_NSIS_MODIFY_PATH ON)

INCLUDE(CPack)

