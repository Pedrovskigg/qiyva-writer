#pragma once

#include <QWidget>

class QComboBox;
class QVBoxLayout;
class QScrollArea;
class QToolButton;
class ProjectModel;

class ManuscriptPanel : public QWidget {
    Q_OBJECT
public:
    explicit ManuscriptPanel(ProjectModel* model, QWidget* parent = nullptr);

    void open();
    void closePanel();
    bool isPanelOpen() const { return isVisible(); }

signals:
    void chapterActivated(QString manuscriptId, QString chapterId);
    void sceneActivated(QString manuscriptId, QString chapterId, int sceneIndex);
    void newChapterRequested(QString manuscriptId);
    void newManuscriptRequested();
    void panelClosed();

private slots:
    void onManuscriptsChanged();
    void onChaptersChanged();
    void onComboChanged(int index);

private:
    void rebuildList();
    QString activeManuscriptId() const;
    void syncCombo();

    ProjectModel* m_model;
    QComboBox* m_combo;
    QVBoxLayout* m_listLayout;
    QScrollArea* m_scroll;
};
