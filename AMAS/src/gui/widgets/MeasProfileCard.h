#ifndef AMAS_MEASPROFILECARD_H
#define AMAS_MEASPROFILECARD_H

#include <QFrame>
#include <QLabel>
#include <QString>

namespace AMAS {

class MeasProfileCard : public QFrame {
    Q_OBJECT
public:
    explicit MeasProfileCard(QWidget *parent = nullptr);
    ~MeasProfileCard() override = default;

    // Set configuration display values
    void setProfile(const QString &band, const QString &calibration, int points);

private:
    QLabel *m_lblTitle;
    QLabel *m_lblBandVal;
    QLabel *m_lblCalVal;
    QLabel *m_lblPointsVal;
};

} // namespace AMAS

#endif // AMAS_MEASPROFILECARD_H
