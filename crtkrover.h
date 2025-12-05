#ifndef CRTKROVER_H
#define CRTKROVER_H

#include <QObject>
#include <QSettings>
#include <QByteArray>
#include "casterreader.h"
#include "serialcom.h"
#include "gpsdataparser.h"

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
    void onNmeaMessage(const QString& message);
    void onGpsFixAcquired(); // New slot

private:
    void loadConfig();
    QString detectMountPoint(); // Keep this private helper function

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

    bool m_readFromSerial;
    bool m_mountPointDetected = false; // New flag

    GpsData m_gpsData;

    CasterReader* m_casterReader;
    SerialCom* m_serialCom;
};

#endif // CRTKROVER_H
