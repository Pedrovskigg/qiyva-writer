#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFontDatabase>
#include <QLocale>
#include <QSet>
#include <QStringList>
#include <QTranslator>

#include "MainWindow.h"

namespace {

QStringList registerCustomFonts()
{
    QString fontsDir = QCoreApplication::applicationDirPath() + QStringLiteral("/fonts");
    if (!QDir(fontsDir).exists()) {
        fontsDir = QString::fromUtf8(DEV_ASSETS_DIR) + QStringLiteral("/fonts");
    }
    if (!QDir(fontsDir).exists()) {
        return {};
    }

    QSet<QString> families;
    QDirIterator it(fontsDir,
                    {QStringLiteral("*.ttf"), QStringLiteral("*.otf")},
                    QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const int id = QFontDatabase::addApplicationFont(path);
        if (id != -1) {
            for (const QString &family : QFontDatabase::applicationFontFamilies(id)) {
                families.insert(family);
            }
        }
    }
    QStringList result = families.values();
    result.sort(Qt::CaseInsensitive);
    return result;
}

}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName("Mira Writing");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("Mira Writing");

    app.setStyleSheet(R"(
        QMainWindow {
            background-color: #141414;
        }
        #editorContainer {
            background-color: #141414;
        }
        QTextEdit {
            background-color: #1d1d1d;
            border: none;
            padding: 80px 100px;
            selection-background-color: #2c578a;
        }
        #topToolbar {
            background-color: #161616;
            border-bottom: 1px solid #232323;
        }
        #topToolbar QToolButton {
            background: transparent;
            color: #8a857a;
            border: none;
            padding: 4px 6px;
            font-size: 12px;
        }
        #topToolbar QToolButton:hover {
            color: #d8d3c6;
        }
        #topToolbar QToolButton:checked {
            color: #d8d3c6;
        }
        #topToolbar QToolButton::menu-indicator {
            image: none;
            width: 0;
        }
        #topToolbar QToolButton#ttbFont {
            min-width: 130px;
            color: #c8c3b6;
            padding: 4px 0;
        }
        #topToolbar QToolButton#ttbFont:hover {
            color: #f0e8d8;
        }
        #topToolbar QToolButton#ttbSize,
        #topToolbar QToolButton#ttbLineHeight {
            min-width: 60px;
            padding: 4px 0;
        }
        #topToolbar QToolButton#ttbIndent {
            font-size: 14px;
            min-width: 18px;
            padding: 4px 0;
        }
        #topToolbar QToolButton#ttbImage,
        #topToolbar QToolButton#ttbFocus {
            min-width: 26px;
            padding: 4px 0;
        }
        #ttbSizeStepper QToolButton#ttbSizeStep {
            background: #2a2a2a;
            color: #c8c3b6;
            border: none;
            border-radius: 4px;
            font-size: 16px;
            font-weight: bold;
        }
        #ttbSizeStepper QToolButton#ttbSizeStep:hover {
            background: #3a3a3a;
            color: #f0e8d8;
        }
        #ttbSizeStepper QLabel#ttbSizeValue {
            color: #e8e3d6;
            font-size: 15px;
            font-weight: bold;
        }
        QMenu {
            background-color: #2a2a2a;
            color: #c8c3b6;
            border: 1px solid #3a3a3a;
            padding: 4px;
        }
        QMenu::item {
            padding: 6px 24px;
        }
        QMenu::item:selected {
            background-color: #3a3a3a;
            border-radius: 3px;
        }
        #fontPickerPopup {
            background-color: #1c1c1c;
            border: 1px solid #2e2e2e;
        }
        #fontPickerList {
            background-color: #1c1c1c;
            color: #c8c3b6;
            border: none;
            outline: none;
            padding: 4px;
        }
        #fontPickerList::item {
            padding: 8px 12px;
            border-radius: 3px;
        }
        #fontPickerList::item:hover {
            background-color: #262626;
            color: #e8e3d6;
        }
        #fontPickerList::item:selected {
            background-color: #2e2e2e;
            color: #f0e8d8;
        }
        #imageInsertDialog {
            background-color: #1a1a1a;
        }
        #imageInsertDialog QLabel {
            color: #c8c3b6;
            font-size: 12px;
        }
        #imageInsertDialog #imagePreview {
            background-color: #1c1c1c;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            color: #6a6558;
        }
        #imageInsertDialog QRadioButton {
            color: #c8c3b6;
            spacing: 6px;
            font-size: 12px;
        }
        #imageInsertDialog QRadioButton::indicator {
            width: 14px;
            height: 14px;
            border-radius: 7px;
            border: 1px solid #4a4a40;
            background: #1c1c1c;
        }
        #imageInsertDialog QRadioButton::indicator:checked {
            background: #d8d3c6;
            border-color: #d8d3c6;
        }
        #imageInsertDialog QSlider::groove:horizontal {
            background: #2a2a2a;
            height: 4px;
            border-radius: 2px;
        }
        #imageInsertDialog QSlider::handle:horizontal {
            background: #d8d3c6;
            width: 14px;
            height: 14px;
            margin: -6px 0;
            border-radius: 7px;
        }
        #imageInsertDialog QSpinBox {
            background: #1c1c1c;
            color: #e8e3d6;
            border: 1px solid #2e2e2e;
            border-radius: 4px;
            padding: 4px 6px;
        }
        #imageInsertDialog QPushButton {
            background: #2a2a2a;
            color: #c8c3b6;
            border: none;
            padding: 8px 18px;
            border-radius: 4px;
            font-size: 12px;
        }
        #imageInsertDialog QPushButton:hover {
            background: #3a3a3a;
            color: #e8e3d6;
        }
        #imageInsertDialog QPushButton:default {
            background: #4a4a3a;
            color: #f0e8d8;
        }
        #imageInsertDialog QPushButton:default:hover {
            background: #5a5a48;
        }
        #imageOverlay {
            background-color: rgba(20, 20, 20, 235);
            border: 1px solid #2e2e2e;
            border-radius: 6px;
        }
        #imageOverlay QToolButton#imgOverlayBtn {
            background: transparent;
            color: #c8c3b6;
            border: none;
            padding: 4px 8px;
            border-radius: 3px;
            font-size: 14px;
            min-width: 22px;
        }
        #imageOverlay QToolButton#imgOverlayBtn:hover {
            background-color: #2a2a2a;
            color: #f0e8d8;
        }
        #imageOverlay QToolButton#imgOverlayBtn[active="true"] {
            background-color: #4a4a3a;
            color: #f0e8d8;
        }
        #imageOverlay QLabel#imgOverlayWidth {
            color: #e8e3d6;
            font-size: 12px;
            font-weight: bold;
        }
        #imageOverlay QFrame#imgOverlaySep {
            color: #2e2e2e;
        }
        QScrollBar:vertical {
            background: #1a1a1a;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #3a3a3a;
            border-radius: 5px;
            min-height: 30px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
        }
    )");

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "mira_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            QApplication::installTranslator(&translator);
            break;
        }
    }

    const QStringList customFontFamilies = registerCustomFonts();

    MainWindow window;
    window.setAvailableFontFamilies(customFontFamilies);
    window.show();

    return app.exec();
}
