#include "ThemesPanel.h"

#include "ThemeEditorDialog.h"
#include "ThemePreviewWidget.h"

#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

constexpr int kCardW = 230;
constexpr int kCardH = 170;
constexpr int kCardSpacing = 14;

// Card visual de um tema: preview + nome + (em uso) ring.
class ThemeCard : public QFrame {
public:
    ThemeCard(const Theme::MiraTheme& theme, bool inUse, QWidget* parent = nullptr)
        : QFrame(parent), m_id(theme.id), m_inUse(inUse), m_selected(false)
    {
        setObjectName(QStringLiteral("themeCard"));
        setFrameShape(QFrame::NoFrame);
        setFixedSize(kCardW, kCardH);
        setCursor(Qt::PointingHandCursor);

        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(8, 8, 8, 8);
        lay->setSpacing(6);

        m_preview = new ThemePreviewWidget(this);
        m_preview->setTheme(theme);
        m_preview->setMinimumHeight(110);
        m_preview->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        lay->addWidget(m_preview, 1);

        auto* nameRow = new QHBoxLayout;
        nameRow->setContentsMargins(0, 0, 0, 0);
        nameRow->setSpacing(6);
        m_nameLabel = new QLabel(theme.name, this);
        m_nameLabel->setObjectName(QStringLiteral("themeCardName"));
        m_nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        nameRow->addWidget(m_nameLabel, 1);
        if (inUse) {
            auto* badge = new QLabel(tr("em uso"), this);
            badge->setObjectName(QStringLiteral("themeCardBadge"));
            badge->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            nameRow->addWidget(badge);
        }
        lay->addLayout(nameRow);

        refreshStyle();
    }

    QString themeId() const { return m_id; }
    bool inUse() const { return m_inUse; }

    void setSelected(bool s) {
        if (m_selected == s) return;
        m_selected = s;
        refreshStyle();
    }
    bool isSelected() const { return m_selected; }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            emit_clicked();
        }
        QFrame::mousePressEvent(event);
    }
    void mouseDoubleClickEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            emit_doubleClicked();
        }
        QFrame::mouseDoubleClickEvent(event);
    }

private:
    // Q_SIGNALS via parent: usamos parent dispatch ao invés de signals (QFrame não tem MOC aqui).
    // Pra simplicidade, usamos QEvent::Type custom. Mas é mais simples: callback via parent.
    // Vou usar property + custom event no parent.
    void emit_clicked();
    void emit_doubleClicked();

    void refreshStyle()
    {
        QString border = m_selected
            ? Theme::accentDefault()
            : (m_inUse ? Theme::accentSuccess() : Theme::panelBorder());
        const int bw = (m_selected || m_inUse) ? 2 : 1;
        setStyleSheet(QStringLiteral(
            "#themeCard { background: %1; border: %2px solid %3; border-radius: 8px; }"
            "#themeCard:hover { border-color: %4; }"
            "#themeCardName { color: %5; font-size: 12px; font-weight: 500; background: transparent; }"
            "#themeCardBadge { color: %6; font-size: 10px; background: %7; "
            "  padding: 2px 6px; border-radius: 8px; font-weight: 600; }"
        ).arg(
            Theme::panelBackground(),
            QString::number(bw),
            border,
            Theme::accentDefault(),
            Theme::textPrimary(),
            Theme::accentSuccess(),
            Theme::accentSuccessSoft()
        ));
    }

    QString m_id;
    bool m_inUse;
    bool m_selected;
    ThemePreviewWidget* m_preview;
    QLabel* m_nameLabel;
};

// Eventos custom pra propagar clicks do card ao painel pai (evita Q_OBJECT no card).
constexpr QEvent::Type kCardClickEvent = static_cast<QEvent::Type>(QEvent::User + 1101);
constexpr QEvent::Type kCardDoubleClickEvent = static_cast<QEvent::Type>(QEvent::User + 1102);

class CardEvent : public QEvent {
public:
    CardEvent(QEvent::Type t, const QString& id) : QEvent(t), themeId(id) {}
    QString themeId;
};

void ThemeCard::emit_clicked()
{
    QApplication::postEvent(parent(), new CardEvent(kCardClickEvent, m_id));
}
void ThemeCard::emit_doubleClicked()
{
    QApplication::postEvent(parent(), new CardEvent(kCardDoubleClickEvent, m_id));
}

// Container do grid que captura os custom events dos cards e despacha ao painel.
class CardGrid : public QWidget {
public:
    using ClickHandler = std::function<void(const QString&)>;
    using DoubleClickHandler = std::function<void(const QString&)>;

    CardGrid(QWidget* parent = nullptr) : QWidget(parent) {}
    void setHandlers(ClickHandler c, DoubleClickHandler d) {
        m_click = std::move(c);
        m_double = std::move(d);
    }

protected:
    bool event(QEvent* e) override {
        if (e->type() == kCardClickEvent) {
            auto* ce = static_cast<CardEvent*>(e);
            if (m_click) m_click(ce->themeId);
            return true;
        }
        if (e->type() == kCardDoubleClickEvent) {
            auto* ce = static_cast<CardEvent*>(e);
            if (m_double) m_double(ce->themeId);
            return true;
        }
        return QWidget::event(e);
    }

private:
    ClickHandler m_click;
    DoubleClickHandler m_double;
};

} // namespace

ThemesPanel::ThemesPanel(QWidget* parent)
    : QDialog(parent)
    , m_tabs(nullptr)
    , m_bundledScroll(nullptr)
    , m_customScroll(nullptr)
    , m_bundledGrid(nullptr)
    , m_customGrid(nullptr)
    , m_applyButton(nullptr)
    , m_newButton(nullptr)
    , m_duplicateButton(nullptr)
    , m_editButton(nullptr)
    , m_deleteButton(nullptr)
    , m_closeButton(nullptr)
    , m_selectionInfo(nullptr)
{
    setObjectName(QStringLiteral("themesPanel"));
    setWindowTitle(tr("Temas"));
    setModal(false);
    resize(720, 580);

    buildUi();
    applyDialogStyle();

    connect(Theme::Manager::instance(), &Theme::Manager::themeChanged,
            this, &ThemesPanel::onThemeChanged);
    connect(Theme::Manager::instance(), &Theme::Manager::customThemesChanged,
            this, &ThemesPanel::onCustomThemesChanged);

    // Seleciona o tema em uso por padrão.
    const QString currentId = Theme::Manager::instance()->current().id;
    m_selectedId = currentId;
    rebuildGrids();
    updateButtonsState();
    if (Theme::Manager::instance()->isCustom(currentId)) {
        m_tabs->setCurrentIndex(TabCustom);
    }
}

void ThemesPanel::buildUi()
{
    auto* main = new QVBoxLayout(this);
    main->setContentsMargins(16, 16, 16, 16);
    main->setSpacing(12);

    // ---- Tabs com botões de ação como cornerWidget (top-right) ----
    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName(QStringLiteral("themesPanelTabs"));

    auto buildScrollWithGrid = [this](QScrollArea*& scroll, QWidget*& grid) {
        scroll = new QScrollArea(m_tabs);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        auto* gridContainer = new CardGrid(scroll);
        gridContainer->setHandlers(
            [this](const QString& id) { selectId(id); },
            [this](const QString& id) { selectId(id); onApplyClicked(); }
        );
        grid = gridContainer;
        scroll->setWidget(grid);
    };
    buildScrollWithGrid(m_bundledScroll, m_bundledGrid);
    buildScrollWithGrid(m_customScroll, m_customGrid);
    m_tabs->addTab(m_bundledScroll, tr("Padrão"));
    m_tabs->addTab(m_customScroll, tr("Personalizado"));
    connect(m_tabs, &QTabWidget::currentChanged, this, &ThemesPanel::onTabChanged);

    // Corner widget: barra horizontal de ações alinhada com as abas.
    auto* corner = new QWidget(m_tabs);
    auto* cornerLayout = new QHBoxLayout(corner);
    cornerLayout->setContentsMargins(0, 4, 6, 0);
    cornerLayout->setSpacing(6);
    m_newButton = new QPushButton(tr("Novo"), corner);
    m_newButton->setObjectName(QStringLiteral("themesPanelActionBtn"));
    m_duplicateButton = new QPushButton(tr("Duplicar"), corner);
    m_duplicateButton->setObjectName(QStringLiteral("themesPanelActionBtn"));
    m_editButton = new QPushButton(tr("Editar"), corner);
    m_editButton->setObjectName(QStringLiteral("themesPanelActionBtn"));
    m_deleteButton = new QPushButton(tr("Excluir"), corner);
    m_deleteButton->setObjectName(QStringLiteral("themesPanelActionBtn"));
    cornerLayout->addWidget(m_newButton);
    cornerLayout->addWidget(m_duplicateButton);
    cornerLayout->addWidget(m_editButton);
    cornerLayout->addWidget(m_deleteButton);
    m_tabs->setCornerWidget(corner, Qt::TopRightCorner);

    main->addWidget(m_tabs, 1);

    // ---- Footer: info da seleção (esquerda) + ações finais (direita) ----
    auto* footer = new QHBoxLayout;
    footer->setSpacing(12);
    m_selectionInfo = new QLabel(this);
    m_selectionInfo->setObjectName(QStringLiteral("themesPanelSelectionInfo"));
    m_selectionInfo->setWordWrap(true);
    footer->addWidget(m_selectionInfo, 1);

    m_closeButton = new QPushButton(tr("Fechar"), this);
    m_closeButton->setObjectName(QStringLiteral("themesPanelCloseBtn"));
    m_applyButton = new QPushButton(tr("Aplicar"), this);
    m_applyButton->setObjectName(QStringLiteral("themesPanelApplyBtn"));
    m_applyButton->setDefault(true);
    footer->addWidget(m_closeButton);
    footer->addWidget(m_applyButton);
    main->addLayout(footer);

    connect(m_applyButton, &QPushButton::clicked, this, &ThemesPanel::onApplyClicked);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_newButton, &QPushButton::clicked, this, &ThemesPanel::onNewClicked);
    connect(m_duplicateButton, &QPushButton::clicked, this, &ThemesPanel::onDuplicateClicked);
    connect(m_editButton, &QPushButton::clicked, this, &ThemesPanel::onEditClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &ThemesPanel::onDeleteClicked);
}

ThemesPanel::Tab ThemesPanel::activeTab() const
{
    return static_cast<Tab>(m_tabs->currentIndex());
}

bool ThemesPanel::selectedIsCustom() const
{
    return Theme::Manager::instance()->isCustom(m_selectedId);
}

void ThemesPanel::onTabChanged(int /*index*/)
{
    // Quando muda de aba, reseta a seleção pro tema em uso se ele for da nova aba,
    // senão pro primeiro da aba.
    auto* mgr = Theme::Manager::instance();
    const QString currentId = mgr->current().id;
    if (activeTab() == TabBundled) {
        const auto bundled = mgr->bundledThemes();
        bool currentIsBundled = false;
        for (const auto& t : bundled) if (t.id == currentId) { currentIsBundled = true; break; }
        m_selectedId = currentIsBundled ? currentId : (bundled.isEmpty() ? QString() : bundled.first().id);
    } else {
        const auto custom = mgr->customThemes();
        bool currentIsCustom = false;
        for (const auto& t : custom) if (t.id == currentId) { currentIsCustom = true; break; }
        m_selectedId = currentIsCustom ? currentId : (custom.isEmpty() ? QString() : custom.first().id);
    }
    rebuildGrids();
    updateButtonsState();
}

void ThemesPanel::selectId(const QString& id)
{
    if (m_selectedId == id) return;
    m_selectedId = id;
    // Atualiza visual usando a lista de cards mantida em rebuildOneGrid.
    for (auto& ptr : m_cards) {
        if (auto* frame = ptr.data()) {
            auto* card = static_cast<ThemeCard*>(frame);
            card->setSelected(card->themeId() == m_selectedId);
        }
    }
    updateButtonsState();
}

void ThemesPanel::updateButtonsState()
{
    auto* mgr = Theme::Manager::instance();
    const bool hasSel = !m_selectedId.isEmpty();
    const bool isCustom = hasSel && mgr->isCustom(m_selectedId);
    const bool isCurrent = hasSel && (m_selectedId == mgr->current().id);

    m_applyButton->setEnabled(hasSel && !isCurrent);
    m_duplicateButton->setEnabled(hasSel);
    m_editButton->setEnabled(isCustom);
    m_deleteButton->setEnabled(isCustom);

    // Info textual — inline no footer
    if (hasSel) {
        const Theme::MiraTheme* th = nullptr;
        for (const auto& t : mgr->available()) if (t.id == m_selectedId) { th = &t; break; }
        if (th) {
            const QString sep = QStringLiteral(" <span style='color:%1'>·</span> ").arg(Theme::textMuted());
            QString msg = QStringLiteral("<b>%1</b>").arg(th->name);
            if (isCurrent) {
                msg += sep + QStringLiteral("<span style='color:%1'>%2</span>")
                    .arg(Theme::accentSuccess(), tr("Em uso"));
            } else {
                msg += sep + QStringLiteral("<span style='color:%1'>%2</span>")
                    .arg(Theme::textMuted(), isCustom ? tr("Personalizado") : tr("Padrão"));
            }
            if (!th->backgroundImage.isEmpty()) {
                msg += sep + QStringLiteral("<span style='color:%1'>%2</span>")
                    .arg(Theme::textMuted(), tr("Com imagem"));
            }
            m_selectionInfo->setText(msg);
        }
    } else {
        m_selectionInfo->setText(QStringLiteral("<i style='color:%1'>%2</i>")
            .arg(Theme::textMuted(), tr("Nenhum tema selecionado")));
    }
}

void ThemesPanel::rebuildGrids()
{
    auto* mgr = Theme::Manager::instance();
    m_cards.clear();
    rebuildOneGrid(m_bundledGrid, mgr->bundledThemes(), false);
    rebuildOneGrid(m_customGrid, mgr->customThemes(), true);
    updateButtonsState();
}

void ThemesPanel::rebuildOneGrid(QWidget* gridContainer, const QList<Theme::MiraTheme>& themes, bool /*custom*/)
{
    if (!gridContainer) return;
    // Limpa filhos.
    if (gridContainer->layout()) {
        QLayoutItem* item;
        while ((item = gridContainer->layout()->takeAt(0))) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete gridContainer->layout();
    }

    auto* gl = new QGridLayout(gridContainer);
    gl->setContentsMargins(8, 12, 8, 8);
    gl->setSpacing(kCardSpacing);
    gl->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    if (themes.isEmpty()) {
        auto* empty = new QLabel(
            tr("Nenhum tema personalizado ainda.\nUse \"Duplicar\" em um tema padrão pra começar."),
            gridContainer);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet(QStringLiteral("color: %1; font-size: 12px;").arg(Theme::textMuted()));
        gl->addWidget(empty, 0, 0);
        return;
    }

    const QString currentId = Theme::Manager::instance()->current().id;
    // 3 colunas por linha — vai esticar conforme o espaço.
    const int cols = 3;
    for (int i = 0; i < themes.size(); ++i) {
        const auto& t = themes.at(i);
        auto* card = new ThemeCard(t, t.id == currentId, gridContainer);
        card->setSelected(t.id == m_selectedId);
        gl->addWidget(card, i / cols, i % cols);
        m_cards.append(QPointer<QFrame>(card));
    }
    // Stretch nas células vazias da última linha pra cards não esticarem.
    gl->setColumnStretch(cols, 1);
    gl->setRowStretch((themes.size() / cols) + 1, 1);
}

void ThemesPanel::onApplyClicked()
{
    if (m_selectedId.isEmpty()) return;
    Theme::Manager::instance()->setCurrent(m_selectedId);
    // onThemeChanged vai rebuildar os grids.
}

void ThemesPanel::onNewClicked()
{
    // Cria custom em branco (baseado no Grafite/bundled[0]).
    auto* mgr = Theme::Manager::instance();
    Theme::MiraTheme base = mgr->bundledThemes().value(0);
    base.bundled = false;
    base.id = mgr->uniqueCustomId();
    base.name = tr("Novo tema");

    ThemeEditorDialog dlg(base, this);
    if (dlg.exec() == QDialog::Accepted) {
        const QString id = mgr->upsertCustom(dlg.theme());
        m_tabs->setCurrentIndex(TabCustom);
        m_selectedId = id;
        rebuildGrids();
    }
}

void ThemesPanel::onDuplicateClicked()
{
    if (m_selectedId.isEmpty()) return;
    auto* mgr = Theme::Manager::instance();
    const Theme::MiraTheme* found = nullptr;
    for (const auto& t : mgr->available()) if (t.id == m_selectedId) { found = &t; break; }
    if (!found) return;

    Theme::MiraTheme copy = *found;
    copy.bundled = false;
    copy.id = mgr->uniqueCustomId();
    copy.name = tr("%1 (cópia)").arg(found->name);

    ThemeEditorDialog dlg(copy, this);
    if (dlg.exec() == QDialog::Accepted) {
        const QString id = mgr->upsertCustom(dlg.theme());
        m_tabs->setCurrentIndex(TabCustom);
        m_selectedId = id;
        rebuildGrids();
    }
}

void ThemesPanel::onEditClicked()
{
    if (!selectedIsCustom()) return;
    auto* mgr = Theme::Manager::instance();
    const Theme::MiraTheme* found = nullptr;
    for (const auto& t : mgr->available()) if (t.id == m_selectedId) { found = &t; break; }
    if (!found) return;

    ThemeEditorDialog dlg(*found, this);
    if (dlg.exec() == QDialog::Accepted) {
        const QString id = mgr->upsertCustom(dlg.theme());
        m_selectedId = id;
        rebuildGrids();
    }
}

void ThemesPanel::onDeleteClicked()
{
    if (!selectedIsCustom()) return;
    const auto answer = QMessageBox::question(
        this, tr("Excluir tema"),
        tr("Excluir este tema personalizado? Esta ação não pode ser desfeita."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    Theme::Manager::instance()->removeCustom(m_selectedId);
    m_selectedId.clear();
    rebuildGrids();
}

void ThemesPanel::onThemeChanged()
{
    rebuildGrids();
    applyDialogStyle();
}

void ThemesPanel::onCustomThemesChanged()
{
    rebuildGrids();
}

void ThemesPanel::applyDialogStyle()
{
    setStyleSheet(QStringLiteral(R"(
        #themesPanel {
            background: %1;
        }
        #themesPanel QLabel {
            color: %2;
            font-size: 12px;
        }
        #themesPanelSelectionInfo {
            font-size: 12px;
            padding: 0 4px;
        }
        #themesPanelTabs::pane {
            border: 1px solid %6;
            border-radius: 8px;
            background: %5;
            top: -1px;
        }
        #themesPanelTabs::tab-bar {
            left: 8px;
        }
        #themesPanelTabs QTabBar::tab {
            background: transparent;
            color: %4;
            padding: 8px 18px;
            border: 1px solid transparent;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            font-size: 12px;
            margin-right: 4px;
        }
        #themesPanelTabs QTabBar::tab:selected {
            background: %5;
            color: %3;
            border-color: %6;
        }
        #themesPanelTabs QTabBar::tab:hover:!selected {
            color: %2;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QScrollArea > QWidget > QWidget {
            background: transparent;
        }
        QPushButton#themesPanelActionBtn {
            background: %5;
            color: %2;
            border: 1px solid %6;
            padding: 5px 12px;
            border-radius: 5px;
            font-size: 11px;
        }
        QPushButton#themesPanelActionBtn:hover {
            background: %7;
            color: %3;
            border-color: %9;
        }
        QPushButton#themesPanelActionBtn:disabled {
            color: %4;
            background: transparent;
            border-color: %8;
        }
        QPushButton#themesPanelCloseBtn, QPushButton#themesPanelApplyBtn {
            background: %5;
            color: %2;
            border: 1px solid %6;
            padding: 7px 22px;
            border-radius: 6px;
            font-size: 12px;
        }
        QPushButton#themesPanelApplyBtn {
            background: %9;
            color: %3;
            border-color: %9;
        }
        QPushButton#themesPanelApplyBtn:hover {
            background: %9;
        }
        QPushButton#themesPanelApplyBtn:disabled {
            background: transparent;
            color: %4;
            border-color: %8;
        }
        QPushButton#themesPanelCloseBtn:hover {
            background: %7;
            color: %3;
        }
    )").arg(
        Theme::appBackground(),     // 1
        Theme::textPrimary(),       // 2
        Theme::textBright(),        // 3
        Theme::textMuted(),         // 4
        Theme::panelBackground(),   // 5
        Theme::panelBorder(),       // 6
        Theme::hoverOverlay(),      // 7
        Theme::subtleBorder(),      // 8
        Theme::accentDefault()      // 9
    ));
}
