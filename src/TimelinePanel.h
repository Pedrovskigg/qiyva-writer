#pragma once

#include "TimelineTypes.h"

#include <QList>
#include <QString>
#include <QWidget>
#include <functional>

class TimelineScene;
class TimelineView;
class ProjectModel;
class ElementsStore;

class TimelinePanel : public QWidget {
    Q_OBJECT
public:
    explicit TimelinePanel(QWidget* parent = nullptr);

    void setProjectRoot(const QString& root);
    void setProjectModel(ProjectModel* model);
    void setElementsStore(ElementsStore* store);
    // Resolve o texto de um doc vinculado (capítulo/cena/gaveta) p/ a descrição.
    void setDocTextResolver(std::function<QString(const QString&)> resolver);

    // Abre o popup de novo evento já pré-preenchido (ex.: criado a partir de um
    // trecho selecionado no editor). Usado pelo MainWindow.
    void promptNewEvent(const QString& description, const QString& marker);

signals:
    void closeRequested();
    void exportEventAsDoc(TimelineEvent data);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event)   override;

private slots:
    void applyTheme();
    void createTimeline();
    void createEventAt(const QPointF& scenePos);
    void commitEvent(TimelineEvent e, const QPointF& scenePos); // cria o evento já resolvido
    void openEditPopup(const QString& eventId);
    void onExportEventAsDoc(const TimelineEvent& event);

private:
    void buildUi();
    void save() const;
    void load();
    void toggleViewMode();
    void toggleAxisMode();
    void refreshModeButtons();
    // Foco / Filtro por linha
    void rebuildFocusMenu();                 // repopula o menu com as linhas atuais
    void refreshFocusButtons();              // texto/estado dos botões de foco
    void setFocusTimeline(const QString& id); // id vazio = mostrar tudo
    void setFocusDepth(int depth);
    // Gera/atualiza as trilhas automáticas de personagem a partir do detector
    // de presença + roles. askSecondary = perguntar sobre papéis secundários.
    void syncCharacterTimelines(bool askSecondary);

    TimelineScene*  m_scene        = nullptr;
    TimelineView*   m_view         = nullptr;
    QWidget*        m_toolbar      = nullptr;
    class QToolButton* m_btnView   = nullptr;
    class QToolButton* m_btnAxis   = nullptr;
    class QToolButton* m_btnFocus  = nullptr;
    class QToolButton* m_btnDepth  = nullptr;
    class QToolButton* m_btnAdd    = nullptr;   // "+" flutuante sobre o canvas
    class QMenu*       m_focusMenu = nullptr;

    QString         m_projectRoot;
    ProjectModel*   m_projectModel  = nullptr;
    ElementsStore*  m_elementsStore = nullptr;
    std::function<QString(const QString&)> m_docTextResolver;

    // dados — preenchidos nas etapas seguintes
    QList<TimelineDef>   m_timelines;
    QList<TimelineEvent> m_events;
    QList<TimelineConn>  m_connections;
};
