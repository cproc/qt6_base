qt_internal_add_plugin(QGenodeIntegrationPlugin
    OUTPUT_NAME qgenode
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES genode
    SOURCES
        main.cpp
        qgenodeclipboard.cpp
        qgenodecursor.cpp
        qgenodeglcontext.cpp
        qgenodeintegration.cpp
        qgenodeplatformwindow.cpp
        qgenodewindowsurface.cpp
        qgenodeclipboard.h
        qgenodeintegrationplugin.h
        qgenodeplatformwindow.h
        qgenodescreen.h
        qgenodesignalproxythread.h
        qgenodewindowsurface.h
    DEFINES
        QT_NO_FOREACH
    EXCEPTIONS
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        ld
)
