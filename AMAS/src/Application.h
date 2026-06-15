#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

namespace AMAS {

class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(QObject *parent = nullptr);
    ~Application() override = default;

    bool initialize();
    void run();
};

} // namespace AMAS

#endif // APPLICATION_H
