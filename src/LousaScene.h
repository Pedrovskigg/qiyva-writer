#pragma once

#include "LousaTypes.h"

#include <QColor>
#include <QGraphicsScene>
#include <QList>

class CardItem;

class LousaScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit LousaScene(QObject* parent = nullptr);

    void setCanvasColor(const QColor& color);
    QColor canvasColor() const { return m_color; }

    // Cards
    CardItem* addCard(const CanvasCard& data);
    void      removeCard(const QString& id);
    void      clearCards();
    QList<CanvasCard> allCardData() const;

signals:
    void cardDataChanged();

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    QColor m_color{QStringLiteral("#1a1a2e")};
    QList<CardItem*> m_cards;
};
