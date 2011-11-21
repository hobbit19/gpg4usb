# #####################################################################
# Automatically generated by qmake (2.01a) Mi Mai 21 02:28:39 2008
# #####################################################################
TEMPLATE = app

# unix:TARGET = start_linux
# win32:TARGET = start_windows
# mac:TARGET = start_mac
DESTDIR = release
DEPENDPATH += .
INCLUDEPATH += . \
    ./include

#CONFIG += release static
CONFIG += debug
QT += network
# Input
HEADERS += attachments.h \
    gpgcontext.h \
    mainwindow.h \
    fileencryptiondialog.h \
    importdetaildialog.h \
    mime.h \
    keygendialog.h \
    keygenthread.h \
    keydetailsdialog.h \
    keylist.h \
    keymgmt.h \
    settingsdialog.h \
    attachmenttablemodel.h \
    textedit.h \
    editorpage.h \
    quitdialog.h \
    keyserverimportdialog.h \
    verifynotification.h \
    verifydetailsdialog.h \
    verifykeydetailbox.h \
    wizard.h

SOURCES += attachments.cpp \
    gpgcontext.cpp \
    mainwindow.cpp \
    main.cpp \
    fileencryptiondialog.cpp \
    importdetaildialog.cpp \
    mime.cpp \
    keygendialog.cpp \
    keygenthread.cpp \
    keydetailsdialog.cpp \
    keylist.cpp \
    keymgmt.cpp \
    settingsdialog.cpp \
    attachmenttablemodel.cpp \
    textedit.cpp \
    editorpage.cpp \
    quitdialog.cpp \
    keyserverimportdialog.cpp \
    verifynotification.cpp \
    verifydetailsdialog.cpp \
    verifykeydetailbox.cpp \
    wizard.cpp

RC_FILE = gpg4usb.rc

RESOURCES = gpg4usb.qrc

# comment out line below for static building
LIBS += -lgpgme \
     -lgpg-error \

# comment in for static buildding in windows
#INCLUDEPATH += ./winbuild/include 
#LIBS +=./winbuild/lib/libgpgme.a ./winbuild/lib/libgpg-error.a 

    
DEFINES += _FILE_OFFSET_BITS=64
TRANSLATIONS = release/ts/gpg4usb_en.ts \
    release/ts/gpg4usb_de.ts \
    release/ts/gpg4usb_ru.ts \
    release/ts/gpg4usb_fr.ts \
    release/ts/gpg4usb_it.ts \
    release/ts/gpg4usb_pt_BR.ts \
    release/ts/gpg4usb_es.ts \
    release/ts/gpg4usb_ar.ts \
    release/ts/gpg4usb_zh.ts \
    release/ts/gpg4usb_vi.ts \
    release/ts/gpg4usb_my.ts \
    release/ts/gpg4usb_fa_IR.ts 
   
