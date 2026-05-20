#pragma once

#include <QWidget>

class QLabel;
class QVBoxLayout;
class QScrollArea;
class ProjectModel;
class ElementsStore;

class DrawerListPanel : public QWidget {
    Q_OBJECT
public:
    explicit DrawerListPanel(ProjectModel* model, QWidget* parent = nullptr);

    void setElementsStore(ElementsStore* store);

    void openDrawer(const QString& drawerKey, const QString& folderId = QString());
    void closePanel();
    bool isPanelOpen() const { return !m_currentKey.isEmpty(); }
    QString currentDrawerKey() const { return m_currentKey; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void itemActivated(QString drawerKey, QString itemId);
    void newItemRequested(QString drawerKey, QString folderId);
    void newFolderRequested(QString drawerKey, QString parentFolderId);
    void panelClosed();

private slots:
    void onDrawersChanged();

private:
    void rebuildContents();
    void enterFolder(const QString& folderId);
    void goUpOneLevel();
    void updateBreadcrumb();
    void showItemContextMenu(const QString& itemId, const QPoint& globalPos);
    void showFolderContextMenu(const QString& folderId, const QPoint& globalPos);

    QWidget* makeRow(const QString& label, bool isFolder, const QString& id);
    QWidget* makeElementCard(const QString& itemId, const QString& title, const QString& role, const QString& elementId);
    QWidget* makeEmptyState();
    QString folderTitle(const QString& folderId) const;
    QStringList ancestorFolderIds(const QString& folderId) const;

    ProjectModel* m_model;
    ElementsStore* m_elementsStore = nullptr;
    QString m_currentKey;
    QString m_currentFolderId; // vazio = raiz da gaveta
    QLabel* m_titleLabel;
    QVBoxLayout* m_listLayout;
    QScrollArea* m_scroll;
};
