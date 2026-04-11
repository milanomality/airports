#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Airports"));
    app.setApplicationVersion(QStringLiteral("1.0"));

    MainWindow window;
    window.show();

    return app.exec();
}
