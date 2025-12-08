#ifndef GPSDATAPARSER_H
#define GPSDATAPARSER_H

#include <QString>
#include <QVector>
#include <QDebug>
#include <cmath>

struct UtmCoords {
    double easting;
    double northing;
    int zone;
    char hemisphere; // 'N' for Northern, 'S' for Southern
};

class GpsData {
public:
    GpsData();

    // --- Public API for NMEA parsing ---
    void parse_NMEA(const QString& sentence);

    // --- Getters for GPS data ---
    int year() const;
    int month() const;
    int day() const;
    int hours() const;
    int minutes() const;
    double seconds() const;
    double latitude() const;
    double longitude() const;
    double altitude() const;
    QString fixQuality() const;
    QString fixMode() const;
    bool hasFix() const;
    double speedKnots() const;
    double speedMs() const;
    double headingDegrees() const;
    double hdop() const;

    // --- UTM Conversion ---
    UtmCoords convertToUtm() const;

    // --- Print method ---
    void print() const;

private:
    // --- Data members (from GpsData struct) ---
    int _year, _month, _day;
    int _hours, _minutes;
    double _seconds;
    double _latitude;
    double _longitude;
    double _altitude;
    int _fix_quality;
    int _fix_mode;
    double _speed_knots;
    double _speed_ms;
    double _heading_degrees;
    double _hdop;

    static const QStringList fixmodelist;
    static const QStringList fixquallist;

    // --- Private parsing methods (from NMEA namespace) ---
    bool validate_checksum(const QString& sentence);
    double parse_lat_lon(const QString& value, const QString& direction);
    void parse_gga(const QStringList& fields);
    void parse_rmc(const QStringList& fields);
    void parse_gsa(const QStringList& fields);
};

#endif // GPSDATAPARSER_H
