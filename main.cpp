#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // The MainWindow should be allocated on the heap to ensure its lifetime
    // persists for the duration of the application's event loop.
    MainWindow *window = nullptr;
    try {
        qDebug() << "Creating MainWindow...";
        window = new MainWindow();
        qDebug() << "MainWindow created, showing...";
        window->show();
        qDebug() << "Window shown, entering event loop...";
        return app.exec();
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "Error",
                              QString("Exception caught: %1").arg(e.what()));
        if(window) {
            delete window;
        }
        return -1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Error",
                              "Unknown exception caught!");
        if(window) {
            delete window;
        }
        return -1;
    }
}
