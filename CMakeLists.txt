cmake_minimum_required(VERSION 3.3)

project(UsbTool)

# Set the CMAKE_PREFIX_PATH environment variable to (for example) /Users/thutt/Qt/5.8/clang_64
# so it can find Qt5.
find_package(Qt5Widgets REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories (
	"dependencies"
)

set(src_files
	"main.cpp"
	"MainWindow.cpp"
	"UsbThread.cpp"
	"UsbLib.cpp"
	"DeviceListModel.cpp"
	"DeviceInterfacesModel.cpp"
)

set(form_files
	"MainWindow.ui"
)

set(resource_files
	"Resources.qrc"
)

# Windows icon.
if (WIN32)
	# Put this in the .rc:
	#
	# IDI_ICON1               ICON    DISCARDABLE     "UsbTool.ico"
	#
	# And to generate the .ico run:
	#
	#	mkdir UsbTool_ico
	#	sips -z 16 16     UsbTool.png --out UsbTool_ico/icon_16.png
	#	sips -z 32 32     UsbTool.png --out UsbTool_ico/icon_32.png
	#	sips -z 48 48     UsbTool.png --out UsbTool_ico/icon_48.png
	#	sips -z 256 256   UsbTool.png --out UsbTool_ico/icon_256.png
	#	convert UsbTool_ico/* UsbTool.ico
	# 
	list(APPEND src_files "WindowsResources.rc")
endif ()

# OSX Icon
if (APPLE)
	# To generate the .icns file, make a 1024x1024 PNG called UsbTool.png.
	#
	#	mkdir UsbTool.iconset
	#	sips -z 16 16     UsbTool.png --out UsbTool.iconset/icon_16x16.png
	#	sips -z 32 32     UsbTool.png --out UsbTool.iconset/icon_16x16@2x.png
	#	sips -z 32 32     UsbTool.png --out UsbTool.iconset/icon_32x32.png
	#	sips -z 64 64     UsbTool.png --out UsbTool.iconset/icon_32x32@2x.png
	#	sips -z 128 128   UsbTool.png --out UsbTool.iconset/icon_128x128.png
	#	sips -z 256 256   UsbTool.png --out UsbTool.iconset/icon_128x128@2x.png
	#	sips -z 256 256   UsbTool.png --out UsbTool.iconset/icon_256x256.png
	#	sips -z 512 512   UsbTool.png --out UsbTool.iconset/icon_256x256@2x.png
	#	sips -z 512 512   UsbTool.png --out UsbTool.iconset/icon_512x512.png
	#	sips -z 1024 1024 UsbTool.png --out UsbTool.iconset/icon_512x512@2x.png
	#	iconutil -c icns UsbTool.iconset
	#
	# This is the property added to Info.plist
	set(MACOSX_BUNDLE_ICON_FILE "UsbTool.icns")
	
	# Add the file to the source.
	set(osx_icon_file "${CMAKE_SOURCE_DIR}/UsbTool.icns")
	set_source_files_properties("${osx_icon_file}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	
	list(APPEND src_files "${osx_icon_file}")
endif ()

# Generate UI & Resource Files
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

# Process the form and resource files.
qt5_wrap_ui(form_generated_files ${form_files})
qt5_add_resources(resource_generated_files ${resource_files})

add_executable(UsbTool MACOSX_BUNDLE WIN32
	${src_files}
	${form_generated_files}
	${resource_generated_files}
)
target_link_libraries(UsbTool
	Qt5::Widgets
)

# Packaging
if (WIN32)
	# Get the location of libusb-1.0.dll
	get_filename_component(libusb_dlldir "${LIBUSB_LIBRARY}" DIRECTORY)
	
	add_custom_target(pack
		# Make a temporary directory for the zip.
		COMMAND "${CMAKE_COMMAND}" "-E" "make_directory" "${CMAKE_BINARY_DIR}/Pack"
		
		# Copy the EXE there.
		COMMAND "${CMAKE_COMMAND}" "-E" "copy" "$<TARGET_FILE:UsbTool>" "${CMAKE_BINARY_DIR}/Pack/UsbTool.exe"
		
		# Deploy Qt to that directory. All the options are to save space.
		COMMAND "${_qt5Core_install_prefix}/bin/windeployqt.exe" "--release" "--compiler-runtime"
			"--no-angle" "--no-opengl-sw" "--no-translations" "--no-system-d3d-compiler" "${CMAKE_BINARY_DIR}/Pack/UsbTool.exe"
		
		DEPENDS UsbTool
		COMMENT "Deploying Qt"
		VERBATIM
	)

	# Zip the result. This is a weird way of doing it but the cmake tar command isn't very flexible
	# and this is the only way to avoid getting a directory called 'Pack' in the zip.
	#
	# You can't glob at this point because the files aren't actually there yet.
	
	# We don't need the iconengines, imageformats, or Qt5SVG.dll despite what windeployqt seems to think.
	set(zip_files
		"${CMAKE_BINARY_DIR}/Pack/platforms/qwindows.dll"
		"${CMAKE_BINARY_DIR}/Pack/libgcc_s_dw2-1.dll"
		"${CMAKE_BINARY_DIR}/Pack/libstdc++-6.dll"
		"${CMAKE_BINARY_DIR}/Pack/libwinpthread-1.dll"
		"${CMAKE_BINARY_DIR}/Pack/Qt5Core.dll"
		"${CMAKE_BINARY_DIR}/Pack/Qt5Gui.dll"
		"${CMAKE_BINARY_DIR}/Pack/Qt5Widgets.dll"
		"${CMAKE_BINARY_DIR}/Pack/UsbTool.exe"
	)

	add_custom_target(artefact
		COMMAND "${CMAKE_COMMAND}" "-E" "tar" "cfv" "${CMAKE_BINARY_DIR}/UsbTool.zip" "--format=zip" "--" ${zip_files}
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/Pack/"
		DEPENDS pack
		COMMENT "Creating Zip"
		VERBATIM
	)

elseif (APPLE)
	# Note there is a slight bug (https://bugreports.qt.io/browse/QTBUG-60324) in macdeployqt which means we really
	# want the working directory to be the place where the dmg is generated.
	add_custom_target(artefact
		COMMAND "${_qt5Core_install_prefix}/bin/macdeployqt" "UsbTool.app" "-dmg"
		COMMAND "${CMAKE_COMMAND}" "-E" "rename" "${CMAKE_BINARY_DIR}/UsbTool.dmg" "${CMAKE_BINARY_DIR}/UsbTool.dmg"
		DEPENDS UsbTool
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
		COMMENT "Deploying Qt"
		VERBATIM
	)
endif ()
