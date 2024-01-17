cmake_minimum_required(VERSION 3.16)
project(gst_rgbd_server)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)
find_package(spdlog REQUIRED)
find_package(realsense2 REQUIRED)

include(FindPkgConfig)
pkg_check_modules(GST    REQUIRED gstreamer-1.0)
pkg_check_modules(GSTAPP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GSTRTSP REQUIRED gstreamer-rtsp-server-1.0)  # Added this line
pkg_check_modules(GLIB   REQUIRED glib-2.0)
pkg_check_modules(GFLAGS REQUIRED gflags)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

set(DeepStream_ROOT "/opt/nvidia/deepstream/deepstream-6.4")
set(DeepStream_INCLUDE_DIRS "${DeepStream_ROOT}/sources/includes")
set(DeepStream_LIBRARY_DIRS "${DeepStream_ROOT}/lib")

message(STATUS "GST:   ${GST_INCLUDE_DIRS},${GST_LIBRARY_DIRS},${GST_LIBRARIES}")
message(STATUS "GSTAPP:${GSTAPP_INCLUDE_DIRS},${GSTAPP_LIBRARY_DIRS},${GSTAPP_LIBRARIES}")
message(STATUS "GLIB:  ${GLIB_INCLUDE_DIRS},${GLIB_LIBRARY_DIRS},${GLIB_LIBRARIES}")
message(STATUS "JSON:  ${JSON_INCLUDE_DIRS},${JSON_LIBRARY_DIRS},${JSON_LIBRARIES}")
message(STATUS "GFLAGS:${GFLAGS_INCLUDE_DIRS},${GFLAGS_LIBRARY_DIRS},${GFLAGS_LIBRARIES}")
message(STATUS "OpenCV:${OpenCV_INCLUDE_DIRS},${OpenCV_LIBRARY_DIRS},${OpenCV_LIBRARIES}")
message(STATUS "DeepStream: ${DeepStream_INCLUDE_DIRS}, ${DeepStream_LIBRARY_DIRS}")

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${GST_INCLUDE_DIRS}
    ${GSTAPP_INCLUDE_DIRS}
    ${GSTRTSP_INCLUDE_DIRS}  # Added this line
    ${GLIB_INCLUDE_DIRS}
    ${GFLAGS_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${spdlog_INCLUDE_DIRS}
    ${DeepStream_INCLUDE_DIRS}
    ${realsense_INCLUDE_DIR}
)




link_directories(
    ${GST_LIBRARY_DIRS}
    ${GSTAPP_LIBRARY_DIRS}
    ${GSTRTSP_LIBRARY_DIRS}  # Added this line
    ${GLIB_LIBRARY_DIRS}
    ${GFLAGS_LIBRARY_DIRS}
    ${JSONCPP_LIBRARY_DIRS}
    ${OpenCV_LIBRARY_DIRS}
    ${spdlog_LIBRARY_DIRS}
    ${DeepStream_LIBRARY_DIRS}
)

# Config Logger (unchanged)

add_executable(${PROJECT_NAME}
    src/gst_rgbd_server.cc
    src/main.cc
)

target_link_libraries(${PROJECT_NAME}
    ${GST_LIBRARIES}
    ${GSTAPP_LIBRARIES}
    ${GSTRTSP_LIBRARIES}  # Added this line
    ${GLIB_LIBRARIES}
    ${GFLAGS_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${OpenCV_LIBRARIES}
    ${realsense2_LIBRARY}
    nvbufsurface
    nvdsgst_meta
    nvds_meta
    nvds_utils
)

