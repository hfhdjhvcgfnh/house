#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    try {
        qDebug() << "Creating MainWindow...";
        MainWindow window;
        qDebug() << "MainWindow created, showing...";
        window.show();
        qDebug() << "Window shown, entering event loop...";
        return app.exec();
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "Error",
                              QString("Exception caught: %1").arg(e.what()));
        return -1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Error",
                              "Unknown exception caught!");
        return -1;
    }
}
