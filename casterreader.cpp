#include "casterreader.h"
#include "crc24q.h"
#include <QDebug>
#include <QThread>
#include <QRegularExpression>

CasterReader::CasterReader(QObject *parent)
    : QObject(parent),
    m_port(0),
    m_socket(nullptr)
{
    m_connected = false;
}

CasterReader::~CasterReader()
{
    stop();
}

void CasterReader::init(const QString &host, int port, const QString &user, const QString &password)
{
    m_host = host;
    m_port = port;
    m_user = user;
    m_password = password;

    if (!m_socket) {
        m_socket = new QTcpSocket(this);
    }
    connect(m_socket, &QTcpSocket::connected, this, &CasterReader::onConnected);

    qDebug() << "NTRIP: Connecting to" << m_host << ":" << m_port;
    m_buffer.reserve(4096);
}

void CasterReader::start(const QString &mountpoint)
{
    m_socket->connectToHost(m_host, m_port);
    m_socket->waitForConnected();
    qDebug()<<"NTRIP: Starting communication with caster";
    QThread::sleep(10);
    m_mountpoint = mountpoint;
    connect(m_socket, &QTcpSocket::readyRead, this, &CasterReader::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &CasterReader::onErrorOccurred);

    QString auth = QByteArray(QString("%1:%2").arg(m_user).arg(m_password).toUtf8()).toBase64();
    QString request = "GET /" + m_mountpoint + " HTTP/1.1\r\n";
    request += "Host: " + m_host + ":" + QString::number(m_port) + "\r\n";
    request += "User-Agent: QtNtripClient/1.0\r\n";
    request += "Authorization: Basic " + auth + "\r\n";
    request += "Ntrip-Version: Ntrip/2.0\r\n";
    request += "Connection: close\r\n\r\n";
    m_socket->write(request.toUtf8());

}

void CasterReader::stop()
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->abort();
    }
}

void CasterReader::onConnected()
{
    qDebug() << "NTRIP: Connected.";
    m_connected = true;
}

void CasterReader::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    // Check for the initial HTTP response
    if (m_buffer.startsWith("ICY 200 OK") || m_buffer.startsWith("HTTP/1.1 200 OK")) {
        qDebug() << "NTRIP: " << m_buffer.split('\r').first();
        qDebug() << "NTRIP: Starting to receive data...";
        // Find the end of the header and remove it
        int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd != -1) {
            m_buffer.remove(0, headerEnd + 4);
        }
    }

    extract_rtcm_packets(m_buffer);
}

void CasterReader::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    qCritical() << "NTRIP Socket Error:" << m_socket->errorString();
    stop();
}

void CasterReader::extract_rtcm_packets(QByteArray &buffer)
{
    int offset = 0;
    while (offset <= buffer.size() - 8) { // Minimum RTCM packet size is 8 bytes
        if (static_cast<unsigned char>(buffer.at(offset)) != 0xD3) {
            offset++;
            continue;
        }

        if (buffer.size() - offset < 3) {
            break; // Not enough data for header
        }

        size_t length = ((static_cast<unsigned char>(buffer.at(offset + 1)) & 0x03) << 8) | static_cast<unsigned char>(buffer.at(offset + 2));
        size_t total_length = length + 6; // 3 header bytes + payload + 3 crc bytes

        if (buffer.size() - offset < static_cast<long>(total_length)) {
            break; // Incomplete packet
        }

        Packet packet = buffer.mid(offset, total_length);
        Packet payload = buffer.mid(offset, total_length - 3);

        uint32_t crc_received = (static_cast<unsigned char>(packet.at(total_length - 3)) << 16) |
                                (static_cast<unsigned char>(packet.at(total_length - 2)) << 8) |
                                 static_cast<unsigned char>(packet.at(total_length - 1));

        std::vector<unsigned char> payload_vec(payload.begin(), payload.end());
        uint32_t crc_computed = rtcm_crc(payload_vec);

        if (crc_received == crc_computed) {
            uint32_t type = (static_cast<unsigned char>(payload.at(3)) << 4) | ((static_cast<unsigned char>(payload.at(4)) & 240) >> 4);
            qDebug() << "RTCM packet received (CRC OK), type:" << type << "length" << total_length;
            emit rtcmPacketReady(packet);
            offset += total_length;
        } else {
            qWarning() << "Invalid CRC for packet. Discarding.";
            offset++; // Skip the 0xD3 byte and search again
        }
    }
    buffer.remove(0, offset);
}


QString CasterReader::getAutoMountPoint(double lat, double lon)
{
    m_socket->connectToHost(m_host, m_port);
    m_socket->waitForConnected();
    m_mountpoint="";
    qDebug() << "NTRIP: *** Detecting closest caster ***";
    QString auth = QByteArray(QString("%1:%2").arg(m_user).arg(m_password).toUtf8()).toBase64();
    QString request = "GET / HTTP/1.1\r\n";
    request += "Host: " + m_host + ":" + QString::number(m_port) + "\r\n";
    request += "User-Agent: QtNtripClient/1.0\r\n";
    request += "Authorization: Basic " + auth + "\r\n";
    request += "Ntrip-Version: Ntrip/2.0\r\n";
    request += "Connection: close\r\n\r\n";
    m_socket->write(request.toLocal8Bit());
    m_socket->waitForBytesWritten();

    QByteArray answer = m_socket->readAll();
    while(m_socket->waitForReadyRead()){
        answer.append(m_socket->readAll());
    }
    QStringList casterlist=QString::fromLocal8Bit(answer).split("\r\n");
    if(casterlist.length()==0)
        qDebug() << " --> ERROR: Caster list empty!";
    else
        qDebug() << " --> "<< m_host << "proposes" << casterlist.length()<<"mount points... Detecting the closest caster";
    QString closestCaster, closestCountry, closestCity;
    float mindist=50;
    float cast_lat,cast_lon,dist;
    for(QString casterdata:casterlist){
        QStringList castersl = casterdata.split(";");
        if(castersl[0]!="STR")continue;
        cast_lat = castersl[9].toFloat();
        cast_lon = castersl[10].toFloat();
        dist = haversine_distance(lat,lon,cast_lat,cast_lon);
        if(dist<mindist){
            qDebug()<<" --> Caster "<<castersl[1]<<" at "<<dist<<"km...";
            mindist = dist;
            closestCaster=castersl[1];
            closestCity=castersl[2];
            closestCountry=castersl[7];
        }
    }
    if(mindist>=50)
        qDebug()<<"ERROR: Closest caster too far ("<<mindist<<"km)";
    else
        qDebug()<<"*** Closest caster"<<closestCaster<<"in"<<closestCity<<"("<<closestCountry<<") at "<<mindist<<"km ***";
    m_socket->disconnect();
    return closestCaster;
}

double CasterReader::haversine_distance(double lat1, double lon1, double lat2, double lon2)
{
    lat1 = to_radians(lat1);
    lon1 = to_radians(lon1);
    lat2 = to_radians(lat2);
    lon2 = to_radians(lon2);

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
               cos(lat1) * cos(lat2) *
                   sin(dlon / 2.0) * sin(dlon / 2.0);

    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

    return EARTH_RADIUS_KM * c;
}
