#pragma once

#include <QString>
#include <QColor>

// Estrutura de dados de um card na lousa. Compat com canvas.json do Mira 1.
struct CanvasCard {
    QString id;
    QString type;            // "note" | "comment" | "image" | "doc" | "character"
    qreal   x       = 0;
    qreal   y       = 0;
    qreal   width   = 200;
    qreal   height  = 160;
    QString content;
    QColor  color   = QColor(QStringLiteral("#f7d070"));
    QString photoDataUrl;    // character
    QString linkedItemId;    // doc, character
    QString linkedDrawerKey; // doc, character
};

struct CanvasConnection {
    QString id;
    QString fromId;
    QString toId;
    QColor  color = QColor(QStringLiteral("#888888"));
    QStringList waypointCardIds;
};

struct CanvasZone {
    QString id;
    qreal   x = 0, y = 0, width = 400, height = 300;
    QString title;
    QColor  color = QColor(QStringLiteral("#4a90d9"));
};
