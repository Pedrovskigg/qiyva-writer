#include "LeftBar.h"
#include "ProjectModel.h"
#include "Theme.h"

#include <QFrame>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr int kBarWidth = 56;
constexpr int kBtnSize = 40;

QString buttonQss(const QString& backgroundColor = QString()) {
    const QString bg = backgroundColor.isEmpty() ? QStringLiteral("transparent") : backgroundColor;
    const QString hoverBg = backgroundColor.isEmpty()
        ? Theme::hoverOverlay()
        : backgroundColor;
    return QStringLiteral(R"(
        QToolButton {
            background: %1;
            color: %3;
            border: 1px solid transparent;
            border-radius: 8px;
            font-family: 'Lora','Crimson Text',serif;
            font-size: 17px;
            font-weight: 600;
        }
        QToolButton:hover {
            background: %2;
            border-color: %4;
            color: %5;
        }
        QToolButton:pressed {
            background: %6;
        }
    )").arg(bg, hoverBg, Theme::textPrimary(), Theme::subtleBorder(), Theme::textBright(), Theme::pressedOverlay());
}
} // namespace

LeftBar::LeftBar(ProjectModel* model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_drawerLayout(nullptr)
    , m_rootLayout(nullptr)
{
    setObjectName(QStringLiteral("leftBar"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(kBarWidth);
    setStyleSheet(Theme::panelQss(QStringLiteral("leftBar")));

    m_rootLayout = new QVBoxLayout(this);
    m_rootLayout->setContentsMargins(6, 10, 6, 10);
    m_rootLayout->setSpacing(6);

    m_rootLayout->addWidget(makeFixedButton(QStringLiteral("i"), tr("Informações do projeto"), Info));
    m_rootLayout->addWidget(makeFixedButton(QStringLiteral("L"), tr("Lousa de planejamento"), Whiteboard));
    m_rootLayout->addWidget(makeFixedButton(QStringLiteral("M"), tr("Mapa narrativo"), Map));
    m_rootLayout->addWidget(makeFixedButton(QStringLiteral("T"), tr("Manuscritos"), Manuscripts));
    m_rootLayout->addWidget(makeFixedButton(QStringLiteral("C"), tr("Consistência narrativa"), Consistency));

    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet(QStringLiteral("color: %1; background: %1; max-height: 1px; border: none;").arg(Theme::panelBorder()));
    m_rootLayout->addWidget(separator);

    m_drawerLayout = new QVBoxLayout();
    m_drawerLayout->setContentsMargins(0, 0, 0, 0);
    m_drawerLayout->setSpacing(6);
    m_rootLayout->addLayout(m_drawerLayout);

    m_rootLayout->addStretch();
    m_rootLayout->addWidget(makeNewDrawerButton());

    if (m_model) {
        connect(m_model, &ProjectModel::drawersChanged, this, &LeftBar::rebuildDrawerButtons);
        connect(m_model, &ProjectModel::loaded, this, &LeftBar::rebuildDrawerButtons);
        rebuildDrawerButtons();
    }
}

void LeftBar::setMirrored(bool mirrored) {
    if (m_mirrored == mirrored) return;
    m_mirrored = mirrored;
    applyMirrorStyle();
}

void LeftBar::applyMirrorStyle() {
    // O lado físico (esquerda/direita) na janela é decidido pelo MainWindow
    // que monta o layout. Aqui o painel tem borda completa (radius), então
    // não há diferença visual entre os dois lados; mantemos o método pra
    // quando precisar inverter alinhamentos internos (tooltips, etc.).
    setStyleSheet(Theme::panelQss(QStringLiteral("leftBar")));
}

QToolButton* LeftBar::makeFixedButton(const QString& placeholderLetter,
                                     const QString& tooltip,
                                     FixedAction action) {
    auto* btn = new QToolButton(this);
    btn->setText(placeholderLetter);
    btn->setToolTip(tooltip);
    btn->setFixedSize(kBtnSize, kBtnSize);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(buttonQss());
    connect(btn, &QToolButton::clicked, this, [this, action]() {
        emit fixedActionTriggered(action);
    });
    return btn;
}

QToolButton* LeftBar::makeDrawerButton(const QString& drawerKey,
                                      const QString& title,
                                      const QString& color) {
    auto* btn = new QToolButton(this);
    const QString letter = title.isEmpty() ? QStringLiteral("?") : title.left(1).toUpper();
    btn->setText(letter);
    btn->setToolTip(title);
    btn->setFixedSize(kBtnSize, kBtnSize);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(buttonQss(color));
    connect(btn, &QToolButton::clicked, this, [this, drawerKey]() {
        emit drawerSelected(drawerKey);
    });
    return btn;
}

QToolButton* LeftBar::makeNewDrawerButton() {
    auto* btn = new QToolButton(this);
    btn->setText(QStringLiteral("+"));
    btn->setToolTip(tr("Nova gaveta"));
    btn->setFixedSize(kBtnSize, kBtnSize);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QStringLiteral(R"(
        QToolButton {
            background: transparent;
            color: %1;
            border: 1px dashed %2;
            border-radius: 8px;
            font-size: 22px;
            font-weight: 300;
        }
        QToolButton:hover {
            color: %3;
            border-color: %4;
            background: %5;
        }
    )").arg(Theme::textMuted(), Theme::panelBorder(), Theme::textBright(), Theme::textMuted(), Theme::pressedOverlay()));
    connect(btn, &QToolButton::clicked, this, &LeftBar::newDrawerRequested);
    return btn;
}

void LeftBar::rebuildDrawerButtons() {
    if (!m_drawerLayout) return;
    QLayoutItem* child;
    while ((child = m_drawerLayout->takeAt(0)) != nullptr) {
        if (auto* w = child->widget()) w->deleteLater();
        delete child;
    }
    if (!m_model) return;
    for (const auto& d : m_model->drawers()) {
        m_drawerLayout->addWidget(makeDrawerButton(d.key, d.title, d.color));
    }
}
