#include "QuickActionButton.h"
#include <QStyle>

namespace AMAS {

QuickActionButton::QuickActionButton(const QString &text, QWidget *parent)
    : QPushButton(parent)
{
    setText(text);
    
    setMinimumWidth(160);
    setMinimumHeight(80);

    // Apply custom QSS for action tiles
    setStyleSheet(
        "QPushButton { "
        "  background-color: #2D2D30; "
        "  border: 1px solid #3F3F46; "
        "  border-radius: 8px; "
        "  padding: 12px 10px; "
        "  font-family: 'Segoe UI', Arial, sans-serif; "
        "  font-size: 13px; "
        "  font-weight: 500; "
        "  color: #FFFFFF; "
        "  text-align: center; "
        "} "
        "QPushButton:hover { "
        "  background-color: #3F3F46; "
        "  border-color: #007ACC; "
        "} "
        "QPushButton:pressed { "
        "  background-color: #007ACC; "
        "  border-color: #007ACC; "
        "} "
    );
}

QuickActionButton::QuickActionButton(QStyle::StandardPixmap iconPixmap, const QString &text, QWidget *parent)
    : QPushButton(parent)
{
    setText(text);
    
    setMinimumWidth(160);
    setMinimumHeight(80);

    // Load standard Qt icon
    setIcon(style()->standardIcon(iconPixmap));
    setIconSize(QSize(22, 22));

    // Apply custom QSS for action tiles
    setStyleSheet(
        "QPushButton { "
        "  background-color: #2D2D30; "
        "  border: 1px solid #3F3F46; "
        "  border-radius: 8px; "
        "  padding: 12px 10px; "
        "  font-family: 'Segoe UI', Arial, sans-serif; "
        "  font-size: 13px; "
        "  font-weight: 500; "
        "  color: #FFFFFF; "
        "  text-align: center; "
        "} "
        "QPushButton:hover { "
        "  background-color: #3F3F46; "
        "  border-color: #007ACC; "
        "} "
        "QPushButton:pressed { "
        "  background-color: #007ACC; "
        "  border-color: #007ACC; "
        "} "
    );
}

} // namespace AMAS
