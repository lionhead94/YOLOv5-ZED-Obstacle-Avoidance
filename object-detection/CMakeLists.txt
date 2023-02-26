cmake_minimum_required(VERSION 3.10)

project(yolov5_7.0-ZED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(CUDA_USE_STATIC_CUDA_RUNTIME OFF)
SET(SPECIAL_OS_LIBS "pthread" "X11")
add_definitions(-Wno-write-strings)
add_definitions(-DAPI_EXPORTS)

option(LINK_SHARED_ZED "Link with the ZED SDK shared executable" ON)
if (NOT LINK_SHARED_ZED AND MSVC)
    message(FATAL_ERROR "LINK_SHARED_ZED OFF : ZED SDK static libraries not available on Windows")
endif()

find_package(ZED 3 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(CUDA REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${ZED_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})
include_directories(${GLUT_INCLUDE_DIR})
include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

link_directories(${ZED_LIBRARY_DIR})
link_directories(${GLEW_LIBRARY_DIRS})
link_directories(${GLUT_LIBRARY_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
link_directories(${CUDA_LIBRARY_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})

# cuda
include_directories(/usr/local/cuda/include)
link_directories(/usr/local/cuda/lib64)

# tensorrt
include_directories(/home/nvidia/TensorRT-8.2.5.1/include/)
link_directories(/home/nvidia/TensorRT-8.2.5.1/lib/)

FILE(GLOB_RECURSE SRC_FILES src/*.c* ${PROJECT_SOURCE_DIR}/src/*.cu)
FILE(GLOB_RECURSE HDR_FILES include/*.h*)
# cuda_add_executable(${PROJECT_NAME} ${HDR_FILES} ${SRC_FILES})
add_definitions(-O3 -D_MWAITXINTRIN_H_INCLUDED -Wno-deprecated-declarations -lglut -lGL)

if (LINK_SHARED_ZED)
    SET(ZED_LIBS ${ZED_LIBRARIES} ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY})
else()
    SET(ZED_LIBS ${ZED_STATIC_LIBRARIES} ${CUDA_CUDA_LIBRARY} ${CUDA_LIBRARY})
endif()

SET(TRT_LIBS nvinfer cudart)

cuda_add_executable(yolov5_ser yolov5_ser.cpp ${SRC_FILES})
target_link_libraries(yolov5_ser
                        ${TRT_LIBS}
                        ${SPECIAL_OS_LIBS}
                        ${ZED_LIBS}
                        ${OPENGL_LIBRARIES}
                        ${GLUT_LIBRARY}
                        ${GLEW_LIBRARIES}
                        ${OpenCV_LIBRARIES}
                        OpenGL::GL
                        GLUT::GLUT)

cuda_add_executable(yolov5_det yolov5_det.cpp ${SRC_FILES})
target_link_libraries(yolov5_det
                        ${TRT_LIBS}
                        ${SPECIAL_OS_LIBS}
                        ${ZED_LIBS}
                        ${OPENGL_LIBRARIES}
                        ${GLUT_LIBRARY}
                        ${GLEW_LIBRARIES}
                        ${OpenCV_LIBRARIES}
                        OpenGL::GL
                        GLUT::GLUT)