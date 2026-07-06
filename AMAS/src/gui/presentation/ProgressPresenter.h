#ifndef AMAS_PROGRESSPRESENTER_H
#define AMAS_PROGRESSPRESENTER_H

#include <QObject>
#include <QThread>
#include "measurement/MeasurementProfile.h"
#include "measurement/MeasurementSession.h"

namespace AMAS {

class MeasurementPresenter;

class ProgressPresenter : public QObject {
    Q_OBJECT
public:
    explicit ProgressPresenter(MeasurementPresenter *parent);
    ~ProgressPresenter() override;

    // Control triggers
    void startMeasurement();
    void pauseMeasurement();
    void resumeMeasurement();
    void stopMeasurement();
    void abortMeasurement();

    // Query parameters
    float getCurrentAngle() const;
    double getCurrentFreq() const;
    double getEstimatedRemainingTime() const;
    QString getStatus() const;
    int getProgress() const;

signals:
    void measurementStarted();
    void measurementProgressUpdated(float progressPercent, const QString &statusMessage);
    void measurementFinished(bool success);
    void logMessageAdded(const QString &msg);

private slots:
    void onProgressUpdated(int progress);
    void onStatusChanged();

private:
    MeasurementPresenter *m_parent;
    QThread *m_workerThread;
};

} // namespace AMAS

#endif // AMAS_PROGRESSPRESENTER_H
