cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(rs_gst_pub)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(realsense2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
find_package(PCL REQUIRED)


include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${realsense_INCLUDE_DIR}
    ${GST_INCLUDE_DIRS}
    src
)

link_directories(${OpenCV_LIBRARY_DIRS})

add_definitions(${OpenCV_DEFINITIONS})

add_executable(rs_gst_pub src/rs_gst_pub.cpp src/utils.hpp)
target_link_libraries(rs_gst_pub
    ${GST_LIBRARIES}
    ${OpenCV_LIBRARIES}
    ${realsense2_LIBRARY}
    gstreamer-1.0  # Add the GStreamer libraries here
    gstapp-1.0     # Add other GStreamer libraries if needed
    ${PCL_LIBRARIES}
)

add_executable(rs_gst_sub src/rs_gst_sub.cpp)
target_link_libraries(rs_gst_sub
    ${GST_LIBRARIES}
    ${OpenCV_LIBRARIES}
    gstreamer-1.0  # Add the GStreamer libraries here
    gstapp-1.0     # Add other GStreamer libraries if needed
    ${PCL_LIBRARIES}
)

add_executable(easyServerWebCam src/easyServerWebCam.cpp)
target_link_libraries(easyServerWebCam
    ${GST_LIBRARIES}
)

add_executable(easyClientWebCam src/easyClientWebCam.cpp)
target_link_libraries(easyClientWebCam
    ${GST_LIBRARIES}
)

add_executable(easyServerNvidiaWebCam src/easyServerNvidiaWebCam.cpp)
target_link_libraries(easyServerNvidiaWebCam
    ${GST_LIBRARIES}
)

add_executable(easyClientNvidiaWebCam src/easyClientNvidiaWebCam.cpp)
target_link_libraries(easyClientNvidiaWebCam
    ${GST_LIBRARIES}
)
