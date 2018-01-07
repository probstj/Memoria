#include <QtGui/QApplication>
#include <QTranslator>
#include "memory.h"


int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/memory.ico"));

    // must be set for QSettings to work (to save settings):
    QCoreApplication::setOrganizationName("Memoria");
    QCoreApplication::setApplicationName("Memoria");
    
    // use translator file "memory.ts" or compiled "memory.qm" to translate tr() strings:
    QTranslator qtTranslator;
    //qtTranslator.load(QApplication::applicationDirPath() + "/memory_de");
    qtTranslator.load(":/memory_de");
    app.installTranslator(&qtTranslator);
    
    Memory foo;
    foo.show();
    if (foo.startNewGame())
        return app.exec();
    return 0;
}


