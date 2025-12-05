#ifndef SERIALCOM_H
#define SERIALCOM_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QByteArray>

class SerialCom : public QObject
{
    Q_OBJECT
public:
    using Packet = QByteArray;

    explicit SerialCom(QObject *parent = nullptr);
    ~SerialCom();

    void init(const QString& portName, int baudRate, int gpsRate);
    void start();
    void stop();

    int getGpsVersion();
    bool setRate(int rate);

public slots:
    void writeRtcmPacket(const Packet& packet);

signals:
    void got_NMEA(const QString& nmea);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    void addChecksum(QByteArray &msg);

    QString m_portName;
    int m_baudRate;
    int m_gpsRate;
    QSerialPort* m_serial;
};

#endif // SERIALCOM_H
