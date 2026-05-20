#include "ElementCreateDialog.h"

#include <QBuffer>
#include <QByteArray>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString labelForType(const QString& t) {
    if (t == QStringLiteral("character")) return QObject::tr("Novo personagem");
    if (t == QStringLiteral("setting"))   return QObject::tr("Novo cenário");
    if (t == QStringLiteral("object"))    return QObject::tr("Novo objeto");
    return QObject::tr("Novo documento");
}

QString nameLabelForType(const QString& t) {
    if (t == QStringLiteral("character")) return QObject::tr("Nome do personagem:");
    if (t == QStringLiteral("setting"))   return QObject::tr("Nome do cenário:");
    if (t == QStringLiteral("object"))    return QObject::tr("Nome do objeto:");
    return QObject::tr("Nome:");
}

// Carrega imagem do disco, redimensiona pra 400×400 quadrado (crop centralizado)
// e devolve como data URL JPEG base64.
QString loadImageAsDataUrl(const QString& path) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage img = reader.read();
    if (img.isNull()) return QString();
    // Crop centralizado pra quadrado
    const int side = qMin(img.width(), img.height());
    const int x = (img.width() - side) / 2;
    const int y = (img.height() - side) / 2;
    QImage square = img.copy(x, y, side, side);
    QImage scaled = square.scaled(400, 400, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    scaled.save(&buf, "JPEG", 92);
    return QStringLiteral("data:image/jpeg;base64,") + QString::fromLatin1(bytes.toBase64());
}

QPixmap pixmapFromDataUrl(const QString& dataUrl) {
    if (dataUrl.isEmpty()) return QPixmap();
    const int comma = dataUrl.indexOf(QLatin1Char(','));
    if (comma < 0) return QPixmap();
    const QByteArray raw = QByteArray::fromBase64(dataUrl.mid(comma + 1).toLatin1());
    QPixmap pm;
    pm.loadFromData(raw);
    return pm;
}

}

ElementCreateDialog::ElementCreateDialog(const QString& elementType, QWidget* parent)
    : QDialog(parent)
    , m_elementType(elementType)
    , m_titleEdit(nullptr)
    , m_roleCombo(nullptr)
    , m_imagePreview(nullptr)
    , m_pickImageBtn(nullptr)
    , m_clearImageBtn(nullptr)
    , m_okBtn(nullptr)
    , m_cancelBtn(nullptr)
{
    setObjectName(QStringLiteral("elementCreateDialog"));
    setWindowTitle(labelForType(elementType));
    setModal(true);
    buildUi();
    setMinimumWidth(360);
}

void ElementCreateDialog::buildUi()
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(16, 16, 16, 16);
    outer->setSpacing(10);

    // Foto (se for elemento visual)
    const bool visual = m_elementType == QStringLiteral("character")
        || m_elementType == QStringLiteral("setting")
        || m_elementType == QStringLiteral("object");
    if (visual) {
        auto* imgRow = new QHBoxLayout();
        m_imagePreview = new QLabel(this);
        m_imagePreview->setObjectName(QStringLiteral("ecdImagePreview"));
        m_imagePreview->setFixedSize(96, 96);
        m_imagePreview->setAlignment(Qt::AlignCenter);
        m_imagePreview->setText(tr("sem foto"));
        imgRow->addWidget(m_imagePreview);

        auto* imgBtnsCol = new QVBoxLayout();
        m_pickImageBtn = new QPushButton(tr("Escolher foto…"), this);
        m_pickImageBtn->setObjectName(QStringLiteral("ecdBtn"));
        m_pickImageBtn->setCursor(Qt::PointingHandCursor);
        connect(m_pickImageBtn, &QPushButton::clicked, this, &ElementCreateDialog::pickImage);
        m_clearImageBtn = new QPushButton(tr("Remover"), this);
        m_clearImageBtn->setObjectName(QStringLiteral("ecdBtn"));
        m_clearImageBtn->setCursor(Qt::PointingHandCursor);
        connect(m_clearImageBtn, &QPushButton::clicked, this, &ElementCreateDialog::clearImage);
        imgBtnsCol->addWidget(m_pickImageBtn);
        imgBtnsCol->addWidget(m_clearImageBtn);
        imgBtnsCol->addStretch();
        imgRow->addLayout(imgBtnsCol);
        imgRow->addStretch();
        outer->addLayout(imgRow);
    }

    // Nome
    auto* nameLabel = new QLabel(nameLabelForType(m_elementType), this);
    outer->addWidget(nameLabel);
    m_titleEdit = new QLineEdit(this);
    outer->addWidget(m_titleEdit);

    // Role (só personagem)
    if (m_elementType == QStringLiteral("character")) {
        auto* roleLabel = new QLabel(tr("Papel:"), this);
        outer->addWidget(roleLabel);
        m_roleCombo = new QComboBox(this);
        m_roleCombo->setEditable(true);
        m_roleCombo->addItem(tr("Protagonista"), QStringLiteral("PROTAGONISTA"));
        m_roleCombo->addItem(tr("Coadjuvante"), QStringLiteral("COADJUVANTE"));
        m_roleCombo->addItem(tr("Antagonista"), QStringLiteral("ANTAGONISTA"));
        m_roleCombo->addItem(tr("Contraponto"), QStringLiteral("CONTRAPONTO"));
        m_roleCombo->addItem(tr("Trickster"), QStringLiteral("TRICKSTER"));
        m_roleCombo->addItem(tr("Mentor"), QStringLiteral("MENTOR"));
        m_roleCombo->addItem(tr("Figurante"), QStringLiteral("FIGURANTE"));
        m_roleCombo->setCurrentIndex(-1);
        outer->addWidget(m_roleCombo);
    }

    outer->addStretch();

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    m_cancelBtn = new QPushButton(tr("Cancelar"), this);
    m_cancelBtn->setObjectName(QStringLiteral("ecdBtn"));
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    m_okBtn = new QPushButton(tr("Criar"), this);
    m_okBtn->setObjectName(QStringLiteral("ecdBtn"));
    m_okBtn->setDefault(true);
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_okBtn);
    outer->addLayout(btnRow);

    setStyleSheet(QStringLiteral(R"(
        QDialog#elementCreateDialog { background: #1a1a1a; }
        QDialog#elementCreateDialog QLabel { color: #c8c3b6; font-size: 12px; }
        QDialog#elementCreateDialog QLineEdit, QDialog#elementCreateDialog QComboBox {
            background: #161616; color: #e8e3d6;
            border: 1px solid #2a2a2a; border-radius: 4px;
            padding: 6px 8px; min-height: 24px;
        }
        QDialog#elementCreateDialog QComboBox::drop-down { border: none; width: 16px; }
        QDialog#elementCreateDialog QComboBox QAbstractItemView {
            background: #161616; color: #e8e3d6;
            selection-background-color: #2c3a5e;
            border: 1px solid #2a2a2a;
        }
        QLabel#ecdImagePreview {
            background: #161616; color: #6a6558;
            border: 1px dashed #2a2a2a; border-radius: 4px;
            font-size: 10px;
        }
        QPushButton#ecdBtn {
            background: #232323; color: #c8c3b6;
            border: 1px solid #2a2a2a; border-radius: 4px;
            padding: 6px 14px; min-height: 26px;
        }
        QPushButton#ecdBtn:hover { background: #2c2c2c; color: #e8e3d6; }
        QPushButton#ecdBtn:default { background: #2c3a5e; color: #f0e8d8; border-color: #3a4d7a; }
    )"));
}

void ElementCreateDialog::setInitial(const QString& title, const QString& role, const QString& imageDataUrl)
{
    if (m_titleEdit) m_titleEdit->setText(title);
    if (m_roleCombo) {
        const int idx = m_roleCombo->findData(role);
        if (idx >= 0) m_roleCombo->setCurrentIndex(idx);
        else m_roleCombo->setEditText(role);
    }
    m_imageDataUrl = imageDataUrl;
    updatePreview();
    if (m_okBtn) m_okBtn->setText(tr("Salvar"));
    setWindowTitle(tr("Editar elemento"));
}

void ElementCreateDialog::updatePreview()
{
    if (!m_imagePreview) return;
    if (m_imageDataUrl.isEmpty()) {
        m_imagePreview->setPixmap(QPixmap());
        m_imagePreview->setText(tr("sem foto"));
        return;
    }
    QPixmap pm = pixmapFromDataUrl(m_imageDataUrl);
    if (pm.isNull()) { m_imagePreview->setText(tr("erro")); return; }
    m_imagePreview->setPixmap(pm.scaled(96, 96, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
}

void ElementCreateDialog::pickImage()
{
    const QString path = QFileDialog::getOpenFileName(this,
        tr("Selecionar foto"),
        QString(),
        tr("Imagens (*.png *.jpg *.jpeg *.gif *.bmp *.webp)"));
    if (path.isEmpty()) return;
    const QString dataUrl = loadImageAsDataUrl(path);
    if (dataUrl.isEmpty()) return;
    m_imageDataUrl = dataUrl;
    updatePreview();
}

void ElementCreateDialog::clearImage()
{
    m_imageDataUrl.clear();
    updatePreview();
}

QString ElementCreateDialog::title() const
{
    return m_titleEdit ? m_titleEdit->text().trimmed() : QString();
}

QString ElementCreateDialog::role() const
{
    if (!m_roleCombo) return QString();
    const QString data = m_roleCombo->currentData().toString();
    if (!data.isEmpty()) return data;
    return m_roleCombo->currentText().trimmed().toUpper();
}
