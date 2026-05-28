#pragma once

#include <QColor>
#include <QGraphicsScene>

class LousaScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit LousaScene(QObject* parent = nullptr);

    void setCanvasColor(const QColor& color);
    QColor canvasColor() const { return m_color; }

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    QColor m_color{QStringLiteral("#1a1a2e")};
};
