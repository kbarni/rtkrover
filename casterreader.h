#ifndef CASTERREADER_H
#define CASTERREADER_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>

const double EARTH_RADIUS_KM = 6371.0;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class CasterReader : public QObject
{
    Q_OBJECT
public:
    using Packet = QByteArray;

    explicit CasterReader(QObject *parent = nullptr);
    ~CasterReader();

    void init(const QString& host, int port, const QString& user, const QString& password);
    void start(const QString& mountpoint);
    void stop();

    QString getAutoMountPoint(double lon, double lat);

signals:
    void rtcmPacketReady(const Packet& packet);

private slots:
    void onConnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    void extract_rtcm_packets(QByteArray& buffer);
    double haversine_distance(double lat1, double lon1, double lat2, double lon2);

    QString m_host;
    int m_port;
    bool m_connected;
    QString m_user;
    QString m_password;
    QString m_mountpoint;
    QTcpSocket* m_socket;
    QByteArray m_buffer;

    inline double to_radians(double degree) {
        return degree * M_PI / 180.0;
    }
};

#endif // CASTERREADER_H
