#ifndef AMAS_COLORS_H
#define AMAS_COLORS_H

#include <QString>

namespace AMAS {
namespace Theme {
namespace Colors {

// Color Hex Strings (used inside QSS compilation)
inline const QString Background    = "#1E1E1E"; // Main window workspace
inline const QString Surface       = "#252526"; // Menu, statusbar, toolbars
inline const QString Card          = "#2D2D30"; // Panels, forms, headers
inline const QString Border        = "#3F3F46"; // Borders, divider lines
inline const QString TextPrimary   = "#FFFFFF"; // Active text, inputs, titles
inline const QString TextSecondary = "#C8C8C8"; // Secondary labels, captions, units
inline const QString Accent        = "#007ACC"; // Buttons, focus outlines, selection
inline const QString Success       = "#4CAF50"; // Connected / Good status
inline const QString Warning       = "#FFC107"; // In-progress / Settle warning
inline const QString Danger        = "#F44336"; // Error / Abort / Emergency Stop

} // namespace Colors
} // namespace Theme
} // namespace AMAS

#endif // AMAS_COLORS_H
