#include "crtkrover.h"
#include "outputhandler.h"
#include <QDebug>

CRTKRover::CRTKRover(const QString &configFile, QObject *parent)
    : QObject{parent},
    m_configFile(configFile),
    m_settings(nullptr),
    m_casterReader(nullptr),
    m_serialCom(nullptr),
    _outputHandler(nullptr),
    m_gpsData()
{
    loadConfig();
}

CRTKRover::~CRTKRover()
{
    stop();
    delete m_settings;
    delete _outputHandler;
}

void CRTKRover::loadConfig()
{
    m_settings = new QSettings(m_configFile, QSettings::IniFormat, this);

    m_ntripHost = m_settings->value("ntrip/host", "crtk.net").toString();
    m_ntripPort = m_settings->value("ntrip/port", 2101).toInt();
    m_mountpoint = m_settings->value("ntrip/mountpoint", "auto").toString();
    m_ntripUsername = m_settings->value("ntrip/username", "centipede").toString();
    m_ntripPassword = m_settings->value("ntrip/password", "centipede").toString();

    m_serialPort = m_settings->value("serial/port", "/dev/ttyACM0").toString();
    m_serialBaud = m_settings->value("serial/baud", 115200).toInt();
    m_gpsRate = m_settings->value("serial/frequency", 10).toInt();

    qDebug() << "Config loaded from" << m_configFile;
}

void CRTKRover::start()
{
    qDebug() << "Starting services...";

    // Setup OutputHandler
    QString outputMethodStr = m_settings->value("output/output", "false").toString().toLower();
    QString outputTypeStr = m_settings->value("output/output_type", "nmea").toString().toLower();
    OutputHandler::OutputMethod method;
    OutputHandler::OutputType type;

    if (outputMethodStr == "socket") method = OutputHandler::OutputMethod::Socket;
    else if (outputMethodStr == "file") method = OutputHandler::OutputMethod::File;
    else if (outputMethodStr == "stdout") method = OutputHandler::OutputMethod::Stdout;
    else method = OutputHandler::OutputMethod::False;

    if (outputTypeStr == "csv") type = OutputHandler::OutputType::CSV;
    else if (outputTypeStr == "json") type = OutputHandler::OutputType::JSON;
    else type = OutputHandler::OutputType::NMEA;

    if (method != OutputHandler::OutputMethod::False) {
        QString outFile = m_settings->value("output/filename", "output.txt").toString();
        int outPort = m_settings->value("output/port", 1298).toInt();
        _outputHandler = new OutputHandler(method, type, outFile, outPort, this);
    }

    m_casterReader = new CasterReader(this);
    m_serialCom = new SerialCom(this);

    // Autodetect serial port if set to auto.
    if(m_serialPort=="auto")
        m_serialPort = SerialCom::autodetect();

    // Connect the data pipeline: Caster -> Serial
    connect(m_casterReader, &CasterReader::rtcmPacketReady, m_serialCom, &SerialCom::writeRtcmPacket);

    // Connect the NMEA output from serial to our handler
    connect(m_serialCom, &SerialCom::got_NMEA, this, &CRTKRover::onNmeaMessage);

    if (_outputHandler) {
        connect(m_serialCom, &SerialCom::got_NMEA, _outputHandler, &OutputHandler::processNmeaData);
    }

    // Start serial communication
    m_serialCom->init(m_serialPort, m_serialBaud, m_gpsRate);
    //setup GPS wuth UBS messages
    //m_serialCom->getGpsVersion();
    //m_serialCom->setRate(m_gpsRate);
    m_serialCom->start();

    //Start caster reader
    m_casterReader->init(m_ntripHost,m_ntripPort,m_ntripUsername,m_ntripPassword);

    // Start CasterReader if mountpoint is defined. Other
    if(m_mountpoint=="auto"){
        qDebug() << "Waiting for GPS fix to determine mount point...";
    } else {
        m_mountPointDetected=true;
        m_casterReader->start(m_mountpoint);
    }
    qDebug() << "Services started.";
}

void CRTKRover::stop()
{
    qDebug() << "Stopping services...";

    if (m_casterReader) {
        m_casterReader->stop();
    }
    if (m_serialCom) {
        m_serialCom->stop();
    }
    // Objects are deleted automatically by QObject parent-child mechanism
    qDebug() << "Services stopped.";
}

void CRTKRover::onNmeaMessage(const QString &message)
{
    m_gpsData.parse_NMEA(message);
    // m_gpsData.print(); // This is now handled by OutputHandler if configured to stdout

    // Check if we have a fix and haven't detected the mount point yet
    if (m_gpsData.hasFix() && !m_mountPointDetected) {
        onGpsFixAcquired();
    }
}

void CRTKRover::onGpsFixAcquired()
{
    m_mountPointDetected = true;
    qDebug() << "GPS fix acquired. "<<m_gpsData.latitude()<<"/"<<m_gpsData.longitude()<<" Detecting closest mount point...";
    m_mountpoint = detectMountPoint();
    qDebug() << "Using mount point:" << m_mountpoint;
    m_casterReader->start(m_mountpoint);
}

QString CRTKRover::detectMountPoint()
{
    // This function now assumes m_gpsData has a valid fix
    return m_casterReader->getAutoMountPoint(m_gpsData.latitude(), m_gpsData.longitude());
}
