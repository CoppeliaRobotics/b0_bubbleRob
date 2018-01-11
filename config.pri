# location of boost headers:
    BOOST_INCLUDEPATH = "c:/local/boost_1_62_0"    # (e.g. Windows)
    #BOOST_INCLUDEPATH = "/usr/local/include"    # (e.g. MacOS)

# location of B0 headers:
    B0_INCLUDEPATH = "../blueZero/include"    # (e.g. Windows)

# location of protobuf headers:
    PROTOBUF_INCLUDEPATH = "d:/protobuf-3.5.0/src"    # (e.g. Windows)

# qscintilla libraries to link:
    B0_LIBS = "../blueZero/build/Release/b0.lib"    # (e.g. Windows)

# Make sure if a config.pri is found one level above, that it will be used instead of this one:
    exists(../config.pri) { include(../config.pri) }
