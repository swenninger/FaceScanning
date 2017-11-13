#include "MainWindow.h"
#include <QApplication>


Q_DECLARE_METATYPE(size_t)

#include "util.h"
int main(int argc, char *argv[])
{
    qRegisterMetaType<size_t>();

    QApplication a(argc, argv);

    MainWindow w;
    w.show();


    return a.exec();
}
