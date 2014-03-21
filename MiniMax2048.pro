TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++0x -pthread
QMAKE_LFLAGS -= -Wl,-O1,--sort-common,--as-needed,-z,relro
QMAKE_LFLAGS += -pthread -Wl,-O1

QMAKE_CFLAGS_RELEASE -= -O2
QMAKE_CFLAGS_RELEASE += -O3 -DNDEBUG
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3 -DNDEBUG

pg {
	QMAKE_CFLAGS += -pg
	QMAKE_CXXFLAGS += -pg
	QMAKE_LFLAGS += -pg
}

SOURCES += main.cpp \
	2048.cpp \
	Minimax.cpp \
	JS_Minimax.cpp

HEADERS += \
	2048.h \
	Minimax.h

