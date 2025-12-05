#include "gpsdataparser.h"

const QStringList GpsData::fixmodelist = {"Error","No fix","2D","3D"};
const QStringList GpsData::fixquallist = {"No fix","GPS","DGPS","","RTK/Fix","RTK/Float"};

GpsData::GpsData()
    : _year(0), _month(0), _day(0),
      _hours(0), _minutes(0), _seconds(0.0),
      _latitude(0.0), _longitude(0.0), _altitude(0.0),
      _fix_quality(0), _fix_mode(0),
      _speed_knots(0.0), _speed_ms(0.0),
      _heading_degrees(0.0), _hdop(0.0)
{
}

void GpsData::parse_NMEA(const QString& sentence) {
    if (!validate_checksum(sentence)) {
        // qWarning() << "NMEA: Invalid checksum for sentence:" << sentence;
        return;
    }

    QString s = sentence.mid(1, sentence.indexOf('*') - 1);
    QStringList fields = s.split(',');

    if (fields.empty()) return;

    const QString& type = fields[0];
    if (type.length() > 2 && type.endsWith("GGA")) {
        parse_gga(fields);
    } else if (type.length() > 2 && type.endsWith("RMC")) {
        parse_rmc(fields);
    } else if (type.length() > 2 && type.endsWith("GSA")) {
        parse_gsa(fields);
    }
}

int GpsData::year() const { return _year; }
int GpsData::month() const { return _month; }
int GpsData::day() const { return _day; }
int GpsData::hours() const { return _hours; }
int GpsData::minutes() const { return _minutes; }
double GpsData::seconds() const { return _seconds; }
double GpsData::latitude() const { return _latitude; }
double GpsData::longitude() const { return _longitude; }
double GpsData::altitude() const { return _altitude; }
QString GpsData::fixQuality() const { return fixquallist[_fix_quality]; }
QString GpsData::fixMode() const { return fixmodelist[_fix_mode]; }
double GpsData::speedKnots() const { return _speed_knots; }
double GpsData::speedMs() const { return _speed_ms; }
double GpsData::headingDegrees() const { return _heading_degrees; }
double GpsData::hdop() const { return _hdop; }
bool GpsData::hasFix() const { return _fix_quality>0; }

void GpsData::print() const {
    qDebug() << "\033[2J\033[1;1H";
    qDebug().noquote() << "========================= GPS Data =========================";
    if (_year != 0) {
        qDebug().noquote() << QString("Date: %1-%2-%3  Time: %4:%5:%6 UTC")
                            .arg(_year, 4, 10, QChar('0'))
                            .arg(_month, 2, 10, QChar('0'))
                            .arg(_day, 2, 10, QChar('0'))
                            .arg(_hours, 2, 10, QChar('0'))
                            .arg(_minutes, 2, 10, QChar('0'))
                            .arg(_seconds, 5, 'f', 2, QChar('0'));
    } else {
        qDebug().noquote() << "Date/Time: N/A";
    }

    qDebug().noquote() << QString("Position: %1 / %2 | Altitude: %3 m")
                            .arg(_latitude, 0, 'f', 6)
                            .arg(_longitude, 0, 'f', 6)
                            .arg(_altitude, 0, 'f', 2);

    qDebug().noquote() << "Fix Quality:" << fixquallist[_fix_quality] << "| Fix Mode:" << fixmodelist[_fix_mode];
    qDebug().noquote() << QString("Speed: %1 m/s | Heading %2°").arg(_speed_ms,0,'f',3).arg(_heading_degrees);
    qDebug().noquote() << "HDOP (Position Error Est.):" << _hdop;
    qDebug().noquote() << "============================================================";
}

bool GpsData::validate_checksum(const QString& sentence) {
    int star_pos = sentence.indexOf('*');
    if (star_pos == -1 || star_pos > sentence.length() - 3) {
        return false;
    }
    unsigned char checksum = 0;
    for (int i = 1; i < star_pos; ++i) {
        checksum ^= sentence.at(i).toLatin1();
    }
    bool ok;
    int received_checksum = sentence.mid(star_pos + 1).toUInt(&ok, 16);
    return ok && (checksum == received_checksum);
}

double GpsData::parse_lat_lon(const QString& value, const QString& direction) {
    if (value.isEmpty()) return 0.0;
    double raw_value = value.toDouble();
    int degrees = static_cast<int>(raw_value / 100.0);
    double minutes = raw_value - (degrees * 100.0);
    double decimal_degrees = degrees + minutes / 60.0;
    if (direction == "S" || direction == "W") {
        decimal_degrees = -decimal_degrees;
    }
    return decimal_degrees;
}

void GpsData::parse_gga(const QStringList& fields) {
    if (fields.size() < 10) return;
    if (!fields[2].isEmpty()) _latitude = parse_lat_lon(fields[2], fields[3]);
    if (!fields[4].isEmpty()) _longitude = parse_lat_lon(fields[4], fields[5]);
    if (!fields[6].isEmpty()) _fix_quality = fields[6].toInt();
    if (!fields[8].isEmpty()) _hdop = fields[8].toDouble();
    if (!fields[9].isEmpty()) _altitude = fields[9].toDouble();
}

void GpsData::parse_rmc(const QStringList& fields) {
    if (fields.size() < 10) return;
    if (!fields[1].isEmpty()) {
        double time_val = fields[1].toDouble();
        _hours = static_cast<int>(time_val / 10000);
        _minutes = static_cast<int>(fmod(time_val, 10000) / 100);
        _seconds = fmod(time_val, 100);
    }
    if (fields[2] != "A") {
        _fix_quality = 0;
    }
    if (!fields[3].isEmpty()) _latitude = parse_lat_lon(fields[3], fields[4]);
    if (!fields[5].isEmpty()) _longitude = parse_lat_lon(fields[5], fields[6]);
    if (!fields[7].isEmpty()) {
        _speed_knots = fields[7].toDouble();
        _speed_ms = _speed_knots * 0.5144;
    }
    if (!fields[8].isEmpty()) _heading_degrees = fields[8].toDouble();
    if (!fields[9].isEmpty()) {
        int date_val = fields[9].toInt();
        _day = date_val / 10000;
        _month = (date_val / 100) % 100;
        _year = date_val % 100 + 2000;
    }
}

void GpsData::parse_gsa(const QStringList& fields) {
    if (fields.size() < 17) return;
    if (!fields[2].isEmpty()) _fix_mode =fields[2].toInt();
    if (!fields[15].isEmpty()) _hdop = fields[15].toDouble();
}
