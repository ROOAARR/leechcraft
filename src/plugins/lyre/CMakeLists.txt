SET (MPD_INCLUDE_DIR "/usr/include/mpd")
SET (MPD_LIBRARIES "/usr/lib/libmpdclient.so")
IF (NOT QT_USE_FILE)
	CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
	IF (COMMAND cmake_policy)
		cmake_policy (SET CMP0003 NEW)
	ENDIF (COMMAND cmake_policy)

	PROJECT (leechcraft_lyre)

	IF (NOT CMAKE_MODULE_PATH)
		SET (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;/usr/local/share/leechcraft/cmake;/usr/share/leechcraft/cmake")
	ENDIF (NOT CMAKE_MODULE_PATH)
	
	FIND_PACKAGE (Qt4 REQUIRED)
	FIND_PACKAGE (LeechCraft REQUIRED)
ENDIF (NOT QT_USE_FILE)

INCLUDE (${QT_USE_FILE})
INCLUDE_DIRECTORIES (
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${LEECHCRAFT_INCLUDE_DIR}
	${MPD_INCLUDE_DIR}
	)
SET (SRCS
	core.cpp
	entity.cpp
	item.cpp
	lyre.cpp
	lyrewidget.cpp
	client.cpp
	treeview.cpp
	playlist.cpp
	track.cpp
	)
SET (FORMS
	lyrewidget.ui
    )
QT4_WRAP_UI (UIS_H ${FORMS})

ADD_LIBRARY (leechcraft_lyre SHARED
	${COMPILED_TRANSLATIONS}
	${SRCS}
	${MOC_SRCS}
	${UIS_H}
	)
TARGET_LINK_LIBRARIES (leechcraft_lyre
	${QT_LIBRARIES}
	${LEECHCRAFT_LIBRARIES}
	${MPD_LIBRARIES}
	)
INSTALL (TARGETS leechcraft_lyre DESTINATION ${LC_PLUGINS_DEST})
