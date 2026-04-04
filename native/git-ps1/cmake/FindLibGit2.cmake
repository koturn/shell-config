# Use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
find_package(PkgConfig QUIET)
pkg_check_modules(PKG_GIT2 QUIET libgit2)

set(LIBGIT2_DEFINITIONS ${PKG_GIT2_CFLAGS_OTHER})

find_path(LIBGIT2_INCLUDE_DIR
  NAMES
    git2.h
  HINTS
    ${PKG_GIT2_INCLUDE_DIRS}
)
find_library(LIBGIT2_LIBRARY
  NAMES
    git2
  HINTS
    ${PKG_GIT2_LIBRARY_DIRS}
)

# get version from header, should work on windows, too
if(LIBGIT2_INCLUDE_DIR)
  file(STRINGS "${LIBGIT2_INCLUDE_DIR}/git2/version.h" LIBGIT2_H REGEX "^#define LIBGIT2_VERSION +\"[^\"]*\"$")

  string(REGEX REPLACE "^.*LIBGIT2_VERSION +\"([0-9]+).*$" "\\1" LIBGIT2_VERSION_MAJOR "${LIBGIT2_H}")
  string(REGEX REPLACE "^.*LIBGIT2_VERSION +\"[0-9]+\\.([0-9]+).*$" "\\1" LIBGIT2_VERSION_MINOR  "${LIBGIT2_H}")
  string(REGEX REPLACE "^.*LIBGIT2_VERSION +\"[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" LIBGIT2_VERSION_PATCH "${LIBGIT2_H}")
  set(LIBGIT2_VERSION "${LIBGIT2_VERSION_MAJOR}.${LIBGIT2_VERSION_MINOR}.${LIBGIT2_VERSION_PATCH}")

  set(LIBGIT2_MAJOR_VERSION "${LIBGIT2_VERSION_MAJOR}")
  set(LIBGIT2_MINOR_VERSION "${LIBGIT2_VERSION_MINOR}")
  set(LIBGIT2_PATCH_VERSION "${LIBGIT2_VERSION_PATCH}")

  unset(LIBGIT2_H)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibGit2
  FOUND_VAR
    LIBGIT2_FOUND
  REQUIRED_VARS
    LIBGIT2_LIBRARY
    LIBGIT2_INCLUDE_DIR
  VERSION_VAR
    LIBGIT2_VERSION
)

if(LIBGIT2_FOUND AND NOT TARGET LibGit2::LibGit2)
  add_library(LibGit2::LibGit2 UNKNOWN IMPORTED)
  set_target_properties(LibGit2::LibGit2 PROPERTIES
    IMPORTED_LOCATION "${LIBGIT2_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${LIBGIT2_DEFINITIONS}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBGIT2_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(LIBGIT2_LIBRARY LIBGIT2_INCLUDE_DIR)

set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARY})
set(LIBGIT2_INCLUDE_DIRS ${LIBGIT2_INCLUDE_DIR})

include(FeatureSummary)
set_package_properties(LibGit2 PROPERTIES
  URL "https://libgit2.github.com/"
  DESCRIPTION "A plain C library to interface with the git version control system."
)
