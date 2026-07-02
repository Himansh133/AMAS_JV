#ifndef AMAS_RESULTSPRESENTER_H
#define AMAS_RESULTSPRESENTER_H

#include <QObject>
#include "measurement/MeasurementSession.h"

namespace AMAS {

class MeasurementPresenter;

class ResultsPresenter : public QObject {
    Q_OBJECT
public:
    explicit ResultsPresenter(MeasurementPresenter *parent);
    ~ResultsPresenter() override = default;

    QStringList getCompletedSessions() const;
    bool loadSession(const QString &sessionFolder, MeasurementSession &outSession);

signals:
    void sessionLoaded(const QString &sessionName);

private:
    MeasurementPresenter *m_parent;
};

} // namespace AMAS

#endif // AMAS_RESULTSPRESENTER_H
