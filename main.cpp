#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include "crtkrover.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("rtkrover_qt");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("NTRIP client for RTK GPS.");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption configFileOption("c", "Configuration file.", "file", "config.ini");
    parser.addOption(configFileOption);
    parser.process(a);

    QString configFile = parser.value(configFileOption);
    qDebug() << "Using configuration file:" << configFile;

    CRTKRover rover(configFile, &a);
    rover.start();

    // Connect a signal to quit the application when the rover is done
    // For now, we can just run the event loop. A signal from CRTKRover could stop it.
    // QObject::connect(&rover, &CRTKRover::finished, &a, &QCoreApplication::quit);

    return a.exec();
}
