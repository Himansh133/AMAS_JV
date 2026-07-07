#ifndef AMAS_THEMEMANAGER_H
#define AMAS_THEMEMANAGER_H

#include <QString>
#include <QApplication>

namespace AMAS {

class ThemeManager {
public:
    // Generate the complete unified stylesheet string
    static QString getStyleSheet();

    // Apply the theme globally to the QApplication instance
    static void applyTheme(QApplication *app, const QString &themeName = "Dark");
};

} // namespace AMAS

#endif // AMAS_THEMEMANAGER_H
