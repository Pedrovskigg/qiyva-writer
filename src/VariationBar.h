#pragma once

#include <QWidget>

class QHBoxLayout;
class QLabel;
class QToolButton;
class ProjectModel;
class EditorHost;

class VariationBar : public QWidget {
    Q_OBJECT
public:
    VariationBar(ProjectModel* model, EditorHost* host, QWidget* parent = nullptr);

public slots:
    void refresh();

private:
    void rebuildButtons();
    void setEmptyState(bool empty);

    ProjectModel* m_model;
    EditorHost* m_host;
    QHBoxLayout* m_buttonsLayout;
    QLabel* m_label;
    QToolButton* m_newBtn;
    QToolButton* m_primaryBtn;
    QToolButton* m_deleteBtn;
};
