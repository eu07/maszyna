# - Try to find libsndfile

# Use pkg-config to get hints about paths
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBSNDFILE_PKGCONF sndfile)
endif(PKG_CONFIG_FOUND)

# Include dir
find_path(LIBSNDFILE_INCLUDE_DIR
	NAMES sndfile.h
	PATHS ${LIBSNDFILE_PKGCONF_INCLUDE_DIRS}
)

# Library
find_library(LIBSNDFILE_LIBRARY
	NAMES sndfile libsndfile-1
	PATHS ${LIBSNDFILE_PKGCONF_LIBRARY_DIRS}
)

find_package(PackageHandleStandardArgs)
find_package_handle_standard_args(SndFile  DEFAULT_MSG  LIBSNDFILE_LIBRARY LIBSNDFILE_INCLUDE_DIR)

if(SNDFILE_FOUND)
      add_library(SndFile::sndfile INTERFACE IMPORTED GLOBAL)
      set_target_properties(SndFile::sndfile PROPERTIES
        INTERFACE_LINK_LIBRARIES ${LIBSNDFILE_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${LIBSNDFILE_INCLUDE_DIR}
      )
endif(SNDFILE_FOUND)
