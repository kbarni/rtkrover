#ifndef OUTPUTHANDLER_H
#define OUTPUTHANDLER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include "gpsdataparser.h"

class OutputHandler : public QObject
{
    Q_OBJECT

public:
    enum class OutputMethod {
        False,
        Socket,
        File,
        Stdout
    };

    enum class OutputType {
        NMEA,
        CSV,
        JSON
    };

    explicit OutputHandler(OutputMethod method, OutputType type, const QString& file = "", int port = 0, QObject *parent = nullptr);
    ~OutputHandler();

public slots:
    void processNmeaData(const QString& nmeaSentence);

private slots:
    void onNewConnection();
    void onSocketDisconnected();

private:
    void writeData(const QString& data);
    QString formatCsv(const GpsData& gpsData);
    QString formatJson(const GpsData& gpsData);

    OutputMethod _method;
    OutputType _type;

    // For file output
    QFile _file;

    // For socket output
    QTcpServer* _server;
    QList<QTcpSocket*> _clients;
    int _port;

    bool _isCsvHeaderWritten;
};

#endif // OUTPUTHANDLER_H
