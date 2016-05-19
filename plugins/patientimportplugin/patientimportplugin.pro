TARGET = patientimport
TEMPLATE = lib

DEFINES += PATIENTIMPORT_LIBRARY
BUILD_PATH_POSTFIXE = FreeMedForms

include(../fmf_plugins.pri)
include(patientimport_dependencies.pri)

!with-patientimport{
    error(patientimport plugin not requested)
} else {
    message(Building patientimport plugin)
}

HEADERS += \
    patientimportplugin.h\
    patientimport_exporter.h\
    patientimportconstants.h
        
SOURCES += \
    patientimportplugin.cpp

OTHER_FILES = patientimport.pluginspec

equals(TEST, 1) {
    SOURCES += \
        tests/test_patientimportplugin.cpp
}

#include translations
TRANSLATION_NAME = patientimport
include($${SOURCES_ROOT_PATH}/buildspecs/translations.pri)