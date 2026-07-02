#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <memory>

namespace AMAS {

class MainWindow;
class MeasurementController;
class IVnaDriver;
class IPositionerDriver;
class CalibrationManager;
class DataLogger;

class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(QObject *parent = nullptr);
    ~Application() override;

    bool initialize();
    void run();

private:
    std::shared_ptr<MeasurementController> m_controller;
    std::shared_ptr<IVnaDriver> m_vna;
    std::shared_ptr<IPositionerDriver> m_positioner;
    std::shared_ptr<CalibrationManager> m_calManager;
    std::shared_ptr<DataLogger> m_logger;

    std::unique_ptr<MainWindow> m_mainWindow;
};

} // namespace AMAS

#endif // APPLICATION_H
