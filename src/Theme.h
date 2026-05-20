#pragma once

#include <QString>

// Constantes de tema centralizadas. Quando virar editável, basta trocar pra
// QSettings ou um struct lido de arquivo — os call-sites já estão prontos.
namespace Theme {

inline QString appBackground()      { return QStringLiteral("#08080a"); }
inline QString panelBackground()    { return QStringLiteral("#1c1c22"); }
inline QString panelBorder()        { return QStringLiteral("#52525e"); }
inline QString panelBorderRadius()  { return QStringLiteral("10px"); }

inline QString textPrimary()        { return QStringLiteral("#d8d3c6"); }
inline QString textMuted()          { return QStringLiteral("#8a857a"); }
inline QString textBright()         { return QStringLiteral("#f0e8d8"); }

inline QString hoverOverlay()       { return QStringLiteral("rgba(255,255,255,0.06)"); }
inline QString pressedOverlay()     { return QStringLiteral("rgba(255,255,255,0.04)"); }
inline QString subtleBorder()       { return QStringLiteral("rgba(255,255,255,0.10)"); }

inline QString accentDefault()      { return QStringLiteral("#3a8c7a"); }

inline QString panelQss(const QString& objectName) {
    return QStringLiteral(R"(
        #%1 {
            background: %2;
            border: 1px solid %3;
            border-radius: %4;
        }
    )").arg(objectName, panelBackground(), panelBorder(), panelBorderRadius());
}

} // namespace Theme
