QT += core gui widgets

CONFIG += c++14

TARGET = UsbTool
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
	main.cpp \
	MainWindow.cpp \
	UsbThread.cpp \
	DeviceListModel.cpp \
	DeviceInterfacesModel.cpp \
	util/HighResClock.cpp \
	usb/EndpointInfo.cpp \
	usb/IsochronousStream.cpp \
	usb/mac/RunLoop.cpp \
    usb/windows/Util_Win.cpp \
    usb/mac/Util_Mac.cpp \
    usb/mac/TypeWrappers_Mac.cpp \
    usb/windows/TypeWrappers_Win.cpp \
    usb/DeviceId.cpp \
    usb/Descriptors.cpp \
    usb/Device.cpp \
    usb/mac/Device_Mac.cpp \
    usb/windows/Device_Win.cpp \
    usb/mac/Discovery_Mac.cpp \
    usb/windows/Discovery_Win.cpp

HEADERS += \
	MainWindow.h \
	UsbThread.h \
	DeviceListModel.h \
	Metatypes.h \
	PaddedSpinBox.h \
	DeviceInterfacesModel.h \
	util/EnumCasts.h \
	util/HighResClock.h \
	util/Result.h \
	util/scope_exit.h \
	usb/EndpointInfo.h \
	usb/IsochronousStream.h \
	usb/UsbSpecification.h \
	usb/mac/RunLoop.h \
    usb/mac/TypeWrappers_Mac.h \
    usb/windows/TypeWrappers_Win.h \
    usb/mac/Util_Mac.h \
    usb/windows/Util_Win.h \
    usb/DeviceId.h \
    usb/Descriptors.h \
    usb/Device.h \
    usb/DeviceInfo.h \
    usb/Discovery.h \
    usb/mac/Device_Mac.h \
    usb/windows/Device_Win.h

FORMS += \
	MainWindow.ui

INCLUDEPATH += dependencies


mac:LIBS += -framework CoreFoundation
win32:LIBS += -lwinusb -lsetupapi

# Set icons
mac:ICON = UsbTool.icns
# Note this fails if the project is in a path with spaces. See https://bugreports.qt.io/browse/QTBUG-62918
# win32:RC_FILE = WindowsResources.rc

OTHER_FILES += \
	CMakeLists.txt \
	WindowsResources.rc \
	UsbTool.png \
	UsbTool.svg \
	UsbTool.ico \
	UsbTool.icns

RESOURCES += \
	Resources.qrc


