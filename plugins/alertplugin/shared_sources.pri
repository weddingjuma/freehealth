TEMPLATE = lib
TARGET = Alert

DEFINES += ALERT_LIBRARY

include(../fmf_plugins.pri)
include(alertplugin_dependencies.pri)

HEADERS += \
    $${PWD}/constants.h \
    $${PWD}/alertplugin.h \
    $${PWD}/alertbase.h \
    $${PWD}/alertcore.h \
    $${PWD}/alertmanager.h \
    $${PWD}/alertitem.h \
    $${PWD}/showalertsIHM.h \
    $${PWD}/xmlalert.h \
    $${PWD}/alertmodel.h \
    $${PWD}/ialertplaceholder.h \
    $${PWD}/alertitemeditorwidget.h

SOURCES += \
    $${PWD}/alertplugin.cpp \
    $${PWD}/alertbase.cpp \
    $${PWD}/alertcore.cpp \
    $${PWD}/alertmanager.cpp \
    $${PWD}/alertitem.cpp \
    $${PWD}/showalertsIHM.cpp \
    $${PWD}/xmlalert.cpp \
    $${PWD}/alertmodel.cpp \
    $${PWD}/ialertplaceholder.cpp \
    $${PWD}/alertitemeditorwidget.cpp

FORMS += $${PWD}/ShowAlertsWidget.ui \
    $${PWD}/alertitemeditorwidget.ui

OTHER_FILES += $${PWD}/Alert.pluginspec

TRANSLATIONS += \
    $${SOURCES_TRANSLATIONS_PATH}/alertplugin_fr.ts \
    $${SOURCES_TRANSLATIONS_PATH}/alertplugin_de.ts \
    $${SOURCES_TRANSLATIONS_PATH}/alertplugin_es.ts
