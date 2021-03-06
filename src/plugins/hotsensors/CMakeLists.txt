cmake_minimum_required (VERSION 2.8)
project (leechcraft_hotsensors)
include (InitLCPlugin OPTIONAL)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package (Qwt REQUIRED)

set (QT_USE_QTSVG TRUE)
set (QT_USE_QTDECLARATIVE TRUE)
include (${QT_USE_FILE})
include_directories (
	${CMAKE_CURRENT_BINARY_DIR}
	${Boost_INCLUDE_DIR}
	${LEECHCRAFT_INCLUDE_DIR}
	${QWT_INCLUDE_DIRS}
	)
set (SRCS
	hotsensors.cpp
	sensorsmanager.cpp
	historymanager.cpp
	plotmanager.cpp
	sensorsgraphmodel.cpp
	contextwrapper.cpp
	)
#CreateTrs("hotsensors" "en;ru_RU" COMPILED_TRANSLATIONS)
CreateTrsUpTarget("hotsensors" "en;ru_RU" "${SRCS}" "${FORMS}" "")

add_library (leechcraft_hotsensors SHARED
	${COMPILED_TRANSLATIONS}
	${SRCS}
	)
target_link_libraries (leechcraft_hotsensors
	${QT_LIBRARIES}
	${LEECHCRAFT_LIBRARIES}
	${QWT_LIBRARIES}
	sensors
	)
install (TARGETS leechcraft_hotsensors DESTINATION ${LC_PLUGINS_DEST})
install (DIRECTORY share/qml/hotsensors DESTINATION ${LC_QML_DEST})
