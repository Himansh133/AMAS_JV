#ifndef AMAS_NOWHEELWIDGETS_H
#define AMAS_NOWHEELWIDGETS_H

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QWheelEvent>

namespace AMAS {

class NoWheelSpinBox : public QSpinBox {
    Q_OBJECT
public:
    explicit NoWheelSpinBox(QWidget *parent = nullptr) : QSpinBox(parent) {
        setFocusPolicy(Qt::StrongFocus);
    }
protected:
    void wheelEvent(QWheelEvent *event) override {
        event->ignore();
    }
};

class NoWheelDoubleSpinBox : public QDoubleSpinBox {
    Q_OBJECT
public:
    explicit NoWheelDoubleSpinBox(QWidget *parent = nullptr) : QDoubleSpinBox(parent) {
        setFocusPolicy(Qt::StrongFocus);
    }
protected:
    void wheelEvent(QWheelEvent *event) override {
        event->ignore();
    }
};

class NoWheelComboBox : public QComboBox {
    Q_OBJECT
public:
    explicit NoWheelComboBox(QWidget *parent = nullptr) : QComboBox(parent) {
        setFocusPolicy(Qt::StrongFocus);
    }
protected:
    void wheelEvent(QWheelEvent *event) override {
        event->ignore();
    }
};

} // namespace AMAS

#endif // AMAS_NOWHEELWIDGETS_H
