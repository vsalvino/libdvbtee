include ( decode_include.pri )

include ( value/value.pri )

include ( table/table.pri )
include ( descriptor/descriptor.pri )

SOURCES += \
    $$PWD/decode.cpp \
    $$PWD/desc.cpp \
    $$PWD/decoder.cpp

HEADERS += \
    $$PWD/decode.h \
    $$PWD/desc.h \
    $$PWD/decoder.h
