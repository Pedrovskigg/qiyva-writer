#pragma once

#include <QWidget>

class QVBoxLayout;
class QToolButton;
class ProjectModel;

class LeftBar : public QWidget {
    Q_OBJECT
public:
    enum FixedAction {
        Info,
        Whiteboard,
        Map,
        Manuscripts,
        Consistency,
    };
    Q_ENUM(FixedAction)

    explicit LeftBar(ProjectModel* model, QWidget* parent = nullptr);

    bool isMirrored() const { return m_mirrored; }
    void setMirrored(bool mirrored);

signals:
    void fixedActionTriggered(LeftBar::FixedAction action);
    void drawerSelected(QString drawerKey);
    void newDrawerRequested();

private slots:
    void rebuildDrawerButtons();

private:
    QToolButton* makeFixedButton(const QString& placeholderLetter,
                                 const QString& tooltip,
                                 FixedAction action);
    QToolButton* makeDrawerButton(const QString& drawerKey,
                                  const QString& title,
                                  const QString& color);
    QToolButton* makeNewDrawerButton();
    void applyMirrorStyle();

    ProjectModel* m_model;
    QVBoxLayout* m_drawerLayout;
    QVBoxLayout* m_rootLayout;
    bool m_mirrored = false;
};
