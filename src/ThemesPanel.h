#pragma once

#include "Theme.h"

#include <QDialog>
#include <QList>
#include <QPointer>
#include <QString>

class QFrame;
class QLabel;
class QPushButton;
class QScrollArea;
class QTabWidget;
class QVBoxLayout;
class QWidget;

// Painel de Temas — estilo FocusWriter. Tabs Padrão / Personalizado, cada
// tab com grid de cards visuais. Ações: Novo, Duplicar, Editar, Excluir
// (Editar/Excluir só na aba Personalizado). Footer: Aplicar + Fechar.
class ThemesPanel : public QDialog {
    Q_OBJECT
public:
    explicit ThemesPanel(QWidget* parent = nullptr);

private slots:
    void onApplyClicked();
    void onNewClicked();
    void onDuplicateClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onTabChanged(int index);
    void onThemeChanged();
    void onCustomThemesChanged();

private:
    enum Tab { TabBundled = 0, TabCustom = 1 };

    void buildUi();
    void rebuildGrids();
    void rebuildOneGrid(QWidget* gridContainer, const QList<Theme::MiraTheme>& themes, bool custom);
    void selectId(const QString& id);
    void updateButtonsState();
    void applyDialogStyle();

    Tab activeTab() const;
    bool selectedIsCustom() const;

    QTabWidget* m_tabs;
    QScrollArea* m_bundledScroll;
    QScrollArea* m_customScroll;
    QWidget* m_bundledGrid;
    QWidget* m_customGrid;

    QPushButton* m_applyButton;
    QPushButton* m_newButton;
    QPushButton* m_duplicateButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_closeButton;

    QLabel* m_selectionInfo;

    QString m_selectedId;
    QList<QPointer<QFrame>> m_cards; // todos os cards visíveis (ambas as abas)
};
