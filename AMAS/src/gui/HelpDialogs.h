#ifndef AMAS_HELPDIALOGS_H
#define AMAS_HELPDIALOGS_H

#include <QDialog>

namespace AMAS {

class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog() override = default;
};

class UserManualDialog : public QDialog {
    Q_OBJECT
public:
    explicit UserManualDialog(QWidget *parent = nullptr);
    ~UserManualDialog() override = default;
};

class ShortcutsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ShortcutsDialog(QWidget *parent = nullptr);
    ~ShortcutsDialog() override = default;
};

} // namespace AMAS

#endif // AMAS_HELPDIALOGS_H
