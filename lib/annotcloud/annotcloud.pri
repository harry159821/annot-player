# annotcloud.pri
# 6/28/2011

DEFINES += WITH_LIB_ANNOTCLOUD

DEPENDPATH +=$$PWD

HEADERS += \
    $$PWD/alias.h\
    $$PWD/annotation.h \
    $$PWD/annothtml.h \
    $$PWD/annotpaint.h \
    $$PWD/annottag.h \
    $$PWD/annotxml.h \
    $$PWD/gamehook.h \
    $$PWD/gamethread.h \
    $$PWD/traits.h \
    $$PWD/token.h \
    $$PWD/user.h

SOURCES += \
    $$PWD/alias.cc \
    $$PWD/annotation.cc \
    $$PWD/annothtml.cc \
    $$PWD/annothtml_parse.cc \
    $$PWD/annothtml_thread.cc \
    $$PWD/annothtml_unparse.cc \
    $$PWD/annotpaint.cc \
    $$PWD/annotxml.cc \
    $$PWD/gamehook.cc \
    $$PWD/gamethread.cc \
    $$PWD/token.cc \
    $$PWD/user.cc

JSF_FILES += \
    $$PWD/jsf/a.xhtml \
    $$PWD/jsf/error.xhtml \
    $$PWD/jsf/i.xhtml \
    $$PWD/jsf/t.xhtml

QT      += core xml webkit

OTHER_FILES += $$JSF_FILES

# EOF


