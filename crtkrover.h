#ifndef CRTKROVER_H
#define CRTKROVER_H

#include <QObject>
#include <QSettings>
#include <QByteArray>
#include "casterreader.h"
#include "serialcom.h"
#include "gpsdataparser.h"

class OutputHandler; // Forward declaration

class CRTKRover : public QObject
{
    Q_OBJECT
public:
    using Packet = QByteArray;

    explicit CRTKRover(const QString& configFile, QObject *parent = nullptr);
    ~CRTKRover();

    void start();
    void stop();

private slots:
    void onGpsFixAcquired();
    void onNmeaMessage(const QString& message);

private:
    void loadConfig();
    QString detectMountPoint();

    QSettings* m_settings;
    QString m_configFile;

    // NTRIP Caster settings
    QString m_ntripHost;
    int m_ntripPort;
    QString m_mountpoint;
    QString m_ntripUsername;
    QString m_ntripPassword;

    // Serial settings
    QString m_serialPort;
    int m_serialBaud;
    int m_gpsRate;

    bool m_mountPointDetected = false;

    GpsData m_gpsData;

    CasterReader* m_casterReader;
    SerialCom* m_serialCom;
    OutputHandler* _outputHandler = nullptr;
};

#endif // CRTKROVER_H
