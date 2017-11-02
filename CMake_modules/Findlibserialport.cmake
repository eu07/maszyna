find_path(libserialport_INCLUDE_DIR libserialport.h)
find_library(libserialport_LIBRARY serialport)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libserialport DEFAULT_MSG libserialport_LIBRARY libserialport_INCLUDE_DIR)
if(libserialport_FOUND)
	set(libserialport_LIBRARIES ${libserialport_LIBRARY})
	set(libserialport_INCLUDE_DIRS ${libserialport_INCLUDE_DIR})
endif()

