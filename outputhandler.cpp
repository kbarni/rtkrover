#include "outputhandler.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QTextStream>
#include <QTimeZone>

OutputHandler::OutputHandler(OutputMethod method, OutputType type, const QString& file, int port, QObject *parent)
    : QObject(parent),
      _method(method),
      _type(type),
      _server(nullptr),
      _port(port),
      _isCsvHeaderWritten(false)
{
    if (_method == OutputMethod::File) {
        _file.setFileName(file);
        if (!_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            qWarning() << "Failed to open output file:" << file;
            _method = OutputMethod::False; // Disable output if file can't be opened
        }
    } else if (_method == OutputMethod::Socket) {
        _server = new QTcpServer(this);
        connect(_server, &QTcpServer::newConnection, this, &OutputHandler::onNewConnection);
        if (!_server->listen(QHostAddress::Any, _port)) {
            qWarning() << "Failed to start TCP server on port" << _port;
            _method = OutputMethod::False; // Disable output if server can't start
        } else {
            qDebug() << "TCP server listening on port" << _port;
        }
    }
}

OutputHandler::~OutputHandler()
{
    if (_file.isOpen()) {
        _file.close();
    }
    if (_server) {
        _server->close();
    }
}

void OutputHandler::processNmeaData(const QString& nmeaSentence)
{
    if (_method == OutputMethod::False) {
        return;
    }

    QString outputData;

    if (_type == OutputType::NMEA) {
        outputData = nmeaSentence;
    } else {
        GpsData gpsData;
        gpsData.parse_NMEA(nmeaSentence);

        if (gpsData.hasFix()) {
            if (_type == OutputType::CSV) {
                if (!_isCsvHeaderWritten) {
                    outputData = formatCsv(gpsData);
                     _isCsvHeaderWritten = true;
                }
                else
                {
                    outputData = formatCsv(gpsData).split('\n').last();
                }
            } else if (_type == OutputType::JSON) {
                outputData = formatJson(gpsData);
            }
        }
    }

    if (!outputData.isEmpty()) {
        writeData(outputData);
    }
}

void OutputHandler::onNewConnection()
{
    QTcpSocket *clientSocket = _server->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::disconnected, this, &OutputHandler::onSocketDisconnected);
    _clients.append(clientSocket);
    qDebug() << "Client connected:" << clientSocket->peerAddress().toString();
}

void OutputHandler::onSocketDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (clientSocket) {
        _clients.removeAll(clientSocket);
        clientSocket->deleteLater();
        qDebug() << "Client disconnected";
    }
}

void OutputHandler::writeData(const QString& data)
{
    switch (_method) {
    case OutputMethod::Stdout:
        qDebug().noquote() << data;
        break;
    case OutputMethod::File:
        if (_file.isOpen()) {
            QTextStream stream(&_file);
            stream << data << "\n";
        }
        break;
    case OutputMethod::Socket:
        for (QTcpSocket* socket : _clients) {
            socket->write(data.toUtf8());
            socket->write("\n");
        }
        break;
    case OutputMethod::False:
    default:
        break;
    }
}

QString OutputHandler::formatCsv(const GpsData& gpsData)
{
    QString csv;
    QTextStream stream(&csv);

    if (!_isCsvHeaderWritten) {
        stream << "timestamp,latitude,longitude,altitude,fix_quality,fix_mode,speed_ms,heading_degrees,hdop\n";
    }

    QDate date(gpsData.year(), gpsData.month(), gpsData.day());
    int seconds = static_cast<int>(gpsData.seconds());
    int ms = static_cast<int>((gpsData.seconds() - seconds) * 1000);
    QTime time(gpsData.hours(), gpsData.minutes(), seconds, ms);
    QDateTime dateTime(date, time, QTimeZone::utc());

    stream << dateTime.toString(Qt::ISODate) << ","
           << QString::number(gpsData.latitude(), 'f', 8) << ","
           << QString::number(gpsData.longitude(), 'f', 8) << ","
           << QString::number(gpsData.altitude(), 'f', 3) << ","
           << gpsData.fixQuality() << ","
           << gpsData.fixMode() << ","
           << QString::number(gpsData.speedMs(), 'f', 3) << ","
           << QString::number(gpsData.headingDegrees(), 'f', 2) << ","
           << QString::number(gpsData.hdop(), 'f', 2);

    return csv;
}

QString OutputHandler::formatJson(const GpsData& gpsData)
{
    QJsonObject json;

    QDate date(gpsData.year(), gpsData.month(), gpsData.day());
    int seconds = static_cast<int>(gpsData.seconds());
    int ms = static_cast<int>((gpsData.seconds() - seconds) * 1000);
    QTime time(gpsData.hours(), gpsData.minutes(), seconds, ms);
    QDateTime dateTime(date, time, QTimeZone::utc());

    json["timestamp"] = dateTime.toString(Qt::ISODate);
    json["latitude"] = gpsData.latitude();
    json["longitude"] = gpsData.longitude();
    json["altitude"] = gpsData.altitude();
    json["fix_quality"] = gpsData.fixQuality();
    json["fix_mode"] = gpsData.fixMode();
    json["speed_ms"] = gpsData.speedMs();
    json["heading_degrees"] = gpsData.headingDegrees();
    json["hdop"] = gpsData.hdop();

    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}
