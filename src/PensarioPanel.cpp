#include "PensarioPanel.h"

#include "DocCache.h"
#include "MarkerStore.h"
#include "NoteEditPopup.h"
#include "NotesStore.h"
#include "ProjectModel.h"
#include "Theme.h"

#include <QAction>
#include <QActionGroup>
#include <QColor>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QScrollArea>
#include <QShowEvent>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>

namespace {
constexpr int kPanelWidth = 380;
constexpr int kMargin = 12;
}

PensarioPanel::PensarioPanel(MarkerStore* markers, ProjectModel* model,
                             NotesStore* notes, QWidget* parent)
    : QWidget(parent)
    , m_markers(markers)
    , m_model(model)
    , m_notesStore(notes)
{
    setObjectName(QStringLiteral("pensarioPanel"));
    buildUi();
    applyTheme();
    connect(Theme::Manager::instance(), &Theme::Manager::themeChanged,
            this, &PensarioPanel::applyTheme);
    if (m_notesStore)
        connect(m_notesStore, &NotesStore::notesChanged, this, &PensarioPanel::rebuildNotes);
    hide();
}

void PensarioPanel::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---------------- Header (arrastável) ----------------
    m_header = new QWidget(this);
    m_header->setObjectName(QStringLiteral("pnHeader"));
    m_header->setFixedHeight(40);
    m_header->installEventFilter(this);
    auto* headLay = new QHBoxLayout(m_header);
    headLay->setContentsMargins(14, 0, 8, 0);
    headLay->setSpacing(6);

    m_title = new QLabel(tr("Pensário"), m_header);
    m_title->setObjectName(QStringLiteral("pnTitle"));
    m_title->installEventFilter(this);
    headLay->addWidget(m_title);
    headLay->addStretch();

    // Seletor de ordenação (só relevante na aba Comentários).
    m_sortBtn = new QToolButton(m_header);
    m_sortBtn->setObjectName(QStringLiteral("pnSort"));
    m_sortBtn->setCursor(Qt::PointingHandCursor);
    m_sortBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_sortBtn->setPopupMode(QToolButton::InstantPopup);
    auto* sortMenu = new QMenu(m_sortBtn);
    auto* actChap = sortMenu->addAction(tr("Por capítulo"));
    auto* actCre = sortMenu->addAction(tr("Por criação"));
    actChap->setCheckable(true);
    actCre->setCheckable(true);
    actChap->setChecked(true);
    auto* sortGrp = new QActionGroup(sortMenu);
    sortGrp->addAction(actChap);
    sortGrp->addAction(actCre);
    m_sortBtn->setMenu(sortMenu);
    auto updateSortText = [this]() {
        m_sortBtn->setText((m_sortMode == SortMode::Chapters
                                ? tr("Capítulo") : tr("Criação"))
                           + QStringLiteral(" ▾"));
    };
    connect(actChap, &QAction::triggered, this, [this, updateSortText]() {
        m_sortMode = SortMode::Chapters; updateSortText(); rebuildComments();
    });
    connect(actCre, &QAction::triggered, this, [this, updateSortText]() {
        m_sortMode = SortMode::Creation; updateSortText(); rebuildComments();
    });
    updateSortText();
    headLay->addWidget(m_sortBtn);

    m_closeBtn = new QToolButton(m_header);
    m_closeBtn->setObjectName(QStringLiteral("pnClose"));
    m_closeBtn->setText(QStringLiteral("×")); // ×
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setToolTip(tr("Fechar"));
    m_closeBtn->setFixedSize(28, 28);
    connect(m_closeBtn, &QToolButton::clicked, this, &PensarioPanel::closePanel);
    headLay->addWidget(m_closeBtn);

    root->addWidget(m_header);

    // ---------------- Abas ----------------
    auto* tabsRow = new QWidget(this);
    tabsRow->setObjectName(QStringLiteral("pnTabsRow"));
    auto* tabsLay = new QHBoxLayout(tabsRow);
    tabsLay->setContentsMargins(8, 6, 8, 6);
    tabsLay->setSpacing(4);

    auto makeTab = [&](const QString& label) {
        auto* b = new QToolButton(tabsRow);
        b->setObjectName(QStringLiteral("pnTab"));
        b->setText(label);
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        tabsLay->addWidget(b);
        return b;
    };
    m_tabComments = makeTab(tr("Comentários"));
    m_tabNotes    = makeTab(tr("Notas"));
    m_tabNames    = makeTab(tr("Nomes"));
    m_tabMap      = makeTab(tr("Mapa"));

    connect(m_tabComments, &QToolButton::clicked, this, [this]() { selectTab(Tab::Comments); });
    connect(m_tabNotes,    &QToolButton::clicked, this, [this]() { selectTab(Tab::Notes); });
    connect(m_tabNames,    &QToolButton::clicked, this, [this]() { selectTab(Tab::Names); });
    connect(m_tabMap,      &QToolButton::clicked, this, [this]() { selectTab(Tab::Map); });

    root->addWidget(tabsRow);

    // ---------------- Corpo (stack) ----------------
    m_stack = new QStackedWidget(this);

    // Página 0: Comentários
    m_commentsScroll = new QScrollArea(m_stack);
    m_commentsScroll->setWidgetResizable(true);
    m_commentsScroll->setFrameShape(QFrame::NoFrame);
    m_commentsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_commentsInner = new QWidget(m_commentsScroll);
    m_commentsLay = new QVBoxLayout(m_commentsInner);
    m_commentsLay->setContentsMargins(12, 10, 12, 12);
    m_commentsLay->setSpacing(8);
    m_commentsLay->addStretch();
    m_commentsScroll->setWidget(m_commentsInner);
    m_stack->addWidget(m_commentsScroll);

    // Página 1: Notas (funcional)
    m_stack->addWidget(buildNotesPage());

    // Páginas 2-3: placeholders das fatias futuras
    m_stack->addWidget(buildPlaceholderPage(
        tr("Gerador de nomes"),
        tr("Nomes de personagens, lugares e mais, por estilo. Em breve.")));
    m_stack->addWidget(buildPlaceholderPage(
        tr("Mapa-múndi"),
        tr("Países, capitais e fronteiras do mundo real. Em breve.")));

    root->addWidget(m_stack, 1);

    selectTab(Tab::Comments);
}

QWidget* PensarioPanel::buildPlaceholderPage(const QString& title, const QString& subtitle)
{
    auto* page = new QWidget(m_stack);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(24, 24, 24, 24);
    lay->addStretch();

    auto* t = new QLabel(title, page);
    t->setObjectName(QStringLiteral("pnPlaceholderTitle"));
    t->setAlignment(Qt::AlignCenter);
    lay->addWidget(t);

    auto* s = new QLabel(subtitle, page);
    s->setObjectName(QStringLiteral("pnPlaceholderSub"));
    s->setAlignment(Qt::AlignCenter);
    s->setWordWrap(true);
    lay->addWidget(s);

    lay->addStretch();
    return page;
}

QWidget* PensarioPanel::buildNotesPage()
{
    auto* page = new QWidget(m_stack);
    auto* lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 10, 12, 12);
    lay->setSpacing(8);

    auto* addBtn = new QToolButton(page);
    addBtn->setObjectName(QStringLiteral("pnAddNote"));
    addBtn->setText(tr("+ Nova nota"));
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(addBtn, &QToolButton::clicked, this, &PensarioPanel::openNoteCreate);
    lay->addWidget(addBtn);

    m_notesScroll = new QScrollArea(page);
    m_notesScroll->setWidgetResizable(true);
    m_notesScroll->setFrameShape(QFrame::NoFrame);
    m_notesScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_notesInner = new QWidget(m_notesScroll);
    m_notesLay = new QVBoxLayout(m_notesInner);
    m_notesLay->setContentsMargins(0, 0, 0, 0);
    m_notesLay->setSpacing(8);
    m_notesLay->addStretch();
    m_notesScroll->setWidget(m_notesInner);
    lay->addWidget(m_notesScroll, 1);

    return page;
}

void PensarioPanel::rebuildNotes()
{
    if (!m_notesLay) return;

    while (m_notesLay->count() > 0) {
        QLayoutItem* item = m_notesLay->takeAt(0);
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }

    int total = 0;
    if (m_notesStore) {
        QVector<NotesStore::Note> list = m_notesStore->notes();
        std::sort(list.begin(), list.end(),
                  [](const NotesStore::Note& a, const NotesStore::Note& b) {
                      return a.createdAt > b.createdAt; // mais recentes primeiro
                  });
        for (const NotesStore::Note& n : list) {
            m_notesLay->addWidget(buildNoteCard(n.id, n.color, n.title, n.text));
            ++total;
        }
    }

    if (total == 0) {
        auto* empty = new QLabel(
            tr("Nenhuma nota ainda.\nClique em “+ Nova nota” para criar a primeira."),
            m_notesInner);
        empty->setObjectName(QStringLiteral("pnEmpty"));
        empty->setAlignment(Qt::AlignCenter);
        empty->setWordWrap(true);
        m_notesLay->addWidget(empty);
    }

    m_notesLay->addStretch();
}

QWidget* PensarioPanel::buildNoteCard(const QString& id, const QString& color,
                                      const QString& title, const QString& text)
{
    auto* card = new QFrame(m_notesInner);
    card->setObjectName(QStringLiteral("pnNoteCard"));
    card->setCursor(Qt::PointingHandCursor);
    card->setProperty("pnNoteId", id);
    card->installEventFilter(this);

    // Mesma caixa neutra dos comentários; a cor da nota vira uma faixa no topo.
    auto* outer = new QVBoxLayout(card);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    QColor c(color);
    if (!c.isValid()) c = QColor(QStringLiteral("#FFD54F"));
    auto* stripe = new QWidget(card);
    stripe->setFixedHeight(4);
    stripe->setStyleSheet(QStringLiteral(
        "background:%1; border-top-left-radius:8px; border-top-right-radius:8px;")
        .arg(c.name()));
    outer->addWidget(stripe);

    auto* body = new QWidget(card);
    auto* lay = new QVBoxLayout(body);
    lay->setContentsMargins(12, 9, 10, 10);
    lay->setSpacing(4);

    // Cabeçalho: título (se houver) + botão excluir.
    auto* head = new QHBoxLayout();
    head->setContentsMargins(0, 0, 0, 0);
    head->setSpacing(4);
    if (!title.trimmed().isEmpty()) {
        auto* t = new QLabel(title.trimmed(), body);
        t->setObjectName(QStringLiteral("pnNoteCardTitle"));
        t->setWordWrap(true);
        head->addWidget(t, 1);
    } else {
        head->addStretch();
    }
    auto* del = new QToolButton(body);
    del->setObjectName(QStringLiteral("pnNoteDelete"));
    del->setText(QStringLiteral("×"));
    del->setCursor(Qt::PointingHandCursor);
    del->setToolTip(tr("Excluir nota"));
    del->setFixedSize(20, 20);
    connect(del, &QToolButton::clicked, this, [this, id]() {
        if (m_notesStore) m_notesStore->removeNote(id);
    });
    head->addWidget(del, 0, Qt::AlignTop);
    lay->addLayout(head);

    if (!text.trimmed().isEmpty()) {
        auto* txt = new QLabel(text, body);
        txt->setObjectName(QStringLiteral("pnNoteText"));
        txt->setWordWrap(true);
        txt->setTextInteractionFlags(Qt::NoTextInteraction);
        txt->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        lay->addWidget(txt);

        if (text.trimmed().size() > 80) {
            txt->setMaximumHeight(64);
            card->setProperty("pnResizable", true);
            card->setProperty("pnResizeTarget", QStringLiteral("pnNoteText"));
        }
    }

    outer->addWidget(body);
    return card;
}

void PensarioPanel::ensureNotePopup()
{
    if (m_notePopup) return;
    QWidget* host = parentWidget() ? parentWidget() : this;
    m_notePopup = new NoteEditPopup(host);
    connect(m_notePopup, &NoteEditPopup::confirmed, this,
            [this](QColor color, QString title, QString text) {
        if (!m_notesStore) return;
        if (m_editingNoteId.isEmpty())
            m_notesStore->addNote(color.name(), title, text);
        else
            m_notesStore->updateNote(m_editingNoteId, color.name(), title, text);
        m_editingNoteId.clear();
    });
    connect(m_notePopup, &NoteEditPopup::cancelled, this,
            [this]() { m_editingNoteId.clear(); });
}

void PensarioPanel::openNoteCreate()
{
    ensureNotePopup();
    m_editingNoteId.clear();
    m_notePopup->openForCreate();
}

void PensarioPanel::openNoteEditById(const QString& id)
{
    if (!m_notesStore) return;
    for (const NotesStore::Note& n : m_notesStore->notes()) {
        if (n.id == id) {
            ensureNotePopup();
            m_editingNoteId = id;
            m_notePopup->openForEdit(QColor(n.color), n.title, n.text);
            return;
        }
    }
}

void PensarioPanel::selectTab(Tab tab)
{
    m_tab = tab;
    m_tabComments->setChecked(tab == Tab::Comments);
    m_tabNotes->setChecked(tab == Tab::Notes);
    m_tabNames->setChecked(tab == Tab::Names);
    m_tabMap->setChecked(tab == Tab::Map);
    m_stack->setCurrentIndex(static_cast<int>(tab));
    if (m_sortBtn) m_sortBtn->setVisible(tab == Tab::Comments);
    if (tab == Tab::Comments) rebuildComments();
    else if (tab == Tab::Notes) rebuildNotes();
}

void PensarioPanel::rebuildComments()
{
    if (!m_commentsLay) return;

    // Limpa cards anteriores (menos o stretch final).
    while (m_commentsLay->count() > 0) {
        QLayoutItem* item = m_commentsLay->takeAt(0);
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }

    int total = 0;
    if (m_markers) {
        // Achata todos os comentários em (docKey, entry).
        struct Item { QString docKey; MarkerStore::Entry e; };
        QVector<Item> items;
        const auto& all = m_markers->allEntries();
        for (auto it = all.constBegin(); it != all.constEnd(); ++it)
            for (const MarkerStore::Entry& e : it.value())
                items.append({it.key(), e});
        total = items.size();

        if (m_sortMode == SortMode::Creation) {
            // Lista cronológica plana (mais recentes primeiro); origem no card.
            std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
                if (a.e.createdAt != b.e.createdAt) return a.e.createdAt > b.e.createdAt;
                return a.e.start < b.e.start;
            });
            for (const Item& item : items) {
                m_commentsLay->addWidget(buildCommentCard(
                    item.docKey, item.e.color, item.e.comment, item.e.text,
                    item.e.start, item.e.end,
                    originLabel(item.docKey, item.e.sceneIndex)));
            }
        } else {
            // Por capítulo: grupos na ordem da obra, subdivididos por cena.
            std::sort(items.begin(), items.end(), [this](const Item& a, const Item& b) {
                const int ra = rankForKey(a.docKey), rb = rankForKey(b.docKey);
                if (ra != rb) return ra < rb;
                if (a.docKey != b.docKey) return a.docKey < b.docKey;
                if (a.e.sceneIndex != b.e.sceneIndex) return a.e.sceneIndex < b.e.sceneIndex;
                if (a.e.blockIndex != b.e.blockIndex) return a.e.blockIndex < b.e.blockIndex;
                return a.e.start < b.e.start;
            });
            QString lastHeader;
            bool first = true;
            for (const Item& item : items) {
                const QString header = originLabel(item.docKey, item.e.sceneIndex);
                if (first || header != lastHeader) {
                    first = false;
                    lastHeader = header;
                    auto* group = new QLabel(header, m_commentsInner);
                    group->setObjectName(QStringLiteral("pnGroup"));
                    m_commentsLay->addWidget(group);
                }
                m_commentsLay->addWidget(buildCommentCard(
                    item.docKey, item.e.color, item.e.comment, item.e.text,
                    item.e.start, item.e.end));
            }
        }
    }

    if (total == 0) {
        auto* empty = new QLabel(
            tr("Nenhum comentário ainda.\nSelecione um trecho e use o marcador "
               "com comentário para que ele apareça aqui."),
            m_commentsInner);
        empty->setObjectName(QStringLiteral("pnEmpty"));
        empty->setAlignment(Qt::AlignCenter);
        empty->setWordWrap(true);
        m_commentsLay->addWidget(empty);
    }

    m_commentsLay->addStretch();
}

QWidget* PensarioPanel::buildCommentCard(const QString& docKey, const QString& color,
                                         const QString& comment, const QString& text,
                                         int start, int end, const QString& origin)
{
    auto* card = new QFrame(m_commentsInner);
    card->setObjectName(QStringLiteral("pnCard"));
    card->setCursor(Qt::PointingHandCursor);
    card->setProperty("pnDocKey", docKey);
    card->setProperty("pnStart", start);
    card->setProperty("pnEnd", end);
    card->setProperty("pnText", text);
    card->installEventFilter(this);

    auto* row = new QHBoxLayout(card);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    // Faixa de cor do marcador na borda esquerda.
    auto* stripe = new QWidget(card);
    stripe->setFixedWidth(4);
    QColor c(color);
    if (!c.isValid()) c = QColor(QStringLiteral("#FFD54F"));
    stripe->setStyleSheet(QStringLiteral("background:%1;border-top-left-radius:8px;"
                                         "border-bottom-left-radius:8px;").arg(c.name()));
    row->addWidget(stripe);

    auto* body = new QWidget(card);
    auto* lay = new QVBoxLayout(body);
    lay->setContentsMargins(12, 9, 12, 9);
    lay->setSpacing(4);

    if (!origin.isEmpty()) {
        auto* org = new QLabel(origin, body);
        org->setObjectName(QStringLiteral("pnCardOrigin"));
        org->setWordWrap(true);
        lay->addWidget(org);
    }

    auto* cmt = new QLabel(comment, body);
    cmt->setObjectName(QStringLiteral("pnCardComment"));
    cmt->setWordWrap(true);
    cmt->setTextInteractionFlags(Qt::NoTextInteraction);
    lay->addWidget(cmt);

    const QString trimmed = text.trimmed();
    if (!trimmed.isEmpty()) {
        auto* quote = new QLabel(QStringLiteral("“%1”").arg(trimmed), body); // “ ”
        quote->setObjectName(QStringLiteral("pnCardQuote"));
        quote->setWordWrap(true);
        quote->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        lay->addWidget(quote);

        // Trecho longo: começa recortado (~3 linhas) e fica redimensionável.
        // A "alça" é a borda inferior do card (invisível) — ver eventFilter.
        if (trimmed.size() > 80) {
            quote->setMaximumHeight(54);
            card->setProperty("pnResizable", true);
        }
    }

    row->addWidget(body, 1);
    return card;
}

const Chapter* PensarioPanel::chapterForKey(const QString& docKey) const
{
    if (!m_model) return nullptr;
    for (const Chapter& ch : m_model->chapters()) {
        if (DocCache::chapterKey(ch.manuscriptId, ch.id) == docKey)
            return &ch;
    }
    return nullptr;
}

QString PensarioPanel::originLabel(const QString& docKey, int sceneIndex) const
{
    const Chapter* chap = chapterForKey(docKey);
    QString base = docTitleForKey(docKey);
    // Capítulo tem >1 cena, ou o próprio marker está numa cena adiante: mostra cena.
    bool multi = (chap && chap->scenes.size() > 1) || sceneIndex > 0;
    if (multi && sceneIndex >= 0) {
        const QString sceneName =
            (chap && sceneIndex < chap->scenes.size() && !chap->scenes[sceneIndex].title.isEmpty())
                ? chap->scenes[sceneIndex].title
                : tr("Cena %1").arg(sceneIndex + 1);
        base += QStringLiteral(" • %1").arg(sceneName);
    }
    return base;
}

int PensarioPanel::rankForKey(const QString& docKey) const
{
    if (m_model) {
        int i = 0;
        for (const Chapter& ch : m_model->chapters()) {
            if (DocCache::chapterKey(ch.manuscriptId, ch.id) == docKey) return i;
            ++i;
        }
    }
    return std::numeric_limits<int>::max(); // não-capítulos vão pro fim
}

QString PensarioPanel::docTitleForKey(const QString& docKey) const
{
    if (!m_model) return docKey;

    // Capítulo? (compara chaves computadas em vez de parsear strings)
    for (const Chapter& ch : m_model->chapters()) {
        if (DocCache::chapterKey(ch.manuscriptId, ch.id) == docKey)
            return ch.title.isEmpty() ? tr("Capítulo sem título") : ch.title;
    }
    // Item de gaveta?
    for (const Drawer& d : m_model->drawers()) {
        for (const DrawerItem& it : d.items) {
            if (DocCache::itemKey(it.id) == docKey) {
                const QString name = it.title.isEmpty() ? tr("Item sem título") : it.title;
                return d.title.isEmpty() ? name
                                         : QStringLiteral("%1 · %2").arg(d.title, name);
            }
        }
    }
    // Manuscrito?
    for (const Manuscript& ms : m_model->manuscripts()) {
        if (QStringLiteral("ms:%1").arg(ms.id) == docKey)
            return ms.title.isEmpty() ? tr("Manuscrito") : ms.title;
    }
    return docKey;
}

void PensarioPanel::refresh()
{
    if (m_tab == Tab::Comments) rebuildComments();
    else if (m_tab == Tab::Notes) rebuildNotes();
}

void PensarioPanel::togglePanel()
{
    if (isPanelOpen()) closePanel();
    else openPanel();
}

void PensarioPanel::openPanel()
{
    refresh();
    ancorRight();
    show();
    raise();
}

void PensarioPanel::closePanel()
{
    hide();
}

bool PensarioPanel::isPanelOpen() const
{
    return isVisible();
}

void PensarioPanel::ancorRight()
{
    QWidget* p = parentWidget();
    if (!p) return;
    const int h = qMax(360, p->height() - kMargin * 2);
    resize(kPanelWidth, qMin(h, p->height() - kMargin * 2));
    move(p->width() - width() - kMargin, kMargin);
    m_positioned = true;
}

void PensarioPanel::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!m_positioned) ancorRight();
}

bool PensarioPanel::eventFilter(QObject* watched, QEvent* event)
{
    // Card (comentário ou nota): clicar age (navega / edita); arrastar a borda
    // inferior (alça invisível, últimos ~12px) redimensiona o conteúdo recortado.
    if (auto* card = qobject_cast<QWidget*>(watched);
        card && (card->property("pnDocKey").isValid()
                 || card->property("pnNoteId").isValid())) {
        const QEvent::Type t = event->type();
        if (t == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton
                && card->property("pnResizable").toBool()
                && me->position().toPoint().y() >= card->height() - 12) {
                m_resizingGrip = card;
                m_gripQuote = card->findChild<QLabel*>(
                    card->property("pnResizeTarget").toString());
                m_gripStartY = me->globalPosition().toPoint().y();
                m_gripStartH = m_gripQuote ? m_gripQuote->maximumHeight() : 0;
                card->grabMouse();
                return true;
            }
            return false;
        }
        if (t == QEvent::MouseMove && m_resizingGrip == card && m_gripQuote) {
            auto* me = static_cast<QMouseEvent*>(event);
            const int delta = me->globalPosition().toPoint().y() - m_gripStartY;
            m_gripQuote->setMaximumHeight(qBound(20, m_gripStartH + delta, 5000));
            return true;
        }
        if (t == QEvent::MouseButtonRelease) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (m_resizingGrip == card) {
                card->releaseMouse();
                m_resizingGrip = nullptr;
                m_gripQuote = nullptr;
                return true;
            }
            if (me->button() == Qt::LeftButton
                && card->rect().contains(me->position().toPoint())) {
                if (card->property("pnDocKey").isValid())
                    emit openMarkerRequested(card->property("pnDocKey").toString(),
                                             card->property("pnStart").toInt(),
                                             card->property("pnEnd").toInt(),
                                             card->property("pnText").toString());
                else
                    openNoteEditById(card->property("pnNoteId").toString());
                return true;
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    // Arrastar o painel pelo header.
    if (watched == m_header || watched == m_title) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_dragging = true;
                m_dragOffset = me->globalPosition().toPoint() - frameGeometry().topLeft();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove && m_dragging) {
            auto* me = static_cast<QMouseEvent*>(event);
            QPoint target = me->globalPosition().toPoint() - m_dragOffset;
            if (QWidget* p = parentWidget()) {
                QPoint local = p->mapFromGlobal(target);
                local.setX(qBound(0, local.x(), qMax(0, p->width() - width())));
                local.setY(qBound(0, local.y(), qMax(0, p->height() - height())));
                move(local);
            }
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            m_dragging = false;
        }
        return QWidget::eventFilter(watched, event);
    }

    return QWidget::eventFilter(watched, event);
}

void PensarioPanel::applyTheme()
{
    const QString bg       = Theme::panelBackground();
    const QString border   = Theme::borderStrong();
    const QString textPri  = Theme::textPrimary();
    const QString textMut  = Theme::textMuted();
    const QString textBrt  = Theme::textBright();
    const QString cardBg   = Theme::inputBackground();
    const QString cardBd   = Theme::subtleBorder();
    const QString hover    = Theme::hoverStrong();
    const QString accent   = Theme::accentDefault();

    setStyleSheet(QStringLiteral(R"(
        #pensarioPanel {
            background: %1;
            border: 1px solid %2;
            border-radius: 10px;
        }
        #pnHeader {
            border-bottom: 1px solid %2;
        }
        #pnTitle {
            color: %3;
            font-size: 15px;
            font-weight: 600;
        }
        #pnClose {
            color: %4;
            border: none;
            background: transparent;
            font-size: 18px;
            border-radius: 6px;
        }
        #pnClose:hover { background: %7; color: %5; }
        #pnSort {
            color: %4;
            background: transparent;
            border: 1px solid %9;
            border-radius: 6px;
            padding: 3px 8px;
            font-size: 11px;
        }
        #pnSort:hover { background: %7; color: %3; }
        #pnSort::menu-indicator { image: none; width: 0; }
        #pnCardOrigin {
            color: %8;
            font-size: 11px;
            font-weight: 600;
        }
        #pnTab {
            color: %4;
            background: transparent;
            border: none;
            border-radius: 7px;
            padding: 6px 4px;
            font-size: 12px;
        }
        #pnTab:hover { background: %7; color: %3; }
        #pnTab:checked { background: %8; color: %5; font-weight: 600; }
        #pnGroup {
            color: %4;
            font-size: 11px;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            padding: 6px 2px 2px 2px;
        }
        #pnCard {
            background: %6;
            border: 1px solid %9;
            border-radius: 8px;
        }
        #pnCard:hover { border-color: %8; }
        #pnCardComment {
            color: %3;
            font-size: 13px;
        }
        #pnCardQuote {
            color: %4;
            font-size: 12px;
            font-style: italic;
        }
        #pnAddNote {
            color: %3;
            background: %6;
            border: 1px dashed %9;
            border-radius: 8px;
            padding: 9px;
            font-size: 13px;
        }
        #pnAddNote:hover { background: %7; border-color: %8; }
        #pnNoteCard {
            background: %6;
            border: 1px solid %9;
            border-radius: 8px;
        }
        #pnNoteCard:hover { border-color: %8; }
        #pnNoteCardTitle { color: %3; font-size: 13px; font-weight: 600; }
        #pnNoteText { color: %3; font-size: 13px; }
        #pnNoteDelete {
            color: %4;
            background: transparent;
            border: none;
            font-size: 16px;
            border-radius: 4px;
        }
        #pnNoteDelete:hover { color: %5; background: %7; }
        #pnEmpty, #pnPlaceholderSub {
            color: %4;
            font-size: 13px;
        }
        #pnPlaceholderTitle {
            color: %3;
            font-size: 16px;
            font-weight: 600;
        }
    )")
        .arg(bg, border, textPri, textMut, textBrt, cardBg, hover, accent, cardBd));
}
