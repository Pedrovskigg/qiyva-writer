#pragma once

#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>

class QCheckBox;
class QComboBox;

class SettingsPanel : public QDialog {
    Q_OBJECT
public:
    explicit SettingsPanel(QWidget* parent = nullptr);

    // Popula a lista de idiomas disponíveis. {code, label}.
    void setAvailableSpellLanguages(const QList<QPair<QString, QString>>& langs);
    void setSpellEnabled(bool enabled);
    void setSpellLanguage(const QString& code);

    bool spellEnabled() const;
    QString spellLanguage() const;

signals:
    void spellEnabledChanged(bool enabled);
    void spellLanguageChanged(const QString& code);

private:
    void onCheckToggled(bool checked);
    void onLanguageChanged(int index);

    QCheckBox* m_spellCheck;
    QComboBox* m_langCombo;
    bool m_blockSignals = false;
};
