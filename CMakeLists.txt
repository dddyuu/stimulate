cmake_minimum_required(VERSION 3.10.0)
project(SleepSystem LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_definitions(-pthread) # 使能线程支持
add_definitions(-exceptions) # 使能异常支持
add_definitions(-frtti) # 使能RTTI支持

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(Qt5_DIR "D:/Qt/Qt5.14.2/5.14.2/msvc2017_64/lib/cmake/Qt5")

find_package(Qt5 COMPONENTS Core Widgets Concurrent SerialPort Network Bluetooth REQUIRED)


if(MSVC)
    add_compile_options("/source-charset:utf-8")
    add_compile_options("/execution-charset:utf-8")
endif()

add_subdirectory(BCIIA)

add_subdirectory(SelfAmplifer)
file(GLOB src "src/*")
file(GLOB res "res/*.qrc")
file(COPY ${MATDLL} DESTINATION ".")
file(COPY ${onnxruntime_DLL} DESTINATION .)
file(GLOB ui_files 
    "src/*.ui"  # 包含所有的UI文件
)
add_executable(SleepSystem   main.cpp ${src} ${res}
    ${bcimonitor_res} ${AmpliferRes} 
     ${ui_files})
target_link_libraries(SleepSystem PRIVATE
    Qt5::Widgets
    Qt5::Core
    Qt5::Concurrent
    Qt5::SerialPort
    Qt5::Network
    Qt5::Bluetooth
    BCIIA )

target_include_directories(SleepSystem PUBLIC "src")

