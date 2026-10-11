#include <QApplication>
#include <QStandardPaths>
#include "ui/mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Rest Cu Me"));
    QApplication::setOrganizationName(QStringLiteral("RestCuMe"));
    QApplication::setApplicationDisplayName(QStringLiteral("Rest Cu Me"));

    rcm::MainWindow window;
    window.show();
    return app.exec();
}
