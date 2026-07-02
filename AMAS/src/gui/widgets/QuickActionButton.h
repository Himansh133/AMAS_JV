#ifndef AMAS_QUICKACTIONBUTTON_H
#define AMAS_QUICKACTIONBUTTON_H

#include <QPushButton>
#include <QString>
#include <QStyle>

namespace AMAS {

class QuickActionButton : public QPushButton {
    Q_OBJECT
public:
    explicit QuickActionButton(const QString &text, QWidget *parent = nullptr);
    explicit QuickActionButton(QStyle::StandardPixmap iconPixmap, const QString &text, QWidget *parent = nullptr);
    ~QuickActionButton() override = default;
};

} // namespace AMAS

#endif // AMAS_QUICKACTIONBUTTON_H
