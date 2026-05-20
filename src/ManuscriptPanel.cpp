#include "ManuscriptPanel.h"
#include "ProjectModel.h"
#include "Theme.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr int kPanelWidth = 280;
}

ManuscriptPanel::ManuscriptPanel(ProjectModel* model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_combo(nullptr)
    , m_listLayout(nullptr)
    , m_scroll(nullptr)
{
    setObjectName(QStringLiteral("manuscriptPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(kPanelWidth);
    setStyleSheet(Theme::panelQss(QStringLiteral("manuscriptPanel")));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Header --------------------------------------------------------------
    auto* header = new QWidget(this);
    header->setObjectName(QStringLiteral("manuscriptHeader"));
    header->setStyleSheet(QStringLiteral(
        "#manuscriptHeader { border-bottom: 1px solid %1; }").arg(Theme::panelBorder()));
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 10, 8, 10);
    headerLayout->setSpacing(4);

    m_combo = new QComboBox(header);
    m_combo->setStyleSheet(QStringLiteral(R"(
        QComboBox {
            background: transparent;
            color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            padding: 4px 10px;
            font-family: 'Lora','Crimson Text',serif;
            font-size: 13px;
        }
        QComboBox:hover { border-color: %3; }
        QComboBox::drop-down { border: none; width: 18px; }
        QComboBox QAbstractItemView {
            background: %4;
            color: %1;
            border: 1px solid %2;
            selection-background-color: %5;
        }
    )").arg(Theme::textBright(), Theme::panelBorder(), Theme::subtleBorder(),
           Theme::panelBackground(), Theme::hoverOverlay()));
    headerLayout->addWidget(m_combo, /*stretch=*/1);
    connect(m_combo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &ManuscriptPanel::onComboChanged);

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

    auto* btnAddChapter = makeIconBtn(QStringLiteral("+"), tr("Novo capítulo"));
    connect(btnAddChapter, &QToolButton::clicked, this, [this]() {
        emit newChapterRequested(activeManuscriptId());
    });
    headerLayout->addWidget(btnAddChapter);

    auto* btnAddMs = makeIconBtn(QStringLiteral("✦"), tr("Novo manuscrito"));
    connect(btnAddMs, &QToolButton::clicked, this, &ManuscriptPanel::newManuscriptRequested);
    headerLayout->addWidget(btnAddMs);

    auto* btnClose = makeIconBtn(QStringLiteral("×"), tr("Fechar"));
    connect(btnClose, &QToolButton::clicked, this, &ManuscriptPanel::closePanel);
    headerLayout->addWidget(btnClose);

    root->addWidget(header);

    // Lista de capítulos --------------------------------------------------
    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("manuscriptScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setStyleSheet(QStringLiteral("#manuscriptScroll { background: transparent; }"));

    auto* listHost = new QWidget(m_scroll);
    listHost->setStyleSheet(QStringLiteral("background: transparent;"));
    m_listLayout = new QVBoxLayout(listHost);
    m_listLayout->setContentsMargins(8, 8, 8, 8);
    m_listLayout->setSpacing(4);
    m_listLayout->addStretch();
    m_scroll->setWidget(listHost);
    root->addWidget(m_scroll, 1);

    if (m_model) {
        connect(m_model, &ProjectModel::manuscriptsChanged, this, &ManuscriptPanel::onManuscriptsChanged);
        connect(m_model, &ProjectModel::chaptersChanged, this, &ManuscriptPanel::onChaptersChanged);
        connect(m_model, &ProjectModel::loaded, this, [this]() { syncCombo(); rebuildList(); });
    }

    hide();
}

void ManuscriptPanel::open() {
    syncCombo();
    rebuildList();
    show();
}

void ManuscriptPanel::closePanel() {
    hide();
    emit panelClosed();
}

QString ManuscriptPanel::activeManuscriptId() const {
    if (!m_combo || m_combo->currentIndex() < 0) return QString();
    return m_combo->currentData().toString();
}

void ManuscriptPanel::syncCombo() {
    if (!m_combo || !m_model) return;
    const QString preserved = m_model->activeManuscriptId().isEmpty()
        ? activeManuscriptId()
        : m_model->activeManuscriptId();

    QSignalBlocker blocker(m_combo);
    m_combo->clear();
    for (const auto& m : m_model->manuscripts()) {
        const QString label = m.title.isEmpty() ? tr("(sem título)") : m.title;
        m_combo->addItem(label, m.id);
    }
    if (m_combo->count() == 0) {
        m_combo->addItem(tr("Nenhum manuscrito"), QString());
        m_combo->setEnabled(false);
    } else {
        m_combo->setEnabled(true);
        int idx = m_combo->findData(preserved);
        m_combo->setCurrentIndex(idx >= 0 ? idx : 0);
    }
}

void ManuscriptPanel::onManuscriptsChanged() {
    syncCombo();
    rebuildList();
}

void ManuscriptPanel::onChaptersChanged() {
    rebuildList();
}

void ManuscriptPanel::onComboChanged(int /*index*/) {
    if (m_model) {
        const QString id = activeManuscriptId();
        if (m_model->activeManuscriptId() != id) {
            m_model->setActiveManuscriptId(id);
        }
    }
    rebuildList();
}

void ManuscriptPanel::rebuildList() {
    if (!m_listLayout) return;
    while (m_listLayout->count() > 1) {
        QLayoutItem* item = m_listLayout->takeAt(0);
        if (auto* w = item->widget()) w->deleteLater();
        delete item;
    }
    if (!m_model) return;

    const QString msId = activeManuscriptId();

    QList<Chapter> filtered;
    for (const auto& c : m_model->chapters()) {
        if (c.manuscriptId == msId) filtered.append(c);
    }
    std::sort(filtered.begin(), filtered.end(), [](const Chapter& a, const Chapter& b) {
        return a.order < b.order;
    });

    if (filtered.isEmpty()) {
        auto* lbl = new QLabel(msId.isEmpty()
            ? tr("Crie um manuscrito pra começar.")
            : tr("Nenhum capítulo. Use + acima."), this);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setWordWrap(true);
        lbl->setStyleSheet(QStringLiteral(
            "color: %1; font-style: italic; padding: 20px 12px;").arg(Theme::textMuted()));
        m_listLayout->insertWidget(0, lbl);
        return;
    }

    const QString chapterQss = QStringLiteral(R"(
        QToolButton {
            background: transparent;
            color: %1;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 6px 10px;
            text-align: left;
            font-family: 'Lora','Crimson Text',serif;
            font-size: 14px;
        }
        QToolButton:hover {
            background: %2;
            color: %3;
            border-color: %4;
        }
    )").arg(Theme::textPrimary(), Theme::hoverOverlay(), Theme::textBright(), Theme::subtleBorder());

    const QString sceneQss = QStringLiteral(R"(
        QToolButton {
            background: transparent;
            color: %1;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 4px 10px 4px 26px;
            text-align: left;
            font-family: 'Lora','Crimson Text',serif;
            font-size: 13px;
            font-style: italic;
        }
        QToolButton:hover {
            background: %2;
            color: %3;
        }
    )").arg(Theme::textMuted(), Theme::hoverOverlay(), Theme::textBright());

    int row = 0;
    for (const auto& c : filtered) {
        auto* btn = new QToolButton(this);
        btn->setText(QStringLiteral("%1.  %2").arg(c.order).arg(c.title.isEmpty() ? tr("(sem título)") : c.title));
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(34);
        btn->setStyleSheet(chapterQss);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        const QString chapterId = c.id;
        const QString manuscriptId = c.manuscriptId;
        connect(btn, &QToolButton::clicked, this, [this, manuscriptId, chapterId]() {
            emit chapterActivated(manuscriptId, chapterId);
        });
        m_listLayout->insertWidget(row++, btn);

        // Cenas indentadas (só aparecem quando há mais de uma).
        if (c.scenes.size() > 1) {
            for (int i = 0; i < c.scenes.size(); ++i) {
                const Scene& s = c.scenes.at(i);
                auto* sbtn = new QToolButton(this);
                const QString title = s.title.isEmpty()
                    ? tr("Cena %1").arg(i + 1)
                    : s.title;
                sbtn->setText(QStringLiteral("· %1").arg(title));
                sbtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
                sbtn->setCursor(Qt::PointingHandCursor);
                sbtn->setMinimumHeight(26);
                sbtn->setStyleSheet(sceneQss);
                sbtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

                const int sceneIdx = i;
                connect(sbtn, &QToolButton::clicked, this, [this, manuscriptId, chapterId, sceneIdx]() {
                    emit sceneActivated(manuscriptId, chapterId, sceneIdx);
                });
                m_listLayout->insertWidget(row++, sbtn);
            }
        }
    }
}
