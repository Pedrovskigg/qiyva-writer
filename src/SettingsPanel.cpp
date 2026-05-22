#include "SettingsPanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SettingsPanel::SettingsPanel(QWidget* parent)
    : QDialog(parent)
    , m_spellCheck(new QCheckBox(tr("Ativar corretor ortográfico"), this))
    , m_langCombo(new QComboBox(this))
{
    setObjectName(QStringLiteral("settingsPanel"));
    setWindowTitle(tr("Configurações"));
    setModal(false);
    resize(420, 360);

    setStyleSheet(QStringLiteral(R"(
        #settingsPanel {
            background-color: #161616;
        }
        #settingsPanel QLabel {
            color: #c8c3b6;
            font-size: 12px;
        }
        #settingsPanel QLabel#settingsTitle {
            color: #e8e3d6;
            font-size: 18px;
            font-weight: bold;
            padding-bottom: 4px;
        }
        #settingsPanel QGroupBox {
            color: #d8d3c6;
            border: 1px solid #2a2a2a;
            border-radius: 8px;
            margin-top: 14px;
            padding-top: 14px;
            font-size: 12px;
            font-weight: bold;
        }
        #settingsPanel QGroupBox::title {
            subcontrol-origin: margin;
            left: 14px;
            padding: 0 6px;
        }
        #settingsPanel QCheckBox {
            color: #c8c3b6;
            spacing: 8px;
            font-size: 12px;
            font-weight: normal;
        }
        #settingsPanel QCheckBox::indicator {
            width: 14px;
            height: 14px;
            border-radius: 3px;
            border: 1px solid #4a4a40;
            background: #1c1c1c;
        }
        #settingsPanel QCheckBox::indicator:checked {
            background: #d8d3c6;
            border-color: #d8d3c6;
        }
        #settingsPanel QComboBox {
            background: #1c1c1c;
            color: #e8e3d6;
            border: 1px solid #2e2e2e;
            border-radius: 4px;
            padding: 4px 8px;
            font-size: 12px;
            font-weight: normal;
            min-height: 22px;
        }
        #settingsPanel QComboBox:hover {
            border-color: #3a3a3a;
        }
        #settingsPanel QComboBox QAbstractItemView {
            background: #1c1c1c;
            color: #c8c3b6;
            border: 1px solid #2e2e2e;
            selection-background-color: #2a2a2a;
            selection-color: #f0e8d8;
        }
        #settingsPanel QPushButton {
            background: #2a2a2a;
            color: #c8c3b6;
            border: none;
            padding: 6px 18px;
            border-radius: 4px;
            font-size: 12px;
        }
        #settingsPanel QPushButton:hover {
            background: #3a3a3a;
            color: #e8e3d6;
        }
    )"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 18, 20, 18);
    root->setSpacing(12);

    auto* title = new QLabel(tr("Configurações"), this);
    title->setObjectName(QStringLiteral("settingsTitle"));
    root->addWidget(title);

    // ---- Seção: Corretor ortográfico ----
    auto* spellGroup = new QGroupBox(tr("Corretor ortográfico"), this);
    auto* spellLayout = new QVBoxLayout(spellGroup);
    spellLayout->setContentsMargins(14, 8, 14, 14);
    spellLayout->setSpacing(8);

    spellLayout->addWidget(m_spellCheck);

    auto* langRow = new QVBoxLayout;
    langRow->setSpacing(4);
    auto* langLabel = new QLabel(tr("Idioma do dicionário:"), spellGroup);
    langRow->addWidget(langLabel);
    langRow->addWidget(m_langCombo);
    spellLayout->addLayout(langRow);

    auto* hint = new QLabel(
        tr("Palavras desconhecidas ganham um sublinhado vermelho. "
           "Clique com o botão direito numa delas para ver sugestões "
           "ou adicionar ao dicionário do projeto."),
        spellGroup);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: #8a857a; font-size: 11px; font-weight: normal;"));
    spellLayout->addWidget(hint);

    root->addWidget(spellGroup);

    root->addStretch();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    root->addWidget(buttons);

    connect(m_spellCheck, &QCheckBox::toggled, this, &SettingsPanel::onCheckToggled);
    connect(m_langCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &SettingsPanel::onLanguageChanged);
}

void SettingsPanel::setAvailableSpellLanguages(const QList<QPair<QString, QString>>& langs)
{
    m_blockSignals = true;
    const QString prev = spellLanguage();
    m_langCombo->clear();
    for (const auto& pair : langs) {
        m_langCombo->addItem(pair.second, pair.first);
    }
    if (!prev.isEmpty()) {
        const int idx = m_langCombo->findData(prev);
        if (idx >= 0) m_langCombo->setCurrentIndex(idx);
    }
    m_blockSignals = false;
}

void SettingsPanel::setSpellEnabled(bool enabled)
{
    m_blockSignals = true;
    m_spellCheck->setChecked(enabled);
    m_langCombo->setEnabled(enabled);
    m_blockSignals = false;
}

void SettingsPanel::setSpellLanguage(const QString& code)
{
    m_blockSignals = true;
    const int idx = m_langCombo->findData(code);
    if (idx >= 0) m_langCombo->setCurrentIndex(idx);
    m_blockSignals = false;
}

bool SettingsPanel::spellEnabled() const
{
    return m_spellCheck->isChecked();
}

QString SettingsPanel::spellLanguage() const
{
    return m_langCombo->currentData().toString();
}

void SettingsPanel::onCheckToggled(bool checked)
{
    m_langCombo->setEnabled(checked);
    if (m_blockSignals) return;
    emit spellEnabledChanged(checked);
}

void SettingsPanel::onLanguageChanged(int /*index*/)
{
    if (m_blockSignals) return;
    emit spellLanguageChanged(spellLanguage());
}
