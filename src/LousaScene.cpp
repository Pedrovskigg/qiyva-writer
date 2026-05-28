#include "LousaScene.h"

#include "CardItem.h"

#include <QPainter>
#include <algorithm>
#include <cmath>

LousaScene::LousaScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setSceneRect(-12000, -12000, 24000, 24000);
}

void LousaScene::setCanvasColor(const QColor& color)
{
    if (m_color == color) return;
    m_color = color;
    update();
}

void LousaScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    painter->fillRect(rect, m_color);

    // Grid de pontos sutis — clareia levemente a cor do canvas.
    constexpr qreal kSpacing = 32.0;
    const int r = m_color.red()   + (255 - m_color.red())   / 6;
    const int g = m_color.green() + (255 - m_color.green()) / 6;
    const int b = m_color.blue()  + (255 - m_color.blue())  / 6;
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(r, g, b));

    const qreal x0 = std::floor(rect.left()   / kSpacing) * kSpacing;
    const qreal y0 = std::floor(rect.top()    / kSpacing) * kSpacing;
    for (qreal x = x0; x <= rect.right();  x += kSpacing)
        for (qreal y = y0; y <= rect.bottom(); y += kSpacing)
            painter->drawEllipse(QPointF(x, y), 1.2, 1.2);
}

CardItem* LousaScene::addCard(const CanvasCard& data)
{
    auto* item = new CardItem(data);
    addItem(item);
    m_cards.append(item);
    connect(item, &CardItem::dataChanged, this, &LousaScene::cardDataChanged);
    connect(item, &CardItem::deleteRequested, this, [this](const QString& id) {
        removeCard(id);
    });
    return item;
}

void LousaScene::removeCard(const QString& id)
{
    for (int i = 0; i < m_cards.size(); ++i) {
        if (m_cards[i]->cardData().id == id) {
            removeItem(m_cards[i]);
            delete m_cards[i];
            m_cards.removeAt(i);
            emit cardDataChanged();
            return;
        }
    }
}

void LousaScene::clearCards()
{
    for (auto* c : m_cards) {
        removeItem(c);
        delete c;
    }
    m_cards.clear();
}

QList<CanvasCard> LousaScene::allCardData() const
{
    QList<CanvasCard> out;
    for (const auto* c : m_cards) out.append(c->cardData());
    return out;
}
