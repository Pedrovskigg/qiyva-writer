#include "SpellEditor.h"

#include "SpellChecker.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QFont>
#include <QMenu>
#include <QTextCursor>

namespace {
constexpr int kMaxSuggestions = 6;
}

SpellEditor::SpellEditor(QWidget* parent)
    : QTextEdit(parent)
{
}

void SpellEditor::setSpellChecker(SpellChecker* checker)
{
    m_checker = checker;
}

void SpellEditor::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* menu = createStandardContextMenu(event->pos());

    if (m_checker && m_checker->isEnabled()) {
        QTextCursor cursor = cursorForPosition(event->pos());
        cursor.select(QTextCursor::WordUnderCursor);
        const QString word = cursor.selectedText();

        if (!word.isEmpty() && !m_checker->isCorrect(word)) {
            const QStringList suggestions = m_checker->suggest(word);

            QAction* firstStandardAction = menu->actions().isEmpty()
                ? nullptr : menu->actions().first();

            QFont boldFont;
            boldFont.setBold(true);

            // Insere as sugestões no topo do menu, em ordem.
            if (suggestions.isEmpty()) {
                QAction* noSugg = new QAction(tr("(sem sugestões)"), menu);
                noSugg->setEnabled(false);
                menu->insertAction(firstStandardAction, noSugg);
            } else {
                const int n = qMin(kMaxSuggestions, suggestions.size());
                for (int i = 0; i < n; ++i) {
                    const QString suggestion = suggestions.at(i);
                    QAction* act = new QAction(suggestion, menu);
                    act->setFont(boldFont);
                    connect(act, &QAction::triggered, this, [this, cursor, suggestion]() mutable {
                        cursor.beginEditBlock();
                        cursor.insertText(suggestion);
                        cursor.endEditBlock();
                    });
                    menu->insertAction(firstStandardAction, act);
                }
            }

            // Ações específicas da palavra: adicionar / ignorar (uma vez nesta sessão).
            menu->insertSeparator(firstStandardAction);

            QAction* addAct = new QAction(tr("Adicionar \"%1\" ao dicionário").arg(word), menu);
            connect(addAct, &QAction::triggered, this, [this, word]() {
                if (m_checker) m_checker->addToPersonalDictionary(word);
            });
            menu->insertAction(firstStandardAction, addAct);

            menu->insertSeparator(firstStandardAction);
        }
    }

    menu->exec(event->globalPos());
    delete menu;
}
