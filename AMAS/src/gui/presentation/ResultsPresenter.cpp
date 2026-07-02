#include "ResultsPresenter.h"
#include "MeasurementPresenter.h"

namespace AMAS {

ResultsPresenter::ResultsPresenter(MeasurementPresenter *parent)
    : QObject(parent)
    , m_parent(parent)
{
}

QStringList ResultsPresenter::getCompletedSessions() const {
    return { "Session_001", "Session_002" };
}

bool ResultsPresenter::loadSession(const QString &sessionFolder, MeasurementSession &outSession) {
    outSession.sessionName = sessionFolder.toStdString();
    outSession.timestamp = "2026-07-01 10:15";
    outSession.bandName = "8-12 GHz";
    outSession.calibrationFile = "calchamber8_12ghz.sta";
    outSession.outputFolder = sessionFolder.toStdString();
    outSession.metadata.operatorName = "Operator-01";
    outSession.metadata.notes = "Coordinated horn antenna scan.";

    if (sessionFolder.contains("001")) {
        outSession.measurementType = "Gain";
        ProcessedMeasurementPoint p1; p1.angleDeg = 0.0f;  p1.frequencyHz = 8.0e9;  p1.magnitudeDb = -12.4; p1.phaseDeg = 45.2;  p1.returnLossDb = 12.4; p1.vswr = 1.6;
        ProcessedMeasurementPoint p2; p2.angleDeg = 10.0f; p2.frequencyHz = 9.0e9;  p2.magnitudeDb = -15.8; p2.phaseDeg = -22.1; p2.returnLossDb = 15.8; p2.vswr = 1.4;
        ProcessedMeasurementPoint p3; p3.angleDeg = 20.0f; p3.frequencyHz = 10.0e9; p3.magnitudeDb = -22.1; p3.phaseDeg = -88.4; p3.returnLossDb = 22.1; p3.vswr = 1.1;
        outSession.results = { p1, p2, p3 };
    } else {
        outSession.measurementType = "RadiationPattern";
        ProcessedMeasurementPoint p1; p1.angleDeg = 0.0f;  p1.frequencyHz = 12.0e9; p1.magnitudeDb = -14.1; p1.phaseDeg = 155.1; p1.returnLossDb = 14.1; p1.vswr = 1.5;
        ProcessedMeasurementPoint p2; p2.angleDeg = 40.0f; p2.frequencyHz = 14.0e9; p2.magnitudeDb = -18.2; p2.phaseDeg = 120.3; p2.returnLossDb = 18.2; p2.vswr = 1.3;
        outSession.results = { p1, p2 };
    }

    emit sessionLoaded(sessionFolder);
    return true;
}

} // namespace AMAS
