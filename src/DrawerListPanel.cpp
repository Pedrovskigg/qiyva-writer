#include "DrawerListPanel.h"
#include "ElementsStore.h"
#include "ProjectModel.h"
#include "Theme.h"

#include <QAction>
#include <QByteArray>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr int kPanelWidth = 260;
}

DrawerListPanel::DrawerListPanel(ProjectModel* model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_titleLabel(nullptr)
    , m_listLayout(nullptr)
    , m_scroll(nullptr)
{
    setObjectName(QStringLiteral("drawerListPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(kPanelWidth);
    setStyleSheet(Theme::panelQss(QStringLiteral("drawerListPanel")));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Header --------------------------------------------------------------
    auto* header = new QWidget(this);
    header->setObjectName(QStringLiteral("drawerListHeader"));
    header->setStyleSheet(QStringLiteral(
        "#drawerListHeader { border-bottom: 1px solid %1; }").arg(Theme::panelBorder()));
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(14, 10, 8, 10);
    headerLayout->setSpacing(4);

    m_titleLabel = new QLabel(tr("Gaveta"), header);
    m_titleLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-family: 'Lora','Crimson Text',serif; font-size: 14px; font-weight: 600;")
        .arg(Theme::textBright()));
    m_titleLabel->setTextFormat(Qt::RichText);
    m_titleLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    connect(m_titleLabel, &QLabel::linkActivated, this, [this](const QString& link) {
        // Links: "root" = raiz da gaveta, ou um folderId
        if (link == QStringLiteral("root")) {
            m_currentFolderId.clear();
        } else {
            m_currentFolderId = link;
        }
        rebuildContents();
    });
    headerLayout->addWidget(m_titleLabel, /*stretch=*/1);

    auto makeIconBtn = [this](const QString& glyph, const QString& tip) {
        auto* b = new QToolButton(this);
        b->setText(glyph);
        b->setToolTip(tip);
        b->setFixedSize(28, 28);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(R"(
            QToolButton {
                background: transparent;
                color: %1;
                border: 1px solid transparent;
                border-radius: 6px;
                font-size: 15px;
            }
            QToolButton:hover {
                background: %2;
                color: %3;
                border-color: %4;
            }
        )").arg(Theme::textMuted(), Theme::hoverOverlay(), Theme::textBright(), Theme::subtleBorder()));
        return b;
    };

    auto* btnAddItem = makeIconBtn(QStringLiteral("+"), tr("Novo item"));
    connect(btnAddItem, &QToolButton::clicked, this, [this]() {
        if (!m_currentKey.isEmpty()) emit newItemRequested(m_currentKey, m_currentFolderId);
    });
    headerLayout->addWidget(btnAddItem);

    auto* btnAddFolder = makeIconBtn(QStringLiteral("▸"), tr("Nova pasta"));
    connect(btnAddFolder, &QToolButton::clicked, this, [this]() {
        if (!m_currentKey.isEmpty()) emit newFolderRequested(m_currentKey, m_currentFolderId);
    });
    headerLayout->addWidget(btnAddFolder);

    auto* btnClose = makeIconBtn(QStringLiteral("×"), tr("Fechar"));
    connect(btnClose, &QToolButton::clicked, this, &DrawerListPanel::closePanel);
    headerLayout->addWidget(btnClose);

    root->addWidget(header);

    // Lista scrollable ----------------------------------------------------
    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("drawerListScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setStyleSheet(QStringLiteral("#drawerListScroll { background: transparent; }"));

    auto* listHost = new QWidget(m_scroll);
    listHost->setStyleSheet(QStringLiteral("background: transparent;"));
    m_listLayout = new QVBoxLayout(listHost);
    m_listLayout->setContentsMargins(8, 8, 8, 8);
    m_listLayout->setSpacing(4);
    m_listLayout->addStretch();
    m_scroll->setWidget(listHost);
    root->addWidget(m_scroll, /*stretch=*/1);

    if (m_model) {
        connect(m_model, &ProjectModel::drawersChanged, this, &DrawerListPanel::onDrawersChanged);
    }

    hide();
}

void DrawerListPanel::setElementsStore(ElementsStore* store) {
    m_elementsStore = store;
    if (store) {
        connect(store, &ElementsStore::changed, this, [this]() { if (isPanelOpen()) rebuildContents(); });
    }
}

void DrawerListPanel::openDrawer(const QString& drawerKey, const QString& folderId) {
    if (m_currentKey != drawerKey) {
        m_currentKey = drawerKey;
        m_currentFolderId.clear();
    }
    if (!folderId.isEmpty()) {
        m_currentFolderId = folderId;
    }
    rebuildContents();
    show();
}

void DrawerListPanel::closePanel() {
    m_currentKey.clear();
    m_currentFolderId.clear();
    hide();
    emit panelClosed();
}

void DrawerListPanel::onDrawersChanged() {
    if (m_currentKey.isEmpty()) return;
    if (m_model && !m_model->findDrawer(m_currentKey)) {
        closePanel();
        return;
    }
    rebuildContents();
}

void DrawerListPanel::enterFolder(const QString& folderId) {
    m_currentFolderId = folderId;
    rebuildContents();
}

void DrawerListPanel::goUpOneLevel() {
    if (m_currentFolderId.isEmpty() || !m_model) return;
    const Drawer* drawer = m_model->findDrawer(m_currentKey);
    if (!drawer) return;
    QString parent;
    for (const auto& f : drawer->folders) {
        if (f.id == m_currentFolderId) { parent = f.parentId; break; }
    }
    m_currentFolderId = parent;
    rebuildContents();
}

void DrawerListPanel::updateBreadcrumb() {
    if (!m_model) return;
    const Drawer* drawer = m_model->findDrawer(m_currentKey);
    if (!drawer) return;

    const QString linkStyle = QStringLiteral(
        "color: %1; text-decoration: none;").arg(Theme::textMuted());
    const QString currentStyle = QStringLiteral(
        "color: %1;").arg(Theme::textBright());
    const QString sepStyle = QStringLiteral(
        "color: %1;").arg(Theme::textMuted());

    QString html;
    const QString drawerTitle = drawer->title.isEmpty() ? tr("Gaveta") : drawer->title;

    if (m_currentFolderId.isEmpty()) {
        html = QStringLiteral("<span style='%1'>%2</span>").arg(currentStyle, drawerTitle.toHtmlEscaped());
    } else {
        html = QStringLiteral("<a href='root' style='%1'>%2</a>").arg(linkStyle, drawerTitle.toHtmlEscaped());
        const QStringList ancestors = ancestorFolderIds(m_currentFolderId);
        for (int i = ancestors.size() - 1; i >= 1; --i) {
            const QString fid = ancestors.at(i);
            html += QStringLiteral(" <span style='%1'>/</span> ").arg(sepStyle);
            html += QStringLiteral("<a href='%1' style='%2'>%3</a>")
                .arg(fid.toHtmlEscaped(), linkStyle, folderTitle(fid).toHtmlEscaped());
        }
        html += QStringLiteral(" <span style='%1'>/</span> ").arg(sepStyle);
        html += QStringLiteral("<span style='%1'>%2</span>")
            .arg(currentStyle, folderTitle(m_currentFolderId).toHtmlEscaped());
    }
    m_titleLabel->setText(html);
}

void DrawerListPanel::rebuildContents() {
    if (!m_listLayout) return;

    while (m_listLayout->count() > 1) {
        QLayoutItem* item = m_listLayout->takeAt(0);
        if (auto* w = item->widget()) w->deleteLater();
        delete item;
    }

    if (!m_model || m_currentKey.isEmpty()) return;
    const Drawer* drawer = m_model->findDrawer(m_currentKey);
    if (!drawer) return;

    updateBreadcrumb();

    int row = 0;
    if (!m_currentFolderId.isEmpty()) {
        auto* back = new QToolButton(this);
        back->setText(QStringLiteral("↑  %1").arg(tr("Voltar")));
        back->setCursor(Qt::PointingHandCursor);
        back->setMinimumHeight(28);
        back->setStyleSheet(QStringLiteral(R"(
            QToolButton {
                background: transparent;
                color: %1;
                border: 1px solid transparent;
                border-radius: 6px;
                padding: 4px 10px;
                text-align: left;
                font-family: 'Lora','Crimson Text',serif;
                font-size: 13px;
                font-style: italic;
            }
            QToolButton:hover { background: %2; color: %3; }
        )").arg(Theme::textMuted(), Theme::hoverOverlay(), Theme::textBright()));
        back->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(back, &QToolButton::clicked, this, &DrawerListPanel::goUpOneLevel);
        m_listLayout->insertWidget(row++, back);
    }

    const bool elementDrawer = drawer->drawerElementType == QStringLiteral("character")
        || drawer->drawerElementType == QStringLiteral("setting")
        || drawer->drawerElementType == QStringLiteral("object");

    int displayedCount = 0;
    // Folders aparecem na lista vertical normal mesmo em element drawer.
    for (const auto& f : drawer->folders) {
        if (f.parentId != m_currentFolderId) continue;
        m_listLayout->insertWidget(row++, makeRow(f.title, /*isFolder=*/true, f.id));
        ++displayedCount;
    }

    if (elementDrawer) {
        // Grid 2-col com cards visuais.
        QHBoxLayout* currentRow = nullptr;
        int colInRow = 0;
        for (const auto& it : drawer->items) {
            if (it.folderId != m_currentFolderId) continue;
            if (colInRow == 0) {
                auto* rowWrap = new QWidget(this);
                currentRow = new QHBoxLayout(rowWrap);
                currentRow->setSpacing(8);
                currentRow->setContentsMargins(0, 0, 0, 0);
                m_listLayout->insertWidget(row++, rowWrap);
            }
            currentRow->addWidget(makeElementCard(it.id, it.title, it.role, it.elementId));
            ++colInRow;
            ++displayedCount;
            if (colInRow >= 2) {
                colInRow = 0;
                currentRow = nullptr;
            }
        }
        if (currentRow) currentRow->addStretch();
    } else {
        for (const auto& it : drawer->items) {
            if (it.folderId != m_currentFolderId) continue;
            m_listLayout->insertWidget(row++, makeRow(it.title, /*isFolder=*/false, it.id));
            ++displayedCount;
        }
    }

    if (displayedCount == 0) {
        m_listLayout->insertWidget(row++, makeEmptyState());
    }
}

namespace {
QPixmap pixFromDataUrl(const QString& dataUrl) {
    if (dataUrl.isEmpty()) return QPixmap();
    const int comma = dataUrl.indexOf(QLatin1Char(','));
    if (comma < 0) return QPixmap();
    const QByteArray raw = QByteArray::fromBase64(dataUrl.mid(comma + 1).toLatin1());
    QPixmap pm;
    pm.loadFromData(raw);
    return pm;
}
}

QWidget* DrawerListPanel::makeElementCard(const QString& itemId, const QString& title, const QString& role, const QString& elementId)
{
    QString imageDataUrl;
    QString resolvedRole = role;
    if (m_elementsStore && !elementId.isEmpty()) {
        if (const Element* e = m_elementsStore->findElement(elementId)) {
            imageDataUrl = e->image;
            if (resolvedRole.isEmpty()) resolvedRole = e->role;
        }
    }
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("elemCard"));
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setCursor(Qt::PointingHandCursor);
    card->setContextMenuPolicy(Qt::CustomContextMenu);
    card->setFixedSize(110, 156);
    card->setStyleSheet(QStringLiteral(R"(
        QFrame#elemCard {
            background: #1a1a1a;
            border: 1px solid #2a2a2a;
            border-radius: 6px;
        }
        QFrame#elemCard:hover {
            background: #1f1f1f;
            border-color: #3a3a3a;
        }
    )"));

    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->setSpacing(4);

    auto* photo = new QLabel(card);
    photo->setFixedSize(96, 96);
    photo->setAlignment(Qt::AlignCenter);
    photo->setStyleSheet(QStringLiteral(
        "background: #161616; border-radius: 4px; color: #6a6558; font-size: 9px;"));
    QPixmap pm = pixFromDataUrl(imageDataUrl);
    if (!pm.isNull()) {
        photo->setPixmap(pm.scaled(96, 96, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else {
        photo->setText(tr("sem foto"));
    }
    lay->addWidget(photo, 0, Qt::AlignHCenter);

    auto* nameLbl = new QLabel(title.isEmpty() ? tr("(sem nome)") : title, card);
    nameLbl->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 11px; font-weight: 600;").arg(Theme::textBright()));
    nameLbl->setAlignment(Qt::AlignHCenter);
    nameLbl->setWordWrap(true);
    lay->addWidget(nameLbl);

    if (!resolvedRole.isEmpty()) {
        auto* roleLbl = new QLabel(resolvedRole.toUpper(), card);
        roleLbl->setStyleSheet(QStringLiteral(
            "color: #6aa3d9; font-size: 9px; font-weight: 700; letter-spacing: 0.5px;"));
        roleLbl->setAlignment(Qt::AlignHCenter);
        lay->addWidget(roleLbl);
    }
    lay->addStretch();

    // Eventos: click → ativa item; right-click → context menu
    const QString drawerKey = m_currentKey;
    card->installEventFilter(this);
    // Vou usar customContextMenuRequested já configurado.
    connect(card, &QWidget::customContextMenuRequested, this, [this, card, itemId](const QPoint& pos) {
        showItemContextMenu(itemId, card->mapToGlobal(pos));
    });
    // Click: mouse press release
    QObject::connect(card, &QObject::destroyed, this, []{});
    card->setProperty("itemId", itemId);
    card->setProperty("drawerKey", drawerKey);

    return card;
}

QWidget* DrawerListPanel::makeRow(const QString& label, bool isFolder, const QString& id) {
    auto* btn = new QToolButton(this);
    btn->setText(QStringLiteral("%1  %2").arg(isFolder ? QStringLiteral("▸") : QStringLiteral("·"), label));
    btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumHeight(32);
    btn->setContextMenuPolicy(Qt::CustomContextMenu);
    btn->setStyleSheet(QStringLiteral(R"(
        QToolButton {
            background: transparent;
            color: %1;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 4px 10px;
            text-align: left;
            font-family: 'Lora','Crimson Text',serif;
            font-size: 14px;
        }
        QToolButton:hover {
            background: %2;
            color: %3;
            border-color: %4;
        }
    )").arg(Theme::textPrimary(), Theme::hoverOverlay(), Theme::textBright(), Theme::subtleBorder()));
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    const QString drawerKey = m_currentKey;
    connect(btn, &QToolButton::clicked, this, [this, isFolder, id, drawerKey]() {
        if (isFolder) {
            enterFolder(id);
        } else {
            emit itemActivated(drawerKey, id);
        }
    });
    connect(btn, &QToolButton::customContextMenuRequested, this, [this, btn, isFolder, id](const QPoint& pos) {
        if (isFolder) {
            showFolderContextMenu(id, btn->mapToGlobal(pos));
        } else {
            showItemContextMenu(id, btn->mapToGlobal(pos));
        }
    });
    return btn;
}

bool DrawerListPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto* w = qobject_cast<QWidget*>(watched);
        if (w && w->objectName() == QStringLiteral("elemCard")) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                const QString itemId = w->property("itemId").toString();
                const QString drawerKey = w->property("drawerKey").toString();
                if (!itemId.isEmpty()) emit itemActivated(drawerKey, itemId);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

QWidget* DrawerListPanel::makeEmptyState() {
    auto* lbl = new QLabel(tr("Vazio. Use + acima pra criar um item."), this);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setStyleSheet(QStringLiteral(
        "color: %1; font-style: italic; padding: 20px 12px;")
        .arg(Theme::textMuted()));
    lbl->setWordWrap(true);
    return lbl;
}

QString DrawerListPanel::folderTitle(const QString& folderId) const {
    if (!m_model || folderId.isEmpty()) return QString();
    const Drawer* drawer = m_model->findDrawer(m_currentKey);
    if (!drawer) return QString();
    for (const auto& f : drawer->folders) {
        if (f.id == folderId) return f.title;
    }
    return QString();
}

QStringList DrawerListPanel::ancestorFolderIds(const QString& folderId) const {
    QStringList chain;
    if (!m_model || folderId.isEmpty()) return chain;
    const Drawer* drawer = m_model->findDrawer(m_currentKey);
    if (!drawer) return chain;

    QString cursor = folderId;
    int safety = 0;
    while (!cursor.isEmpty() && safety++ < 64) {
        chain << cursor;
        bool found = false;
        for (const auto& f : drawer->folders) {
            if (f.id == cursor) { cursor = f.parentId; found = true; break; }
        }
        if (!found) break;
    }
    return chain;
}

void DrawerListPanel::showItemContextMenu(const QString& itemId, const QPoint& globalPos) {
    if (!m_model) return;
    const Drawer* drawer = m_model->findDrawer(m_currentKey);
    if (!drawer) return;

    QMenu menu(this);
    menu.setStyleSheet(QStringLiteral(R"(
        QMenu {
            background: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 6px;
            padding: 4px;
        }
        QMenu::item { padding: 6px 16px; border-radius: 4px; }
        QMenu::item:selected { background: %4; color: %5; }
    )").arg(Theme::panelBackground(), Theme::textPrimary(), Theme::panelBorder(),
           Theme::hoverOverlay(), Theme::textBright()));

    QMenu* moveMenu = menu.addMenu(tr("Mover para"));
    auto* rootAction = moveMenu->addAction(tr("Raiz da gaveta"));
    connect(rootAction, &QAction::triggered, this, [this, itemId]() {
        m_model->moveDrawerItem(m_currentKey, itemId, QString());
    });
    if (!drawer->folders.isEmpty()) moveMenu->addSeparator();
    for (const auto& f : drawer->folders) {
        const QString fid = f.id;
        const QString ftitle = f.title;
        auto* a = moveMenu->addAction(ftitle);
        connect(a, &QAction::triggered, this, [this, itemId, fid]() {
            m_model->moveDrawerItem(m_currentKey, itemId, fid);
        });
    }
    menu.exec(globalPos);
}

void DrawerListPanel::showFolderContextMenu(const QString& folderId, const QPoint& globalPos) {
    if (!m_model) return;
    const Drawer* drawer = m_model->findDrawer(m_currentKey);
    if (!drawer) return;

    QMenu menu(this);
    menu.setStyleSheet(QStringLiteral(R"(
        QMenu {
            background: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 6px;
            padding: 4px;
        }
        QMenu::item { padding: 6px 16px; border-radius: 4px; }
        QMenu::item:selected { background: %4; color: %5; }
    )").arg(Theme::panelBackground(), Theme::textPrimary(), Theme::panelBorder(),
           Theme::hoverOverlay(), Theme::textBright()));

    QMenu* moveMenu = menu.addMenu(tr("Mover pasta para"));
    auto* rootAction = moveMenu->addAction(tr("Raiz da gaveta"));
    connect(rootAction, &QAction::triggered, this, [this, folderId]() {
        m_model->moveDrawerFolder(m_currentKey, folderId, QString());
    });

    // Não permite mover para si mesmo ou pra seus descendentes (cria ciclo).
    QStringList disallowed;
    disallowed << folderId;
    // BFS de descendentes
    QStringList queue; queue << folderId;
    while (!queue.isEmpty()) {
        const QString cur = queue.takeFirst();
        for (const auto& f : drawer->folders) {
            if (f.parentId == cur) {
                disallowed << f.id;
                queue << f.id;
            }
        }
    }

    bool addedSeparator = false;
    for (const auto& f : drawer->folders) {
        if (disallowed.contains(f.id)) continue;
        if (!addedSeparator) { moveMenu->addSeparator(); addedSeparator = true; }
        const QString fid = f.id;
        auto* a = moveMenu->addAction(f.title);
        connect(a, &QAction::triggered, this, [this, folderId, fid]() {
            m_model->moveDrawerFolder(m_currentKey, folderId, fid);
        });
    }
    menu.exec(globalPos);
}
