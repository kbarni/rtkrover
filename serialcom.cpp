#include "serialcom.h"
#include <QDebug>
#include <QSerialPortInfo>

SerialCom::SerialCom(QObject *parent)
    : QObject{parent},
    m_baudRate(0),
    m_gpsRate(0),
    m_serial(nullptr)
{
}

SerialCom::~SerialCom()
{
    stop();
}

void SerialCom::init(const QString &portName, int baudRate, int gpsRate)
{
    m_portName = portName;
    m_baudRate = baudRate;
    m_gpsRate = gpsRate;

    m_serial = new QSerialPort(this);
    m_serial->setPortName(m_portName);
    m_serial->setBaudRate(m_baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        qCritical() << "Serial: Failed to open port" << m_portName << ":" << m_serial->errorString();
        return;
    }

    qDebug() << "Serial: Opened port" << m_portName << "at" << m_baudRate << "baud.";

}

void SerialCom::start()
{
    connect(m_serial, &QSerialPort::readyRead, this, &SerialCom::handleReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialCom::handleError);
}

void SerialCom::stop()
{
    if (m_serial && m_serial->isOpen()) {
        m_serial->close();
    }
}

void SerialCom::writeRtcmPacket(const Packet& packet)
{
    if (m_serial && m_serial->isOpen()) {
        m_serial->write(packet);
        //qDebug() << "Serial: Wrote" << packet.size() << "bytes to GPS.";
    }
}

void SerialCom::handleReadyRead()
{
    if (!m_serial || !m_serial->isOpen()) return;

    QByteArray data = m_serial->readAll();
    QString nmeaString(data);

    for(const QString& line : nmeaString.split("\r\n", Qt::SkipEmptyParts)) {
        if (line.startsWith('$')) {
            emit got_NMEA(line);
        }
    }
}

void SerialCom::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        qCritical() << "Serial Error:" << error << m_serial->errorString();
    }
}

int SerialCom::getGpsVersion()
{
    if (!m_serial || !m_serial->isOpen()) return 0;

    QByteArray payload = QByteArray::fromHex("B5620A040000");
    addChecksum(payload);
    m_serial->write(payload);
    m_serial->waitForBytesWritten(100);
    QByteArrayView monver(QByteArray::fromHex("B5620A04"));
    // Reading the response should be handled in handleReadyRead
    // and a signal emitted with the result. This is a simplified version.
    int k=0;
    while(k++<100){
        if (m_serial->waitForReadyRead(1000)) {
            QByteArray response = m_serial->readAll();
            qDebug()<<response;
            int pos = response.indexOf(monver);
            if(pos>0){
                QString swversion = QString::fromLocal8Bit(response.mid(4,10));
                QString hwversion = QString::fromLocal8Bit(response.mid(14,30));
                qDebug() << "GPS Version : Sofrware: " << swversion << " Hardware: "<< hwversion;
                break;
            }else {
                qDebug() <<"GPS version not received";
            }
        }
    }
    return 0;
}


bool SerialCom::setRate(int rate)
{
    if (!m_serial || !m_serial->isOpen() || rate <= 0) return false;

    uint16_t period_ms = static_cast<uint16_t>(1000.0 / rate);
    QByteArray payload=QByteArray::fromHex("B5620608000000010000");
    payload[4]=static_cast<char>((period_ms >> 8) & 0xFF);
    payload[5]=static_cast<char>(period_ms & 0xFF);
    addChecksum(payload);

    m_serial->write(payload);
    return true;//m_serial->waitForBytesWritten(100);
}

void SerialCom::addChecksum(QByteArray &msg)
{
    unsigned char ck_a = 0, ck_b = 0;
    for (unsigned char byte : msg) {
        ck_a = static_cast<unsigned char>(ck_a + byte);
        ck_b = static_cast<unsigned char>(ck_b + ck_a);
    }
    msg.append(ck_a);
    msg.append(ck_b);
}
