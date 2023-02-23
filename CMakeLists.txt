cmake_minimum_required(VERSION 3.5)
project (cppgtfs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

find_package(LibZip)
# lib zip is usually stated as found using LIBZIP_FOUND or on newer versions LibZip_FOUND
if (LIBZIP_FOUND OR LibZip_FOUND)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLIBZIP_FOUND=1")
endif()


file(GLOB_RECURSE ad_cppgtfs_SOURCES *.cpp)
include_directories(
	SYSTEM ${LIBZIP_INCLUDE_DIR}
	SYSTEM ${LIBZIP_CONF_INCLUDE_DIR}
)


add_library(ad_cppgtfs ${ad_cppgtfs_SOURCES})
target_compile_features(ad_cppgtfs PUBLIC cxx_std_11)
target_include_directories(ad_cppgtfs PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

