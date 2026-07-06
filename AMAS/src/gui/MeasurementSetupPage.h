#ifndef AMAS_MEASUREMENTSETUPPAGE_H
#define AMAS_MEASUREMENTSETUPPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include "widgets/NoWheelWidgets.h"
#include <QHBoxLayout>
#include <QScrollArea>

#include "measurement/MeasurementProfile.h"

namespace AMAS {

class SetupPresenter;

class MeasurementSetupPage : public QWidget {
    Q_OBJECT
public:
    explicit MeasurementSetupPage(SetupPresenter *presenter, QWidget *parent = nullptr);
    ~MeasurementSetupPage() override = default;

signals:
    void startRequested();

private:
    // Layout sections
    void createMeasTypePanel(QGroupBox *box);
    void createFreqPanel(QGroupBox *box);
    void createCalPanel(QGroupBox *box);
    void createAngularPanel(QGroupBox *box);
    void createOutputPanel(QGroupBox *box);
    void createSummaryPanel(QGroupBox *box);
    void createBottomBar(QHBoxLayout *layout);

    // Dynamic slot updates
    void onMeasTypeChanged(int index);
    void onFreqBandChanged(int index);
    void updateSummary();

    // Section 1 Widgets
    NoWheelComboBox *m_comboMeasType;
    QLabel    *m_lblMeasDesc;

    // Section 2 Widgets
    NoWheelDoubleSpinBox *m_spinStartFreq;
    NoWheelDoubleSpinBox *m_spinStopFreq;
    NoWheelSpinBox       *m_spinPoints;
    NoWheelComboBox      *m_comboBand;

    // Section 3 Widgets
    QLabel      *m_lblCalFile;
    QPushButton *m_btnCalBrowse;
    QLabel      *m_lblCalStatus;
    QLabel      *m_lblCalDate;

    // Section 4 Widgets
    NoWheelDoubleSpinBox *m_spinAzStart;
    NoWheelDoubleSpinBox *m_spinAzStop;
    NoWheelDoubleSpinBox *m_spinAzStep;
    
    NoWheelDoubleSpinBox *m_spinElStart;
    NoWheelDoubleSpinBox *m_spinElStop;
    NoWheelDoubleSpinBox *m_spinElStep;

    NoWheelDoubleSpinBox *m_spinPlStart;
    NoWheelDoubleSpinBox *m_spinPlStop;
    NoWheelDoubleSpinBox *m_spinPlStep;

    // Section 5 Widgets
    QLineEdit   *m_txtOutputFolder;
    QPushButton *m_btnOutputBrowse;
    QCheckBox   *m_chkExportCsv;
    QCheckBox   *m_chkGeneratePdf;
    QCheckBox   *m_chkOverwrite;

    // Section 6 (Summary) Widgets
    QLabel *m_lblSumMeasType;
    QLabel *m_lblSumBand;
    QLabel *m_lblSumPoints;
    QLabel *m_lblSumPositions;
    QLabel *m_lblSumDuration;
    QLabel *m_lblSumStorage;

    // Bottom Buttons
    QPushButton *m_btnLoad;
    QPushButton *m_btnSave;
    QPushButton *m_btnValidate;
    QPushButton *m_btnStart;
    QPushButton *m_btnCancel;

private slots:
    void onSaveClicked();
    void onProfileLoaded(const MeasurementProfile &profile);
    void onCalBrowseClicked();
    void onSetupControlChanged();

private:
    SetupPresenter *m_presenter;
};

} // namespace AMAS

#endif // AMAS_MEASUREMENTSETUPPAGE_H
