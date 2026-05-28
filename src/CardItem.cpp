#include "CardItem.h"

#include <QColorDialog>
#include <QFontMetrics>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {

// Paleta de cores padrão do post-it (igual ao Mira 1).
const QColor kNotePalette[] = {
    QColor(QStringLiteral("#ffd060")),
    QColor(QStringLiteral("#ffb347")),
    QColor(QStringLiteral("#ff8fab")),
    QColor(QStringLiteral("#ff6b6b")),
    QColor(QStringLiteral("#a8e6cf")),
    QColor(QStringLiteral("#87ceeb")),
    QColor(QStringLiteral("#c9b1ff")),
    QColor(QStringLiteral("#f8f8f8")),
};

// Widget mínimo que hospeda o QTextEdit dentro do proxy.
class CardTextWidget : public QWidget {
public:
    QTextEdit* edit = nullptr;
    explicit CardTextWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_OpaquePaintEvent, false);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setStyleSheet(QStringLiteral("background: transparent;"));
        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(6, 2, 6, 0);
        lay->setSpacing(0);
        edit = new QTextEdit(this);
        edit->setFrameStyle(QFrame::NoFrame);
        edit->setAcceptRichText(false);
        edit->setStyleSheet(QStringLiteral(
            "QTextEdit { background: transparent; border: none; }"));
        edit->setPlaceholderText(QObject::tr("Escreva aqui..."));
        edit->setFont(QFont(QStringLiteral("Segoe UI"), 12));
        lay->addWidget(edit);
    }
};

bool calcIsDark(const QColor& c) {
    return (c.red() * 299 + c.green() * 587 + c.blue() * 114) / 1000 < 128;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────

CardItem::CardItem(const CanvasCard& data, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_data(data)
{
    setFlag(ItemIsMovable, false);       // movimento manual no mouseMoveEvent
    setFlag(ItemSendsGeometryChanges);
    setFlag(ItemIsFocusable);
    setAcceptHoverEvents(true);
    setPos(m_data.x, m_data.y);

    // Proxy + widget de texto
    auto* tw = new CardTextWidget();
    tw->edit->setPlainText(m_data.content);
    m_textEdit = tw->edit;

    m_proxy = new QGraphicsProxyWidget(this);
    m_proxy->setWidget(tw);
    m_proxy->setZValue(1.0);

    updateProxyGeometry();
    applyTextColor();

    // Propaga mudança de texto para m_data.
    connect(m_textEdit, &QTextEdit::textChanged, this, [this]() {
        m_data.content = m_textEdit->toPlainText();
        emit dataChanged(m_data);
    });
}

CanvasCard CardItem::cardData() const
{
    CanvasCard d = m_data;
    const QPointF p = pos();
    d.x = p.x();
    d.y = p.y();
    return d;
}

void CardItem::syncFromData()
{
    setPos(m_data.x, m_data.y);
    prepareGeometryChange();
    updateProxyGeometry();
    applyTextColor();
    update();
}

// ── Geometria ──────────────────────────────────────────────────────────────

QRectF CardItem::boundingRect() const
{
    return QRectF(-kShadow, -kShadow,
                  m_data.width  + kShadow * 2,
                  m_data.height + kShadow * 2);
}

QPainterPath CardItem::shape() const
{
    QPainterPath p;
    p.addRoundedRect(QRectF(0, 0, m_data.width, m_data.height), kRadius, kRadius);
    return p;
}

void CardItem::updateProxyGeometry()
{
    if (!m_proxy) return;
    const qreal pw = m_data.width  - 2.0;
    const qreal ph = m_data.height - kHeaderH - 1.0;
    m_proxy->setGeometry(QRectF(1, kHeaderH + 1, pw, ph));
}

// ── Pintura ────────────────────────────────────────────────────────────────

bool CardItem::isDark() const { return calcIsDark(m_data.color); }

QColor CardItem::contrastColor() const
{
    return isDark() ? QColor(255, 255, 255, 220) : QColor(0, 0, 0, 180);
}

void CardItem::applyTextColor()
{
    if (!m_textEdit) return;
    const QColor tc = contrastColor();
    QPalette pal = m_textEdit->palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    pal.setColor(QPalette::Text, tc);
    pal.setColor(QPalette::PlaceholderText, QColor(tc.red(), tc.green(), tc.blue(), 90));
    m_textEdit->setPalette(pal);
}

void CardItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*)
{
    const qreal w = m_data.width;
    const qreal h = m_data.height;
    const QColor& bg = m_data.color;

    p->setRenderHint(QPainter::Antialiasing);

    // Sombra
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(0, 0, 0, 40));
    p->drawRoundedRect(QRectF(3, 5, w, h), kRadius, kRadius);

    // Fundo do card
    p->setBrush(bg);
    p->drawRoundedRect(QRectF(0, 0, w, h), kRadius, kRadius);

    // Separador do header
    const QColor sep = isDark()
        ? QColor(255, 255, 255, 30) : QColor(0, 0, 0, 18);
    p->setPen(QPen(sep, 1));
    p->drawLine(QPointF(1, kHeaderH), QPointF(w - 1, kHeaderH));

    // ── Dobra de canto (bottom-right) ──
    const qreal f = kFoldSize;
    // Triângulo escuro (sombra da dobra)
    QPolygonF back;
    back << QPointF(w - f, h) << QPointF(w, h) << QPointF(w, h - f);
    p->setPen(Qt::NoPen);
    p->setBrush(isDark() ? QColor(0, 0, 0, 70) : QColor(0, 0, 0, 36));
    p->drawPolygon(back);
    // Triângulo claro (face da dobra)
    QPolygonF face;
    face << QPointF(w - f, h) << QPointF(w, h - f) << QPointF(w - f, h - f);
    p->setBrush(isDark() ? QColor(255, 255, 255, 18) : QColor(255, 255, 255, 60));
    p->drawPolygon(face);
    // Linha de vinco
    p->setPen(QPen(isDark() ? QColor(255, 255, 255, 30) : QColor(0, 0, 0, 30), 0.8));
    p->drawLine(QPointF(w - f, h), QPointF(w, h - f));

    // ── Header: grip + × ──
    const QColor tc = contrastColor();
    const QColor muted(tc.red(), tc.green(), tc.blue(), 90);

    // Grip (6 pontos)
    p->setPen(Qt::NoPen);
    p->setBrush(muted);
    const qreal gx = 8.0, gy = (kHeaderH - 9.0) / 2.0;
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 2; ++col)
            p->drawEllipse(QPointF(gx + col * 4.5, gy + row * 4.5), 1.2, 1.2);

    // Botão ×
    const QColor xCol = m_hoverDelete
        ? QColor(220, 60, 60) : muted;
    p->setPen(QPen(xCol, 1.4, Qt::SolidLine, Qt::RoundCap));
    const qreal xx = w - 13.0, xy = kHeaderH / 2.0, xr = 4.0;
    p->drawLine(QPointF(xx - xr, xy - xr), QPointF(xx + xr, xy + xr));
    p->drawLine(QPointF(xx + xr, xy - xr), QPointF(xx - xr, xy + xr));

    // Handle de resize (canto inf-esq da dobra)
    if (m_hoverResize) {
        p->setPen(Qt::NoPen);
        p->setBrush(muted);
        const qreal hx = w - f - 2, hy = h - 8;
        p->drawRoundedRect(QRectF(hx - 10, hy - 4, 10, 4), 2, 2);
    }
}

// ── Helpers de hit-test ──────────────────────────────────────────────────

bool CardItem::isOnDeleteBtn(const QPointF& p) const
{
    const qreal xx = m_data.width - 13.0, xy = kHeaderH / 2.0;
    return QRectF(xx - 8, xy - 8, 16, 16).contains(p);
}

bool CardItem::isOnResizeZone(const QPointF& p) const
{
    // Zona de resize = canto inf-dir, à esquerda da dobra
    const qreal f  = kFoldSize + 2.0;
    const qreal hz = 14.0;
    return QRectF(m_data.width - f - hz, m_data.height - hz,
                  hz, hz).contains(p);
}

// ── Eventos de mouse ────────────────────────────────────────────────────────

void CardItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) { e->ignore(); return; }

    if (isOnDeleteBtn(e->pos())) {
        emit deleteRequested(m_data.id);
        e->accept();
        return;
    }
    if (isOnResizeZone(e->pos())) {
        m_resizing      = true;
        m_pressScene    = e->scenePos();
        m_pressSize     = QSizeF(m_data.width, m_data.height);
        e->accept();
        return;
    }
    // Drag só a partir do header (y < kHeaderH)
    if (e->pos().y() < kHeaderH) {
        m_dragging        = true;
        m_pressScene      = e->scenePos();
        m_pressItemOrigin = pos();
        setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }
    e->ignore();
}

void CardItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if (m_dragging) {
        const QPointF delta = e->scenePos() - m_pressScene;
        setPos(m_pressItemOrigin + delta);
        e->accept();
        return;
    }
    if (m_resizing) {
        const QPointF delta = e->scenePos() - m_pressScene;
        const qreal nw = qMax(kMinW, m_pressSize.width()  + delta.x());
        const qreal nh = qMax(kMinH, m_pressSize.height() + delta.y());
        prepareGeometryChange();
        m_data.width  = nw;
        m_data.height = nh;
        updateProxyGeometry();
        update();
        e->accept();
        return;
    }
    e->ignore();
}

void CardItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    if (m_dragging || m_resizing) {
        m_dragging  = false;
        m_resizing  = false;
        setCursor(Qt::ArrowCursor);
        // Persiste posição/tamanho
        const QPointF p = pos();
        m_data.x = p.x(); m_data.y = p.y();
        emit dataChanged(m_data);
        e->accept();
        return;
    }
    e->ignore();
}

// ── Hover ────────────────────────────────────────────────────────────────────

void CardItem::hoverMoveEvent(QGraphicsSceneHoverEvent* e)
{
    const QPointF lp = e->pos();
    const bool onDel    = isOnDeleteBtn(lp);
    const bool onResize = isOnResizeZone(lp);
    const bool onHeader = lp.y() < kHeaderH;

    if (onDel != m_hoverDelete || onResize != m_hoverResize) {
        m_hoverDelete = onDel;
        m_hoverResize = onResize;
        update();
    }
    if (onDel)         setCursor(Qt::PointingHandCursor);
    else if (onResize) setCursor(Qt::SizeFDiagCursor);
    else if (onHeader) setCursor(Qt::OpenHandCursor);
    else               setCursor(Qt::ArrowCursor);
}

void CardItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
    if (m_hoverDelete || m_hoverResize) {
        m_hoverDelete = false;
        m_hoverResize = false;
        update();
    }
    setCursor(Qt::ArrowCursor);
}

// ── Context menu (trocar cor) ────────────────────────────────────────────────

void CardItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
    QMenu menu;
    menu.setTitle(tr("Cor do card"));

    // Swatches da paleta
    for (const QColor& c : kNotePalette) {
        auto* act = menu.addAction(QString());
        act->setIcon(QIcon()); // sem ícone — usa setData para recuperar cor
        // Cria um pixmap colorido para o ícone
        QPixmap px(14, 14);
        px.fill(c);
        act->setIcon(QIcon(px));
        act->setData(c.name());
    }
    menu.addSeparator();
    auto* custom = menu.addAction(tr("Cor personalizada..."));

    QAction* chosen = menu.exec(e->screenPos());
    if (!chosen) return;
    QColor newColor;
    if (chosen == custom) {
        newColor = QColorDialog::getColor(m_data.color, nullptr, tr("Cor do card"));
        if (!newColor.isValid()) return;
    } else {
        newColor = QColor(chosen->data().toString());
        if (!newColor.isValid()) return;
    }
    prepareGeometryChange();
    m_data.color = newColor;
    applyTextColor();
    update();
    emit dataChanged(m_data);
}
