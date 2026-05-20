#include "TopToolbar.h"

#include <QAction>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPair>
#include <QToolButton>
#include <QWidgetAction>

#include "FontPickerPopup.h"
#include "IconUtils.h"

namespace {

constexpr auto kIconNormal   = "#8a857a";
constexpr auto kIconHover    = "#d8d3c6";
constexpr auto kIconSelected = "#f0e8d8";

QToolButton *makeFlatButton(QWidget *parent)
{
    auto *b = new QToolButton(parent);
    b->setToolButtonStyle(Qt::ToolButtonTextOnly);
    b->setAutoRaise(true);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

QIcon loadIcon(const QString &name)
{
    return IconUtils::loadToolbarIcon(
        QStringLiteral(":/icons/%1").arg(name),
        QColor(kIconNormal),
        QColor(kIconHover),
        QColor(kIconSelected));
}

int fontButtonPointSize(int editorPt)
{
    return qBound(10, editorPt * 2 / 3, 18);
}

}

TopToolbar::TopToolbar(QWidget *parent)
    : QWidget(parent)
    , newProjectButton(makeFlatButton(this))
    , openProjectButton(makeFlatButton(this))
    , saveProjectButton(makeFlatButton(this))
    , fontButton(makeFlatButton(this))
    , sizeButton(makeFlatButton(this))
    , lineHeightButton(makeFlatButton(this))
    , indentButton(makeFlatButton(this))
    , imageButton(makeFlatButton(this))
    , focusButton(makeFlatButton(this))
    , refMenuButton(makeFlatButton(this))
    , fontPicker(nullptr)
    , sizeStepperValueLabel(nullptr)
    , currentFontFamily(QStringLiteral("Alegreya"))
    , currentFontSize(16)
    , currentLineHeightPercent(170)
{
    setObjectName(QStringLiteral("topToolbar"));
    setFixedHeight(36);

    focusOffIcon = loadIcon(QStringLiteral("focusmode-off.svg"));
    focusOnIcon  = loadIcon(QStringLiteral("focusmode-on.svg"));

    fontButton->setObjectName(QStringLiteral("ttbFont"));
    fontButton->setText(currentFontFamily);
    applyFontButtonStyle();

    fontPicker = new FontPickerPopup(this);
    connect(fontPicker, &FontPickerPopup::fontSelected, this, [this](const QString &family) {
        currentFontFamily = family;
        fontButton->setText(family);
        applyFontButtonStyle();
        emit fontFamilyChanged(family);
    });
    connect(fontButton, &QToolButton::clicked, this, [this]() {
        if (!fontPicker) return;
        fontPicker->setFontFamilies(fontFamilies, currentFontFamily);
        const QPoint anchor = fontButton->mapToGlobal(QPoint(0, fontButton->height()));
        fontPicker->showAtBelow(anchor);
    });

    const QSize iconSize(18, 18);

    sizeButton->setObjectName(QStringLiteral("ttbSize"));
    sizeButton->setPopupMode(QToolButton::InstantPopup);
    sizeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    sizeButton->setIcon(loadIcon(QStringLiteral("font-size.svg")));
    sizeButton->setIconSize(iconSize);
    sizeButton->setText(QString::number(currentFontSize));
    sizeButton->setToolTip(tr("Tamanho da fonte"));

    lineHeightButton->setObjectName(QStringLiteral("ttbLineHeight"));
    lineHeightButton->setPopupMode(QToolButton::InstantPopup);
    lineHeightButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    lineHeightButton->setIcon(loadIcon(QStringLiteral("text-spacing.svg")));
    lineHeightButton->setIconSize(iconSize);
    lineHeightButton->setText(QString::number(currentLineHeightPercent / 100.0, 'f', 1));
    lineHeightButton->setToolTip(tr("Espaçamento da linha"));

    indentButton->setObjectName(QStringLiteral("ttbIndent"));
    indentButton->setText(QStringLiteral("¶"));
    indentButton->setCheckable(true);
    indentButton->setChecked(true);
    indentButton->setToolTip(tr("Identação de parágrafo"));
    connect(indentButton, &QToolButton::toggled, this, &TopToolbar::firstLineIndentToggled);

    imageButton->setObjectName(QStringLiteral("ttbImage"));
    imageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    imageButton->setIcon(loadIcon(QStringLiteral("add-image.svg")));
    imageButton->setIconSize(iconSize);
    imageButton->setToolTip(tr("Adicionar imagem"));
    connect(imageButton, &QToolButton::clicked, this, &TopToolbar::addImageRequested);

    focusButton->setObjectName(QStringLiteral("ttbFocus"));
    focusButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    focusButton->setIcon(focusOffIcon);
    focusButton->setIconSize(iconSize);
    focusButton->setCheckable(true);
    focusButton->setToolTip(tr("Modo foco"));
    connect(focusButton, &QToolButton::toggled, this, [this](bool on) {
        focusButton->setIcon(on ? focusOnIcon : focusOffIcon);
        emit focusModeToggled(on);
    });

    refMenuButton->setObjectName(QStringLiteral("ttbRefMenu"));
    refMenuButton->setText(QStringLiteral("☰"));
    refMenuButton->setToolTip(tr("Painel de Referência"));
    connect(refMenuButton, &QToolButton::clicked, this, &TopToolbar::refMenuToggleRequested);

    newProjectButton->setObjectName(QStringLiteral("ttbProject"));
    newProjectButton->setText(tr("Novo"));
    newProjectButton->setToolTip(tr("Novo projeto"));
    connect(newProjectButton, &QToolButton::clicked, this, &TopToolbar::newProjectRequested);

    openProjectButton->setObjectName(QStringLiteral("ttbProject"));
    openProjectButton->setText(tr("Abrir"));
    openProjectButton->setToolTip(tr("Abrir projeto"));
    connect(openProjectButton, &QToolButton::clicked, this, &TopToolbar::openProjectRequested);

    saveProjectButton->setObjectName(QStringLiteral("ttbProject"));
    saveProjectButton->setText(tr("Salvar"));
    saveProjectButton->setToolTip(tr("Salvar projeto (Ctrl+S)"));
    connect(saveProjectButton, &QToolButton::clicked, this, &TopToolbar::saveProjectRequested);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(14);

    layout->addWidget(newProjectButton);
    layout->addWidget(openProjectButton);
    layout->addWidget(saveProjectButton);
    layout->addStretch();
    layout->addWidget(fontButton);
    layout->addWidget(sizeButton);
    layout->addWidget(lineHeightButton);
    layout->addWidget(indentButton);
    layout->addWidget(imageButton);
    layout->addWidget(focusButton);
    layout->addWidget(refMenuButton);
    layout->addStretch();

    buildSizeMenu();
    buildLineHeightMenu();
}

void TopToolbar::setFontFamilies(const QStringList &families, const QString &current)
{
    fontFamilies = families;
    currentFontFamily = current;
    fontButton->setText(currentFontFamily);
    applyFontButtonStyle();
}

void TopToolbar::setFontSize(int pt)
{
    currentFontSize = pt;
    updateSizeMenuState();
}

void TopToolbar::setLineHeightPercent(int percent)
{
    currentLineHeightPercent = percent;
    lineHeightButton->setText(QString::number(percent / 100.0, 'f', 1));
    buildLineHeightMenu();
}

void TopToolbar::setFirstLineIndentEnabled(bool enabled)
{
    indentButton->setChecked(enabled);
}

void TopToolbar::buildSizeMenu()
{
    auto *menu = new QMenu(sizeButton);
    menu->setObjectName(QStringLiteral("ttbSizeMenu"));

    auto *stepperWidget = new QWidget(menu);
    stepperWidget->setObjectName(QStringLiteral("ttbSizeStepper"));
    auto *stepperLayout = new QHBoxLayout(stepperWidget);
    stepperLayout->setContentsMargins(10, 6, 10, 6);
    stepperLayout->setSpacing(6);

    auto *minus = new QToolButton(stepperWidget);
    minus->setObjectName(QStringLiteral("ttbSizeStep"));
    minus->setText(QStringLiteral("−"));
    minus->setFixedSize(28, 28);
    minus->setCursor(Qt::PointingHandCursor);
    minus->setAutoRaise(true);

    sizeStepperValueLabel = new QLabel(stepperWidget);
    sizeStepperValueLabel->setObjectName(QStringLiteral("ttbSizeValue"));
    sizeStepperValueLabel->setText(QString::number(currentFontSize));
    sizeStepperValueLabel->setAlignment(Qt::AlignCenter);
    sizeStepperValueLabel->setFixedWidth(40);

    auto *plus = new QToolButton(stepperWidget);
    plus->setObjectName(QStringLiteral("ttbSizeStep"));
    plus->setText(QStringLiteral("+"));
    plus->setFixedSize(28, 28);
    plus->setCursor(Qt::PointingHandCursor);
    plus->setAutoRaise(true);

    stepperLayout->addWidget(minus);
    stepperLayout->addWidget(sizeStepperValueLabel);
    stepperLayout->addWidget(plus);

    connect(minus, &QToolButton::clicked, this, [this]() { applySize(currentFontSize - 2); });
    connect(plus, &QToolButton::clicked, this, [this]() { applySize(currentFontSize + 2); });

    auto *stepperAction = new QWidgetAction(menu);
    stepperAction->setDefaultWidget(stepperWidget);
    menu->addAction(stepperAction);

    menu->addSeparator();

    sizePresetActions.clear();
    const QList<int> presets = {12, 14, 16, 18, 20, 24, 28};
    for (int s : presets) {
        QAction *a = menu->addAction(QString::number(s) + tr(" pt"));
        a->setCheckable(true);
        a->setChecked(s == currentFontSize);
        a->setData(s);
        connect(a, &QAction::triggered, this, [this, s]() { applySize(s); });
        sizePresetActions.append(a);
    }

    sizeButton->setMenu(menu);
}

void TopToolbar::updateSizeMenuState()
{
    sizeButton->setText(QString::number(currentFontSize));
    applyFontButtonStyle();
    if (sizeStepperValueLabel) {
        sizeStepperValueLabel->setText(QString::number(currentFontSize));
    }
    for (QAction *a : std::as_const(sizePresetActions)) {
        a->setChecked(a->data().toInt() == currentFontSize);
    }
}

void TopToolbar::applyFontButtonStyle()
{
    QFont f(currentFontFamily, fontButtonPointSize(currentFontSize));
    f.setBold(true);
    fontButton->setFont(f);
}

void TopToolbar::buildLineHeightMenu()
{
    const QList<QPair<int, QString>> spacings = {
        { 130, tr("Compacto (1.3)") },
        { 150, tr("Confortável (1.5)") },
        { 170, tr("Padrão (1.7)") },
        { 190, tr("Amplo (1.9)") },
        { 220, tr("Espaçoso (2.2)") },
    };

    auto *menu = new QMenu(lineHeightButton);
    for (const auto &sp : spacings) {
        const int percent = sp.first;
        QAction *a = menu->addAction(sp.second);
        a->setCheckable(true);
        a->setChecked(percent == currentLineHeightPercent);
        connect(a, &QAction::triggered, this, [this, percent]() {
            currentLineHeightPercent = percent;
            lineHeightButton->setText(QStringLiteral("AA  %1").arg(percent / 100.0, 0, 'f', 1));
            emit lineHeightChanged(percent);
            buildLineHeightMenu();
        });
    }
    lineHeightButton->setMenu(menu);
}

void TopToolbar::applySize(int pt)
{
    const int minSize = 10;
    const int maxSize = 48;
    int next = qBound(minSize, pt, maxSize);
    if (next == currentFontSize) return;
    currentFontSize = next;
    updateSizeMenuState();
    emit fontSizeChanged(next);
}
