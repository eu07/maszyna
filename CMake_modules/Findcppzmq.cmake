find_path(cppzmq_INCLUDE_DIR zmq.hpp)
find_library(cppzmq_LIBRARY zmq)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cppzmq DEFAULT_MSG cppzmq_LIBRARY cppzmq_INCLUDE_DIR)
if(cppzmq_FOUND)
	set(cppzmq_LIBRARIES ${cppzmq_LIBRARY})
	set(cppzmq_INCLUDE_DIRS ${cppzmq_INCLUDE_DIR})
endif()

