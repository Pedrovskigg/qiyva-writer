#pragma once

#include "LousaTypes.h"

#include <QGraphicsObject>
#include <QColor>
#include <QSizeF>
#include <QPointF>

class QGraphicsProxyWidget;
class QTextEdit;

class CardItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit CardItem(const CanvasCard& data, QGraphicsItem* parent = nullptr);

    CanvasCard cardData() const;

    QRectF      boundingRect() const override;
    QPainterPath shape()        const override;
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

    // Chamado após mudança de cor/tamanho externas (load).
    void syncFromData();

    static constexpr qreal kHeaderH  = 26.0;
    static constexpr qreal kFoldSize = 20.0;
    static constexpr qreal kRadius   =  8.0;
    static constexpr qreal kShadow   =  6.0;
    static constexpr qreal kMinW     = 120.0;
    static constexpr qreal kMinH     =  80.0;

signals:
    void dataChanged(const CanvasCard& data);
    void deleteRequested(const QString& id);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* e)   override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e)    override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* e)    override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e)   override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* e) override;

private:
    bool    isOnDeleteBtn(const QPointF& p) const;
    bool    isOnResizeZone(const QPointF& p) const;
    void    applyTextColor();
    void    updateProxyGeometry();
    QColor  contrastColor() const;
    bool    isDark() const;

    CanvasCard            m_data;
    QGraphicsProxyWidget* m_proxy    = nullptr;
    QTextEdit*            m_textEdit = nullptr;

    // Estado de interação
    bool   m_dragging    = false;
    bool   m_resizing    = false;
    QPointF m_pressScene;
    QPointF m_pressItemOrigin; // pos() no início do drag
    QSizeF  m_pressSize;
    bool   m_hoverDelete = false;
    bool   m_hoverResize = false;
};
