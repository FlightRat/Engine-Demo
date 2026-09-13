# Import the bundled Release library without rebuilding ReactPhysics3D.
set(CMAKE_IMPORT_FILE_VERSION 1)
set_property(TARGET ReactPhysics3D::ReactPhysics3D APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(ReactPhysics3D::ReactPhysics3D PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/Release/lib/reactphysics3d.lib"
)
list(APPEND _cmake_import_check_targets ReactPhysics3D::ReactPhysics3D)
list(APPEND _cmake_import_check_files_for_ReactPhysics3D::ReactPhysics3D "${_IMPORT_PREFIX}/Release/lib/reactphysics3d.lib")
set(CMAKE_IMPORT_FILE_VERSION)
