#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QSplitter;
class QTextBrowser;
class QToolButton;
class ProjectModel;
class EditorHost;
class DocCache;

// Painel flutuante de Referência (canto superior direito). Permite olhar e abrir
// qualquer manuscrito (capítulos) ou gaveta (items) sem perder o doc atual no editor.
// MVP: picker (manuscrito ou gaveta) + lista de docs do que foi escolhido.
class RefMenuPanel : public QWidget {
    Q_OBJECT
public:
    RefMenuPanel(ProjectModel* model, EditorHost* host, DocCache* cache, QWidget* parent = nullptr);

    void setProjectRoot(const QString& root) { m_projectRoot = root; }

    void togglePanel();
    void openPanel();
    void closePanel();

signals:
    void geometryChanged();

public slots:
    void refresh();

private slots:
    void onSourceChanged(int idx);
    void onItemActivated(QListWidgetItem* item);

private:
    void buildUi();
    void rebuildSourceCombo();
    void rebuildDocList();
    QString resolveDocHtml(const QString& key) const;

    ProjectModel* m_model;
    EditorHost* m_host;
    DocCache* m_cache;
    QString m_projectRoot;
    QComboBox* m_sourceCombo;
    QListWidget* m_docList;
    QTextBrowser* m_preview;
    QLabel* m_emptyLabel;
    QSplitter* m_splitter;
    QToolButton* m_closeBtn;
};
