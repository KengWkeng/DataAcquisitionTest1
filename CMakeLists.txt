cmake_minimum_required(VERSION 3.16)

project(DataAcquisitionTest1 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets PrintSupport SerialPort SerialBus)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets PrintSupport SerialPort SerialBus)


set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        Core/Constants.h
        Core/DataTypes.h
        Config/ConfigManager.h
        Config/ConfigManager.cpp
        Device/AbstractDevice.h
        Device/AbstractDevice.cpp
        Device/VirtualDevice.h
        Device/VirtualDevice.cpp
        Device/ModbusDevice.h
        Device/ModbusDevice.cpp
        Device/DAQDevice.h
        Device/DAQDevice.cpp
        Device/ECUDevice.h
        Device/ECUDevice.cpp
        Device/DeviceManager.h
        Device/DeviceManager.cpp
        Processing/Channel.h
        Processing/Channel.cpp
        Processing/DataProcessor.h
        Processing/DataProcessor.cpp
        Processing/DataStorage.h
        Processing/DataStorage.cpp
        Processing/SecondaryInstrument.h
        Processing/SecondaryInstrument.cpp
        plot/qcustomplot.h
        plot/qcustomplot.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(DataAcquisitionTest1
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET DataAcquisitionTest1 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(DataAcquisitionTest1 SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(DataAcquisitionTest1
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(DataAcquisitionTest1 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::SerialBus
    ${CMAKE_SOURCE_DIR}/lib/Art_DAQ.lib
)

# 添加包含目录
target_include_directories(DataAcquisitionTest1 PRIVATE ${CMAKE_SOURCE_DIR}/Include)

# 复制DAQ动态链接库到输出目录
add_custom_command(TARGET DataAcquisitionTest1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/lib/Art_DAQ.dll
    $<TARGET_FILE_DIR:DataAcquisitionTest1>)

# 如果是64位系统，复制64位版本的DLL
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_custom_command(TARGET DataAcquisitionTest1 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/lib/Art_DAQ_64.dll
        $<TARGET_FILE_DIR:DataAcquisitionTest1>)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.DataAcquisitionTest1)
endif()
set_target_properties(DataAcquisitionTest1 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS DataAcquisitionTest1
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(DataAcquisitionTest1)
endif()
