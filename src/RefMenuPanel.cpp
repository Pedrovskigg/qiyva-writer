#include "RefMenuPanel.h"

#include "DocCache.h"
#include "EditorHost.h"
#include "ProjectModel.h"
#include "ProjectStorage.h"
#include "SceneUtils.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSplitter>
#include <QTextBrowser>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr int kPanelWidth = 320;

// Source ID schema:
// "ms:<id>"   → manuscrito
// "dr:<key>"  → gaveta
}

RefMenuPanel::RefMenuPanel(ProjectModel* model, EditorHost* host, DocCache* cache, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_host(host)
    , m_cache(cache)
    , m_sourceCombo(nullptr)
    , m_docList(nullptr)
    , m_preview(nullptr)
    , m_emptyLabel(nullptr)
    , m_splitter(nullptr)
    , m_closeBtn(nullptr)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName(QStringLiteral("refMenuPanel"));
    setFixedWidth(kPanelWidth);
    buildUi();
    if (m_model) {
        connect(m_model, &ProjectModel::manuscriptsChanged, this, &RefMenuPanel::refresh);
        connect(m_model, &ProjectModel::chaptersChanged, this, &RefMenuPanel::refresh);
        connect(m_model, &ProjectModel::drawersChanged, this, &RefMenuPanel::refresh);
        connect(m_model, &ProjectModel::loaded, this, &RefMenuPanel::refresh);
    }
    hide();
}

void RefMenuPanel::buildUi()
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Header
    auto* header = new QWidget(this);
    header->setObjectName(QStringLiteral("refHeader"));
    header->setAttribute(Qt::WA_StyledBackground, true);
    auto* headerLay = new QHBoxLayout(header);
    headerLay->setContentsMargins(12, 6, 6, 6);
    headerLay->setSpacing(6);
    auto* title = new QLabel(tr("Referência"), header);
    title->setObjectName(QStringLiteral("refTitle"));
    headerLay->addWidget(title);
    headerLay->addStretch();
    m_closeBtn = new QToolButton(header);
    m_closeBtn->setObjectName(QStringLiteral("refCloseBtn"));
    m_closeBtn->setText(QStringLiteral("✕"));
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setToolTip(tr("Fechar"));
    connect(m_closeBtn, &QToolButton::clicked, this, &RefMenuPanel::closePanel);
    headerLay->addWidget(m_closeBtn);
    outer->addWidget(header);

    // Body
    auto* body = new QWidget(this);
    body->setObjectName(QStringLiteral("refBody"));
    body->setAttribute(Qt::WA_StyledBackground, true);
    auto* bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(12, 10, 12, 12);
    bodyLay->setSpacing(8);

    m_sourceCombo = new QComboBox(body);
    m_sourceCombo->setObjectName(QStringLiteral("refSourceCombo"));
    connect(m_sourceCombo, &QComboBox::currentIndexChanged, this, &RefMenuPanel::onSourceChanged);
    bodyLay->addWidget(m_sourceCombo);

    m_splitter = new QSplitter(Qt::Vertical, body);
    m_splitter->setObjectName(QStringLiteral("refSplitter"));
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(4);

    m_docList = new QListWidget(m_splitter);
    m_docList->setObjectName(QStringLiteral("refDocList"));
    m_docList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_docList, &QListWidget::itemClicked, this, &RefMenuPanel::onItemActivated);
    connect(m_docList, &QListWidget::currentItemChanged, this,
        [this](QListWidgetItem* current, QListWidgetItem*) { onItemActivated(current); });
    m_splitter->addWidget(m_docList);

    m_preview = new QTextBrowser(m_splitter);
    m_preview->setObjectName(QStringLiteral("refPreview"));
    m_preview->setReadOnly(true);
    m_preview->setOpenExternalLinks(false);
    m_preview->setPlaceholderText(tr("Selecione um documento acima pra visualizar aqui."));
    m_splitter->addWidget(m_preview);

    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 2);
    bodyLay->addWidget(m_splitter, /*stretch=*/1);

    m_emptyLabel = new QLabel(tr("Nenhum item encontrado."), body);
    m_emptyLabel->setObjectName(QStringLiteral("refEmpty"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setVisible(false);
    bodyLay->addWidget(m_emptyLabel);

    outer->addWidget(body, /*stretch=*/1);

    setStyleSheet(QStringLiteral(R"(
        QWidget#refMenuPanel {
            background: rgba(20, 20, 20, 240);
            border: 1px solid #2a2a2a;
            border-radius: 6px;
        }
        QWidget#refHeader {
            background: #161616;
            border-bottom: 1px solid #232323;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
        }
        QLabel#refTitle {
            color: #e8e3d6; font-size: 13px; font-weight: 600;
        }
        QToolButton#refCloseBtn {
            background: transparent;
            color: #8a857a;
            border: none;
            min-width: 22px; min-height: 22px;
            font-size: 13px;
        }
        QToolButton#refCloseBtn:hover { color: #e8e3d6; }
        QWidget#refBody { background: transparent; }
        QComboBox#refSourceCombo {
            background: #1a1a1a;
            color: #e8e3d6;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            padding: 6px 8px;
            min-height: 26px;
        }
        QComboBox#refSourceCombo::drop-down { border: none; width: 16px; }
        QComboBox#refSourceCombo QAbstractItemView {
            background: #1a1a1a; color: #e8e3d6;
            selection-background-color: #2c3a5e;
            border: 1px solid #2a2a2a;
        }
        QListWidget#refDocList {
            background: #161616;
            color: #c8c3b6;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            outline: none;
            padding: 4px;
        }
        QListWidget#refDocList::item {
            padding: 6px 8px;
            border-radius: 3px;
        }
        QListWidget#refDocList::item:hover { background: #1f1f1f; color: #e8e3d6; }
        QListWidget#refDocList::item:selected { background: #2c3a5e; color: #f0e8d8; }
        QLabel#refEmpty { color: #6a6558; font-size: 11px; padding: 18px 0; }
        QTextBrowser#refPreview {
            background: #161616;
            color: #d8d3c6;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            padding: 10px 12px;
            font-size: 12px;
        }
        QSplitter#refSplitter::handle {
            background: #232323;
            border-radius: 2px;
        }
        QSplitter#refSplitter::handle:hover { background: #2c2c2c; }
    )"));
}

void RefMenuPanel::rebuildSourceCombo()
{
    if (!m_sourceCombo || !m_model) return;
    const QString prev = m_sourceCombo->currentData().toString();
    QSignalBlocker block(m_sourceCombo);
    m_sourceCombo->clear();
    // Manuscritos
    for (const auto& m : m_model->manuscripts()) {
        m_sourceCombo->addItem(tr("📖 %1").arg(m.title),
                               QStringLiteral("ms:%1").arg(m.id));
    }
    // Drawers
    for (const auto& d : m_model->drawers()) {
        m_sourceCombo->addItem(tr("📂 %1").arg(d.title),
                               QStringLiteral("dr:%1").arg(d.key));
    }
    // Restaura seleção anterior se possível
    if (!prev.isEmpty()) {
        const int idx = m_sourceCombo->findData(prev);
        if (idx >= 0) m_sourceCombo->setCurrentIndex(idx);
    }
}

void RefMenuPanel::rebuildDocList()
{
    if (!m_docList || !m_model || !m_sourceCombo) return;
    m_docList->clear();
    const QString src = m_sourceCombo->currentData().toString();
    if (src.isEmpty()) {
        if (m_emptyLabel) m_emptyLabel->setVisible(true);
        return;
    }

    if (src.startsWith(QStringLiteral("ms:"))) {
        const QString msId = src.mid(3);
        QList<Chapter> chs;
        for (const auto& c : m_model->chapters()) if (c.manuscriptId == msId) chs.append(c);
        std::sort(chs.begin(), chs.end(), [](const Chapter& a, const Chapter& b) {
            return a.order < b.order;
        });
        for (const auto& c : chs) {
            auto* it = new QListWidgetItem(c.title.isEmpty() ? tr("(sem título)") : c.title);
            it->setData(Qt::UserRole, QStringLiteral("ch:%1:%2").arg(msId, c.id));
            m_docList->addItem(it);
            // Cenas indentadas (se houver mais de 1)
            if (c.scenes.size() > 1) {
                for (int i = 0; i < c.scenes.size(); ++i) {
                    const auto& sc = c.scenes.at(i);
                    auto* si = new QListWidgetItem(QStringLiteral("    %1").arg(
                        sc.title.isEmpty() ? tr("Cena %1").arg(i + 1) : sc.title));
                    QFont f = si->font(); f.setItalic(true); si->setFont(f);
                    si->setData(Qt::UserRole, QStringLiteral("sc:%1:%2:%3").arg(msId, c.id).arg(i));
                    m_docList->addItem(si);
                }
            }
        }
    } else if (src.startsWith(QStringLiteral("dr:"))) {
        const QString drawerKey = src.mid(3);
        const Drawer* d = m_model->findDrawer(drawerKey);
        if (!d) return;

        // Agrupa por folder (raíz primeiro, depois cada pasta)
        auto addItem = [this](const DrawerItem& it) {
            auto* li = new QListWidgetItem(it.title.isEmpty() ? tr("(sem título)") : it.title);
            li->setData(Qt::UserRole, QStringLiteral("it:%1").arg(it.id));
            m_docList->addItem(li);
        };

        // Raíz
        for (const auto& it : d->items) if (it.folderId.isEmpty()) addItem(it);
        // Cada pasta
        for (const auto& f : d->folders) {
            // header da pasta (não selecionável)
            auto* hdr = new QListWidgetItem(QStringLiteral("📁 %1").arg(f.title));
            hdr->setFlags(hdr->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
            QFont hf = hdr->font(); hf.setBold(true); hdr->setFont(hf);
            m_docList->addItem(hdr);
            for (const auto& it : d->items) if (it.folderId == f.id) addItem(it);
        }
    }

    if (m_emptyLabel) m_emptyLabel->setVisible(m_docList->count() == 0);
}

void RefMenuPanel::refresh()
{
    rebuildSourceCombo();
    rebuildDocList();
}

void RefMenuPanel::onSourceChanged(int)
{
    rebuildDocList();
}

QString RefMenuPanel::resolveDocHtml(const QString& key) const
{
    if (!m_model) return QString();

    if (key.startsWith(QStringLiteral("ch:"))) {
        const QStringList parts = key.split(QLatin1Char(':'));
        if (parts.size() < 3) return QString();
        const QString msId = parts.at(1);
        const QString chId = parts.at(2);
        const QString cacheKey = DocCache::chapterKey(msId, chId);
        if (m_cache && m_cache->has(cacheKey)) return m_cache->get(cacheKey);
        const Chapter* ch = m_model->findChapter(chId);
        if (!ch || ch->file.isEmpty() || m_projectRoot.isEmpty()) return QString();
        bool ok = false;
        return ProjectStorage::readChapter(m_projectRoot, ch->file, &ok);
    }
    if (key.startsWith(QStringLiteral("sc:"))) {
        const QStringList parts = key.split(QLatin1Char(':'));
        if (parts.size() < 4) return QString();
        const QString msId = parts.at(1);
        const QString chId = parts.at(2);
        const int sceneIdx = parts.at(3).toInt();
        const QString cacheKey = DocCache::chapterKey(msId, chId);
        QString chapterHtml;
        if (m_cache && m_cache->has(cacheKey)) {
            chapterHtml = m_cache->get(cacheKey);
        } else if (const Chapter* ch = m_model->findChapter(chId); ch && !m_projectRoot.isEmpty()) {
            bool ok = false;
            chapterHtml = ProjectStorage::readChapter(m_projectRoot, ch->file, &ok);
        }
        return SceneUtils::getSceneHtml(chapterHtml, sceneIdx);
    }
    if (key.startsWith(QStringLiteral("it:"))) {
        const QString itemId = key.mid(3);
        const QString cacheKey = DocCache::itemKey(itemId);
        if (m_cache && m_cache->has(cacheKey)) return m_cache->get(cacheKey);
        const DrawerItem* item = m_model->findDrawerItem(itemId);
        if (!item) return QString();
        if (item->hasInlineHtml) return item->html;
        if (!item->file.isEmpty() && !m_projectRoot.isEmpty()) {
            bool ok = false;
            return ProjectStorage::readText(ProjectStorage::joinPath(m_projectRoot, item->file), &ok);
        }
    }
    return QString();
}

void RefMenuPanel::onItemActivated(QListWidgetItem* item)
{
    if (!item || !m_preview) return;
    const QString key = item->data(Qt::UserRole).toString();
    if (key.isEmpty()) {
        m_preview->clear();
        return;
    }
    const QString html = resolveDocHtml(key);
    m_preview->setHtml(html.isEmpty() ? QStringLiteral("<p style='color:#6a6558;'>(documento vazio)</p>") : html);
}

void RefMenuPanel::openPanel()
{
    refresh();
    show();
    raise();
    emit geometryChanged();
}

void RefMenuPanel::closePanel()
{
    hide();
    emit geometryChanged();
}

void RefMenuPanel::togglePanel()
{
    if (isVisible()) closePanel();
    else openPanel();
}
