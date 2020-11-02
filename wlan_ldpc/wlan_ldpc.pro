TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    wlan_ldpc.cpp

HEADERS += \
    wlan_ldpc.h \
    wlan_ldpc_def.h
