#include "MainWindow.h"
#include <QApplication>
#include <QStyleFactory>

// QGst는 현재 사용하지 않음
// #ifdef QGST_AVAILABLE
// #include <QGst/Init>
// #endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
// QGst는 현재 사용하지 않음
// #ifdef QGST_AVAILABLE
//     // GStreamer 초기화
//     QGst::init(&argc, &argv);
// #endif
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
