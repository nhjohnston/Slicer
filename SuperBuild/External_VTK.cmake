
set(proj VTK)

# Set dependency list
set(${proj}_DEPENDENCIES "zlib")
if (Slicer_USE_PYTHONQT)
  list(APPEND ${proj}_DEPENDENCIES python)
endif()
if(Slicer_USE_TBB)
  list(APPEND ${proj}_DEPENDENCIES tbb)
endif()

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(Slicer_USE_SYSTEM_${proj})
  unset(VTK_DIR CACHE)
  unset(VTK_SOURCE_DIR CACHE)
  find_package(VTK REQUIRED NO_MODULE)
endif()

# Sanity checks
if(DEFINED VTK_DIR AND NOT EXISTS ${VTK_DIR})
  message(FATAL_ERROR "VTK_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(DEFINED VTK_SOURCE_DIR AND NOT EXISTS ${VTK_SOURCE_DIR})
  message(FATAL_ERROR "VTK_SOURCE_DIR variable is defined but corresponds to nonexistent directory")
endif()


if((NOT DEFINED VTK_DIR OR NOT DEFINED VTK_SOURCE_DIR) AND NOT Slicer_USE_SYSTEM_${proj})

  set(EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS)
  set(EXTERNAL_PROJECT_OPTIONAL_VTK9_CMAKE_CACHE_ARGS)

  set(VTK_WRAP_PYTHON OFF)

  if(Slicer_USE_PYTHONQT)
    set(VTK_WRAP_PYTHON ON)
  endif()

  if(Slicer_USE_PYTHONQT)
    list(APPEND EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS
      -DPYTHON_EXECUTABLE:PATH=${PYTHON_EXECUTABLE}
      -DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIR}
      -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
      )
    list(APPEND EXTERNAL_PROJECT_OPTIONAL_VTK9_CMAKE_CACHE_ARGS
      -DPython3_ROOT_DIR:PATH=${Python3_ROOT_DIR}
      -DPython3_INCLUDE_DIR:PATH=${Python3_INCLUDE_DIR}
      -DPython3_LIBRARY:FILEPATH=${Python3_LIBRARY}
      -DPython3_LIBRARY_DEBUG:FILEPATH=${Python3_LIBRARY_DEBUG}
      -DPython3_LIBRARY_RELEASE:FILEPATH=${Python3_LIBRARY_RELEASE}
      -DPython3_EXECUTABLE:FILEPATH=${Python3_EXECUTABLE}
      )
  endif()

  # Markups module needs vtkFrenetSerretFrame, which is available in
  # SplineDrivenImageSlicer remote module.
  list(APPEND EXTERNAL_PROJECT_OPTIONAL_VTK9_CMAKE_CACHE_ARGS
    -DVTK_MODULE_ENABLE_VTK_SplineDrivenImageSlicer:BOOL=YES
    )

  list(APPEND EXTERNAL_PROJECT_OPTIONAL_VTK9_CMAKE_CACHE_ARGS
    -DVTK_MODULE_ENABLE_VTK_ChartsCore:STRING=YES
    -DVTK_MODULE_ENABLE_VTK_ViewsContext2D:STRING=YES
    -DVTK_MODULE_ENABLE_VTK_RenderingContext2D:STRING=YES
    -DVTK_MODULE_ENABLE_VTK_RenderingContextOpenGL2:STRING=YES
    )

  list(APPEND EXTERNAL_PROJECT_OPTIONAL_VTK9_CMAKE_CACHE_ARGS
    -DVTK_MODULE_ENABLE_VTK_GUISupportQt:STRING=YES
    -DVTK_GROUP_ENABLE_Qt:STRING=YES
    )

  list(APPEND EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS
    -DVTK_QT_VERSION:STRING=5
    -DVTK_Group_Qt:BOOL=ON
    -DQt5_DIR:FILEPATH=${Qt5_DIR}
    )

  if(Slicer_USE_TBB)
    list(APPEND EXTERNAL_PROJECT_OPTIONAL_VTK9_CMAKE_CACHE_ARGS
      -DTBB_DIR:PATH=${TBB_DIR}
      )
  endif()
  if(APPLE)
    list(APPEND EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS
      -DVTK_USE_CARBON:BOOL=OFF
      -DVTK_USE_COCOA:BOOL=ON # Default to Cocoa, VTK/CMakeLists.txt will enable Carbon and disable cocoa if needed
      -DVTK_USE_X:BOOL=OFF
      -DVTK_REQUIRED_OBJCXX_FLAGS:STRING=
      #-DVTK_USE_RPATH:BOOL=ON # Unused
      )
  endif()
  if(UNIX AND NOT APPLE)
    find_package(Fontconfig QUIET)
    if(Fontconfig_FOUND)
      list(APPEND EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS
        -DModule_vtkRenderingFreeTypeFontConfig:BOOL=ON
        )
    endif()

    # OpenGL_GL_PREFERENCE
    if(NOT "${OpenGL_GL_PREFERENCE}" MATCHES "^(LEGACY|GLVND)$")
      message(FATAL_ERROR "OpenGL_GL_PREFERENCE variable is expected to be set to LEGACY or GLVND")
    endif()
    list(APPEND EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS
      -DOpenGL_GL_PREFERENCE:STRING=${OpenGL_GL_PREFERENCE}
      )
  endif()

  # Disable Tk when Python wrapping is enabled
  if(Slicer_USE_PYTHONQT)
    list(APPEND EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS
      -DVTK_USE_TK:BOOL=OFF
      )
  endif()

  list(APPEND EXTERNAL_PROJECT_OPTIONAL_VTK9_CMAKE_CACHE_ARGS
    -DVTK_BUILD_TESTING:STRING=OFF
    -DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKm:BOOL=NO
    -DCMAKE_INSTALL_LIBDIR:STRING=lib # Force value to prevent lib64 from being used on Linux
    -DVTK_MODULE_ENABLE_VTK_GUISupportQtQuick:BOOL=NO
    )

  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_REPOSITORY
    "${EP_GIT_PROTOCOL}://github.com/slicer/VTK.git"
    QUIET
    )

  set(_git_tag)
  if("${Slicer_VTK_VERSION_MAJOR}.${Slicer_VTK_VERSION_MINOR}" STREQUAL "9.2")
    set(_git_tag "59ec450206012e86d4855bc669800499254bfc77") # slicer-v9.2.20230607-1ff325c54-2
    set(vtk_dist_info_version "9.2.20230607")
  elseif("${Slicer_VTK_VERSION_MAJOR}.${Slicer_VTK_VERSION_MINOR}" STREQUAL "9.4")
    set(_git_tag "454bb391dff78c6ff463298a5143ab5b4f0aa083") # slicer-v9.4.2-2025-03-26-13acb1a5d
    set(vtk_dist_info_version "9.4.2")
  elseif("${Slicer_VTK_VERSION_MAJOR}.${Slicer_VTK_VERSION_MINOR}" STREQUAL "9.5")
    set(_git_tag "c5657e3056c00db06084c7a8676ff5f0cd2063e3") # slicer-v9.5.0-2025-06-19-e70c856bd
    set(vtk_dist_info_version "9.5.0")
  else()
    message(FATAL_ERROR "error: Unsupported Slicer_VTK_VERSION_MAJOR.Slicer_VTK_VERSION_MINOR: ${Slicer_VTK_VERSION_MAJOR}.${Slicer_VTK_VERSION_MINOR}")
  endif()

  ExternalProject_SetIfNotDefined(
    Slicer_${proj}_GIT_TAG
    ${_git_tag}
    QUIET
    )

  set(EP_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${Slicer_${proj}_GIT_REPOSITORY}"
    GIT_TAG "${Slicer_${proj}_GIT_TAG}"
    SOURCE_DIR ${EP_SOURCE_DIR}
    BINARY_DIR ${EP_BINARY_DIR}
    CMAKE_CACHE_ARGS
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${ep_common_c_flags}
      -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
      -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
      -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
      -DVTK_DEBUG_LEAKS:BOOL=${VTK_DEBUG_LEAKS}
      -DVTK_LEGACY_REMOVE:BOOL=ON
      #-DVTK_USE_RPATH:BOOL=ON # Unused
      -DVTK_WRAP_PYTHON:BOOL=${VTK_WRAP_PYTHON}
      -DVTK_INSTALL_RUNTIME_DIR:PATH=${Slicer_INSTALL_BIN_DIR}
      -DVTK_INSTALL_LIBRARY_DIR:PATH=${Slicer_INSTALL_LIB_DIR}
      -DVTK_INSTALL_ARCHIVE_DIR:PATH=${Slicer_INSTALL_LIB_DIR}
      -DVTK_Group_Qt:BOOL=ON
      -DVTK_USE_SYSTEM_ZLIB:BOOL=ON
      -DZLIB_ROOT:PATH=${ZLIB_ROOT}
      -DZLIB_INCLUDE_DIR:PATH=${ZLIB_INCLUDE_DIR}
      -DZLIB_LIBRARY:FILEPATH=${ZLIB_LIBRARY}
      -DVTK_ENABLE_KITS:BOOL=ON
      -DVTK_SMP_IMPLEMENTATION_TYPE:STRING=${Slicer_VTK_SMP_IMPLEMENTATION_TYPE}
      -DVTK_MODULE_ENABLE_VTK_IOEnSight:STRING=NO
      ${EXTERNAL_PROJECT_OPTIONAL_CMAKE_CACHE_ARGS}
      ${EXTERNAL_PROJECT_OPTIONAL_VTK${Slicer_VTK_VERSION_MAJOR}_CMAKE_CACHE_ARGS}
    INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )

  if(Slicer_USE_PYTHONQT AND NOT Slicer_USE_SYSTEM_python)
    # Create the vtk-*.dist-info directory to prevent pip from re-installing
    # vtk package as a wheel when listed as dependency in Slicer extension.
    # See https://packaging.python.org/en/latest/specifications/recording-installed-packages/
    set(_vtk_dist_info_metadata "${CMAKE_BINARY_DIR}/${proj}-dist-info-METADATA")
    file(CONFIGURE OUTPUT ${_vtk_dist_info_metadata}
      CONTENT [==[
Metadata-Version: 2.1
Name: vtk
Version: @vtk_dist_info_version@
]==]
      @ONLY
      )
    set(_vtk_dist_info_dir "${python_DIR}/${PYTHON_SITE_PACKAGES_SUBDIR}/vtk-${vtk_dist_info_version}.dist-info")
    set(_vtk_egg_info_dir "${python_DIR}/${PYTHON_SITE_PACKAGES_SUBDIR}/vtk-${vtk_dist_info_version}-py${Slicer_REQUIRED_PYTHON_VERSION_DOT}.egg-info")
    ExternalProject_Add_Step(${proj} create_dist_info
      # If any, remove existing "vtk-.*.egg-info" directory.
      # This will avoid issue when doing incremental build.
      COMMAND ${CMAKE_COMMAND} -E rm -rf ${_vtk_egg_info_dir}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${_vtk_dist_info_dir}
      COMMAND ${CMAKE_COMMAND} -E copy
        ${_vtk_dist_info_metadata}
        ${_vtk_dist_info_dir}/METADATA
      COMMENT "Creating '${_vtk_dist_info_dir}' directory"
      DEPENDEES build
      )
  endif()

  ExternalProject_GenerateProjectDescription_Step(${proj})

  set(VTK_DIR ${EP_BINARY_DIR})
  set(VTK_SOURCE_DIR ${EP_SOURCE_DIR})

  if(NOT DEFINED VTK_VALGRIND_SUPPRESSIONS_FILE)
    set(VTK_VALGRIND_SUPPRESSIONS_FILE ${EP_SOURCE_DIR}/CMake/VTKValgrindSuppressions.supp)
  endif()
  mark_as_superbuild(VTK_VALGRIND_SUPPRESSIONS_FILE:FILEPATH)

  #-----------------------------------------------------------------------------
  # Launcher setting specific to build tree

  # library paths
  set(_library_output_subdir bin)
  if(UNIX)
    set(_library_output_subdir lib)
  endif()
  set(${proj}_LIBRARY_PATHS_LAUNCHER_BUILD ${VTK_DIR}/${_library_output_subdir}/<CMAKE_CFG_INTDIR>)
  mark_as_superbuild(
    VARS ${proj}_LIBRARY_PATHS_LAUNCHER_BUILD
    LABELS "LIBRARY_PATHS_LAUNCHER_BUILD"
    )

  # pythonpath
    if(UNIX)
      set(${proj}_PYTHONPATH_LAUNCHER_BUILD
        ${VTK_DIR}/${_library_output_subdir}/python${Slicer_REQUIRED_PYTHON_VERSION_DOT}/site-packages
        )
    else()
      if(${vtk_dist_info_version} VERSION_GREATER_EQUAL 9.4)
        set(${proj}_PYTHONPATH_LAUNCHER_BUILD
          ${VTK_DIR}/lib/site-packages # Location for VTK 9.4+
          )
      else()
        set(${proj}_PYTHONPATH_LAUNCHER_BUILD
          ${VTK_DIR}/${_library_output_subdir}/Lib/site-packages # Location for VTK 9.3 or older
          )
      endif()
    endif()

  mark_as_superbuild(
    VARS ${proj}_PYTHONPATH_LAUNCHER_BUILD
    LABELS "PYTHONPATH_LAUNCHER_BUILD"
    )

  #-----------------------------------------------------------------------------
  # Launcher setting specific to install tree

  if(NOT APPLE)
    # This is not required for macOS where VTK python package is installed
    # in a standard location using CMake/SlicerBlockInstallExternalPythonModules.cmake

    # library paths
    if(UNIX)
      set(${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED
        <APPLAUNCHER_SETTINGS_DIR>/../lib
        )
    else()
      set(${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED
        <APPLAUNCHER_SETTINGS_DIR>/../bin
        )
    endif()
    mark_as_superbuild(
      VARS ${proj}_LIBRARY_PATHS_LAUNCHER_INSTALLED
      LABELS "LIBRARY_PATHS_LAUNCHER_INSTALLED"
      )

    # pythonpath
    set(_library_install_subdir "bin")
    if(UNIX)
      set(_library_install_subdir "lib")
    endif()
    if(UNIX)
      set(${proj}_PYTHONPATH_LAUNCHER_INSTALLED
        <APPLAUNCHER_SETTINGS_DIR>/../${_library_install_subdir}/python${Slicer_REQUIRED_PYTHON_VERSION_DOT}/site-packages
        )
    else()
      if(${vtk_dist_info_version} VERSION_GREATER_EQUAL 9.4)
        set(${proj}_PYTHONPATH_LAUNCHER_INSTALLED
          <APPLAUNCHER_SETTINGS_DIR>/../lib/site-packages # Location for VTK 9.4+
          )
      else()
        set(${proj}_PYTHONPATH_LAUNCHER_INSTALLED
          <APPLAUNCHER_SETTINGS_DIR>/../${_library_install_subdir}/Lib/site-packages # Location for VTK 9.3 or older
          )
      endif()
    endif()
    mark_as_superbuild(
      VARS ${proj}_PYTHONPATH_LAUNCHER_INSTALLED
      LABELS "PYTHONPATH_LAUNCHER_INSTALLED"
      )
  endif()

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

mark_as_superbuild(VTK_SOURCE_DIR:PATH)

mark_as_superbuild(
  VARS VTK_DIR:PATH
  LABELS "FIND_PACKAGE"
  )
