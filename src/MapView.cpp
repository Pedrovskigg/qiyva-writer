#include "MapView.h"

#include "GeoData.h"
#include "MapPinsStore.h"
#include "Theme.h"

#include <QDateTime>
#include <QHash>
#include <QLineF>
#include <QLocale>
#include <QTimeZone>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QSettings>
#include <QShowEvent>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace {
struct Continent { const char* name; double lon; double lat; };
const Continent kContinents[] = {
    { "América do Norte", -100.0, 48.0 },
    { "América do Sul",    -60.0, -12.0 },
    { "Europa",             14.0,  52.0 },
    { "África",             20.0,   2.0 },
    { "Ásia",               90.0,  47.0 },
    { "Oceania",           140.0, -25.0 },
    { "Antártida",           0.0, -82.0 },
};

QString fmtNum(qint64 n)
{
    return QLocale(QLocale::Portuguese, QLocale::Brazil).toString(n);
}

// Interpola entre duas cores (t de 0 a 1) — pra derivar terra/fronteira do tema
// com contraste garantido.
QColor mix(const QColor& a, const QColor& b, double t)
{
    return QColor(int(a.red()   + (b.red()   - a.red())   * t),
                  int(a.green() + (b.green() - a.green()) * t),
                  int(a.blue()  + (b.blue()  - a.blue())  * t));
}

constexpr double kPi = 3.14159265358979323846;

// Distância em km entre dois pontos geográficos (Haversine).
double haversineKm(double lat1, double lon1, double lat2, double lon2)
{
    constexpr double R = 6371.0, D = kPi / 180.0;
    const double dLat = (lat2 - lat1) * D, dLon = (lon2 - lon1) * D;
    const double a = std::sin(dLat / 2) * std::sin(dLat / 2)
        + std::cos(lat1 * D) * std::cos(lat2 * D) * std::sin(dLon / 2) * std::sin(dLon / 2);
    return R * 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
}

// Offset UTC (segundos) de um ponto: usa o fuso da cidade mais próxima; se não
// houver cidade por perto (oceano), estima pela longitude.
int utcOffsetSec(double lon, double lat)
{
    const GeoData& geo = GeoData::instance();
    const GeoData::Place* best = nullptr;
    double bestD = 1e18;
    for (const GeoData::Place& p : geo.places()) {
        const double d = (p.lon - lon) * (p.lon - lon) + (p.lat - lat) * (p.lat - lat);
        if (d < bestD) { bestD = d; best = &p; }
    }
    if (best && bestD < 25.0 && !best->timezone.isEmpty()) {
        const QTimeZone tz(best->timezone.toUtf8());
        if (tz.isValid()) return tz.offsetFromUtc(QDateTime::currentDateTimeUtc());
    }
    return int(std::round(lon / 15.0)) * 3600;
}

QString continentName(const QString& code)
{
    if (code == QLatin1String("SA")) return QStringLiteral("América do Sul");
    if (code == QLatin1String("NA")) return QStringLiteral("América do Norte");
    if (code == QLatin1String("EU")) return QStringLiteral("Europa");
    if (code == QLatin1String("AS")) return QStringLiteral("Ásia");
    if (code == QLatin1String("AF")) return QStringLiteral("África");
    if (code == QLatin1String("OC")) return QStringLiteral("Oceania");
    if (code == QLatin1String("AN")) return QStringLiteral("Antártida");
    return code;
}

QString languageNames(const QString& codes)
{
    static const QHash<QString, QString> names = {
        {"af","Africâner"},{"sq","Albanês"},{"am","Amárico"},{"ar","Árabe"},
        {"hy","Armênio"},{"az","Azerbaijano"},{"eu","Basco"},{"be","Bielorrusso"},
        {"bn","Bengali"},{"bs","Bósnio"},{"bg","Búlgaro"},{"my","Birmanês"},
        {"ca","Catalão"},{"zh","Chinês"},{"hr","Croata"},{"cs","Tcheco"},
        {"da","Dinamarquês"},{"nl","Holandês"},{"en","Inglês"},{"et","Estoniano"},
        {"fi","Finlandês"},{"fr","Francês"},{"gl","Galego"},{"ka","Georgiano"},
        {"de","Alemão"},{"el","Grego"},{"gu","Guzerate"},{"ha","Hauçá"},
        {"he","Hebraico"},{"hi","Híndi"},{"hu","Húngaro"},{"is","Islandês"},
        {"ig","Igbo"},{"id","Indonésio"},{"ga","Irlandês"},{"it","Italiano"},
        {"ja","Japonês"},{"kk","Cazaque"},{"km","Khmer"},{"ko","Coreano"},
        {"ku","Curdo"},{"ky","Quirguiz"},{"lo","Laosiano"},{"lv","Letão"},
        {"lt","Lituano"},{"lb","Luxemburguês"},{"mk","Macedônio"},{"mg","Malgaxe"},
        {"ms","Malaio"},{"ml","Malaiala"},{"mt","Maltês"},{"mn","Mongol"},
        {"ne","Nepali"},{"no","Norueguês"},{"nb","Norueguês"},{"nn","Norueguês"},
        {"fa","Persa"},{"pl","Polonês"},{"pt","Português"},{"pa","Panjabi"},
        {"ro","Romeno"},{"ru","Russo"},{"sr","Sérvio"},{"si","Cingalês"},
        {"sk","Eslovaco"},{"sl","Esloveno"},{"so","Somali"},{"es","Espanhol"},
        {"sw","Suaíli"},{"sv","Sueco"},{"tg","Tadjique"},{"ta","Tâmil"},
        {"te","Télugo"},{"th","Tailandês"},{"bo","Tibetano"},{"ti","Tigrínia"},
        {"tr","Turco"},{"tk","Turcomeno"},{"uk","Ucraniano"},{"ur","Urdu"},
        {"uz","Uzbeque"},{"vi","Vietnamita"},{"cy","Galês"},{"yo","Iorubá"},
        {"zu","Zulu"},{"rw","Quiniaruanda"},{"sn","Shona"},{"xh","Xhosa"},
        {"st","Soto"},{"tn","Tswana"},{"wo","Uólofe"},{"ak","Akan"},
        {"lg","Luganda"},{"ny","Chichewa"},{"dz","Dzonga"},{"ps","Pashto"},
    };
    QStringList out;
    const QStringList parts = codes.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i) {
        const QString raw = parts.at(i);
        // Só o idioma principal (1º) e os oficiais regionais (com sufixo de país,
        // ex pt-BR, de-CH). Idiomas soltos (es, en, fr no Brasil) são ruído.
        if (i != 0 && !raw.contains(QLatin1Char('-'))) continue;
        const QString base = raw.section(QLatin1Char('-'), 0, 0).toLower();
        const QString nm = names.value(base);
        if (nm.isEmpty()) continue;          // desconhecido: omite (sem sigla)
        if (!out.contains(nm)) out << nm;
        if (out.size() >= 4) break;
    }
    return out.join(QStringLiteral(", "));
}
}

MapView::MapView(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(220);
    setMouseTracking(true); // hover ao vivo
    setCursor(Qt::OpenHandCursor);
    m_textured = QSettings().value(QStringLiteral("map/textured"), true).toBool();
    m_globe = QSettings().value(QStringLiteral("map/globe"), false).toBool();
    connect(Theme::Manager::instance(), &Theme::Manager::themeChanged,
            this, qOverload<>(&QWidget::update));
}

QPointF MapView::project(double lon, double lat) const
{
    // Equiretangular: x cresce com a longitude; y invertido (lat+ = cima).
    return QPointF(lon * m_scale + m_offset.x(),
                   -lat * m_scale + m_offset.y());
}

QPointF MapView::screenToLonLat(const QPointF& s) const
{
    if (m_scale <= 0.0) return QPointF();
    return QPointF((s.x() - m_offset.x()) / m_scale,
                   (m_offset.y() - s.y()) / m_scale);
}

int MapView::countryAt(const QPointF& ll) const
{
    const QVector<GeoData::Country>& cs = GeoData::instance().countries();
    for (int i = 0; i < cs.size(); ++i) {
        if (!cs.at(i).bounds.contains(ll)) continue;
        for (const QPolygonF& ring : cs.at(i).rings)
            if (ring.containsPoint(ll, Qt::OddEvenFill)) return i;
    }
    return -1;
}

int MapView::stateAt(const QPointF& ll) const
{
    const QVector<GeoData::State>& ss = GeoData::instance().states();
    for (int i = 0; i < ss.size(); ++i) {
        if (!ss.at(i).bounds.contains(ll)) continue;
        for (const QPolygonF& ring : ss.at(i).rings)
            if (ring.containsPoint(ll, Qt::OddEvenFill)) return i;
    }
    return -1;
}

void MapView::leaveEvent(QEvent*)
{
    if (m_hoverCountry >= 0 || m_hoverState >= 0) {
        m_hoverCountry = -1;
        m_hoverState = -1;
        update();
    }
}

void MapView::hitTest(const QPoint& pos)
{
    const GeoData& geo = GeoData::instance();
    const QPointF ll = screenToLonLat(QPointF(pos));
    const double r = (m_fitScale > 0.0) ? m_scale / m_fitScale : 1.0;

    // 1) Cidade próxima do clique (mesma visibilidade do desenho).
    const qint64 thr =
          (r < 2.0)  ? Q_INT64_C(9000000000)
        : (r < 4.5)  ? Q_INT64_C(3000000)
        : (r < 9.0)  ? Q_INT64_C(700000)
        : (r < 16.0) ? Q_INT64_C(150000)
        : (r < 28.0) ? Q_INT64_C(30000)
                     : Q_INT64_C(0);
    // Raio curto: só pega cidade se o clique for quase em cima do ponto.
    // Assim, clicar numa área livre do estado seleciona o estado.
    const GeoData::Place* bestCity = nullptr;
    double bestD = 8.0;
    for (const GeoData::Place& pl : geo.places()) {
        const bool show = pl.population >= thr
            || (pl.isCapital() && r >= 2.0)
            || (pl.isStateCapital() && r >= 4.5);
        if (!show) continue;
        const double d = QLineF(project(pl.lon, pl.lat), QPointF(pos)).length();
        if (d < bestD) { bestD = d; bestCity = &pl; }
    }
    if (bestCity) {
        QString country = bestCity->countryCode;
        for (const GeoData::Country& c : geo.countries())
            if (c.iso == bestCity->countryCode) { country = c.name; break; }
        QString sub = tr("Cidade");
        if (bestCity->isCapital()) sub = tr("Capital nacional");
        else if (bestCity->isStateCapital()) sub = tr("Capital estadual");
        QStringList det;
        if (!bestCity->state.isEmpty()) det << tr("Estado: %1").arg(bestCity->state);
        if (!country.isEmpty()) det << tr("País: %1").arg(country);
        if (bestCity->population > 0) det << tr("População: %1").arg(fmtNum(bestCity->population));
        det << tr("Altitude: %1 m").arg(bestCity->elevation);
        if (!bestCity->timezone.isEmpty()) {
            det << tr("Fuso: %1").arg(bestCity->timezone);
            const QTimeZone tz(bestCity->timezone.toUtf8());
            if (tz.isValid()) {
                const QString hora = QDateTime::currentDateTime().toTimeZone(tz).toString(QStringLiteral("HH:mm"));
                det << tr("Hora local: %1").arg(hora);
            }
        }
        emit featureClicked(bestCity->name, sub, det, bestCity->countryCode);
        return;
    }

    // 2) Estado (quando há zoom de estado).
    if (r >= 4.5) {
        for (const GeoData::State& st : geo.states()) {
            if (!st.bounds.contains(ll)) continue;
            bool inside = false;
            for (const QPolygonF& ring : st.rings)
                if (ring.containsPoint(ll, Qt::OddEvenFill)) { inside = true; break; }
            if (!inside) continue;
            QString iso;
            for (const GeoData::Country& c : geo.countries())
                if (c.nameEn == st.country) { iso = c.iso; break; }
            emit featureClicked(st.name, tr("Estado"), { tr("País: %1").arg(st.country) }, iso);
            return;
        }
    }

    // 3) País (ficha completa).
    const int ci = countryAt(ll);
    if (ci >= 0) emitCountryFicha(ci);
}

void MapView::emitCountryFicha(int idx)
{
    const GeoData& geo = GeoData::instance();
    if (idx < 0 || idx >= geo.countries().size()) return;
    const GeoData::Country& c = geo.countries().at(idx);
    QStringList det;
    if (const GeoData::CountryInfo* ci = geo.countryInfo(c.iso)) {
        if (!ci->capital.isEmpty()) det << tr("Capital: %1").arg(ci->capital);
        if (ci->population > 0) det << tr("População: %1").arg(fmtNum(ci->population));
        if (!ci->languages.isEmpty()) det << tr("Idiomas: %1").arg(languageNames(ci->languages));
        if (!ci->currency.isEmpty()) det << tr("Moeda: %1").arg(ci->currency);
        if (!ci->continent.isEmpty()) det << tr("Continente: %1").arg(continentName(ci->continent));
        if (ci->area > 0) det << tr("Área: %1 km²").arg(fmtNum(static_cast<qint64>(ci->area)));
    }
    emit featureClicked(c.name, tr("País"), det, c.iso);
}

void MapView::fitToWidget()
{
    if (width() <= 0 || height() <= 0) return;
    m_fitScale = width() / 360.0;       // mundo (360°) cabe na largura
    m_scale = m_fitScale;
    m_offset = QPointF(width() / 2.0, height() / 2.0); // lon0/lat0 no centro
    m_fitted = true;
}

void MapView::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!m_fitted) fitToWidget();
}

void MapView::flyToBounds(const QRectF& b)
{
    if (!m_fitted) fitToWidget();
    if (b.isNull() || b.width() <= 0.0 || b.height() <= 0.0) return;
    const double sx = width()  / b.width();
    const double sy = height() / b.height();
    double sc = std::min(sx, sy) * 0.75; // folga nas bordas
    sc = std::clamp(sc, m_fitScale, m_fitScale * 40.0);
    m_scale = sc;
    const QPointF c = b.center();
    m_offset = QPointF(width() / 2.0 - c.x() * m_scale,
                       height() / 2.0 + c.y() * m_scale);
    update();
}

void MapView::flyToPoint(double lon, double lat)
{
    if (!m_fitted) fitToWidget();
    m_scale = std::clamp(m_fitScale * 12.0, m_fitScale, m_fitScale * 40.0);
    m_offset = QPointF(width() / 2.0 - lon * m_scale,
                       height() / 2.0 + lat * m_scale);
    update();
}

void MapView::setRulerMode(bool on)
{
    m_ruler = on;
    m_rulerCount = 0;
    if (on) m_addPin = false;
    setCursor(on ? Qt::CrossCursor : Qt::OpenHandCursor);
    update();
}

void MapView::setPinsStore(MapPinsStore* s)
{
    m_pins = s;
    if (m_pins)
        connect(m_pins, &MapPinsStore::pinsChanged, this, qOverload<>(&QWidget::update));
    update();
}

void MapView::setAddPinMode(bool on)
{
    m_addPin = on;
    if (on) m_ruler = false;
    setCursor(on ? Qt::CrossCursor : Qt::OpenHandCursor);
    update();
}

void MapView::setTextured(bool on)
{
    m_textured = on;
    QSettings().setValue(QStringLiteral("map/textured"), on);
    update();
}

void MapView::setGlobeMode(bool on)
{
    m_globe = on;
    QSettings().setValue(QStringLiteral("map/globe"), on);
    setCursor(Qt::OpenHandCursor);
    update();
}

double MapView::globeRadius() const
{
    return qMin(width(), height()) * 0.42 * m_globeZoom;
}

QPointF MapView::projectGlobe(double lon, double lat, bool* visible) const
{
    const double D = kPi / 180.0;
    const double R = globeRadius();
    const double la = lat * D, la0 = m_globeLat0 * D, lo = (lon - m_globeLon0) * D;
    const double cosc = std::sin(la0) * std::sin(la)
                      + std::cos(la0) * std::cos(la) * std::cos(lo);
    if (visible) *visible = (cosc >= 0.0);
    const double x = R * std::cos(la) * std::sin(lo);
    const double y = R * (std::cos(la0) * std::sin(la) - std::sin(la0) * std::cos(la) * std::cos(lo));
    return QPointF(width() / 2.0 + x, height() / 2.0 - y);
}

QPointF MapView::invGlobe(const QPointF& screen, bool* ok) const
{
    const double D = kPi / 180.0;
    const double R = globeRadius();
    const double x = screen.x() - width() / 2.0;
    const double y = -(screen.y() - height() / 2.0);
    const double rho = std::sqrt(x * x + y * y);
    if (rho > R) { if (ok) *ok = false; return QPointF(); }
    if (ok) *ok = true;
    if (rho < 1e-6) return QPointF(m_globeLon0, m_globeLat0);
    const double cc = std::asin(std::clamp(rho / R, -1.0, 1.0));
    const double la0 = m_globeLat0 * D;
    const double lat = std::asin(std::clamp(
        std::cos(cc) * std::sin(la0) + y * std::sin(cc) * std::cos(la0) / rho, -1.0, 1.0)) / D;
    const double lon = m_globeLon0 + std::atan2(
        x * std::sin(cc), rho * std::cos(la0) * std::cos(cc) - y * std::sin(la0) * std::sin(cc)) / D;
    return QPointF(lon, lat);
}

void MapView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!m_fitted) fitToWidget();
}

void MapView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (m_globe) { paintGlobe(p); return; }

    const bool tex = m_textured;
    const QColor appBg(Theme::appBackground());
    const QColor textP(Theme::textPrimary());
    // Texturizado: esquema físico próprio (independe do tema). Simples: tema.
    const QColor water      = tex ? QColor(0x9f, 0xbd, 0xd0) : appBg;
    const QColor stroke     = tex ? QColor(0x3c, 0x3c, 0x3c) : mix(appBg, textP, 0.92);
    const QColor capitalDot = tex ? QColor(0xb0, 0x3a, 0x2e) : QColor(Theme::accentDefault());
    const QColor cityDot    = tex ? QColor(0x55, 0x55, 0x55) : QColor(Theme::textMuted());
    const QColor labelColor = tex ? QColor(0x2b, 0x2b, 0x2b) : textP;
    const QColor bigLabel   = tex ? QColor(0x16, 0x16, 0x16) : QColor(Theme::textBright());
    const QColor haloColor  = tex ? QColor(0xff, 0xff, 0xff) : water;

    p.fillRect(rect(), water);

    const QColor stateStroke = tex ? QColor(0x5a, 0x5a, 0x5a) : mix(appBg, textP, 0.74);
    const GeoData& geo = GeoData::instance();

    // Desenha texto com um leve contorno (halo) pra os nomes se destacarem do
    // fundo em vez de se perderem na terra.
    auto haloText = [&](const QPointF& at, const QString& t, const QColor& fg) {
        p.setPen(haloColor);
        p.drawText(at + QPointF(1, 0), t);
        p.drawText(at + QPointF(-1, 0), t);
        p.drawText(at + QPointF(0, 1), t);
        p.drawText(at + QPointF(0, -1), t);
        p.setPen(fg);
        p.drawText(at, t);
    };
    const QRect view = rect().adjusted(-40, -40, 40, 40);

    // Nível de detalhe pela razão de zoom em relação ao enquadramento.
    const double r = (m_fitScale > 0.0) ? m_scale / m_fitScale : 1.0;
    const bool lvlContinents = r < 2.0;
    const bool lvlCountries  = (r >= 2.0 && r < 4.5);
    const bool lvlStates     = (r >= 4.5 && r < 9.0);
    const bool lvlCities     = r >= 9.0;

    auto boundsOnScreen = [&](const QRectF& geo) {
        const QPointF a = project(geo.left(), geo.top());
        const QPointF b = project(geo.right(), geo.bottom());
        return QRectF(a, b).normalized();
    };

    // Terra colorida por faixa climática (gradiente latitudinal) + fronteiras.
    // A projeção é linear na latitude, então um gradiente vertical em coords de
    // tela já corresponde às faixas de clima — países grandes mostram a variação.
    {
        QBrush landBrush;
        if (tex) {
            const double yTop = project(0.0, 90.0).y();
            const double yBot = project(0.0, -90.0).y();
            QLinearGradient climate(QPointF(0, yTop), QPointF(0, yBot));
            auto fr = [](double lat) { return (90.0 - lat) / 180.0; };
            const QColor polar(0xc4, 0xcc, 0xd1);     // gelo
            const QColor boreal(0x8f, 0x9e, 0x8a);    // subpolar
            const QColor temperate(0x86, 0xa0, 0x64); // temperado
            const QColor arid(0xc2, 0xa2, 0x63);      // subtropical seco
            const QColor tropic(0x6e, 0x9a, 0x54);    // tropical
            const QColor equator(0x5e, 0x8c, 0x4e);   // equatorial
            climate.setColorAt(fr(90),   polar);
            climate.setColorAt(fr(60),   boreal);
            climate.setColorAt(fr(40),   temperate);
            climate.setColorAt(fr(25),   arid);
            climate.setColorAt(fr(12),   tropic);
            climate.setColorAt(fr(0),    equator);
            climate.setColorAt(fr(-12),  tropic);
            climate.setColorAt(fr(-25),  arid);
            climate.setColorAt(fr(-40),  temperate);
            climate.setColorAt(fr(-60),  boreal);
            climate.setColorAt(fr(-90),  polar);
            landBrush = QBrush(climate);
        } else {
            landBrush = mix(appBg, textP, 0.26); // terra única do tema
        }
        p.setBrush(landBrush);
        p.setPen(QPen(stroke, 1.4));
        for (const GeoData::Country& c : geo.countries()) {
            for (const QPolygonF& ring : c.rings) {
                QPolygonF screen;
                screen.reserve(ring.size());
                for (const QPointF& pt : ring)
                    screen << project(pt.x(), pt.y());
                p.drawPolygon(screen);
            }
        }
    }

    // Divisões internas (estados): contorno TRACEJADO — destaca-se da borda do
    // país e fica nítido. Aparecem já num zoom moderado.
    if (r >= 3.0) {
        p.setBrush(Qt::NoBrush);
        QPen statePen(stateStroke, 1.3, Qt::DashLine);
        statePen.setDashPattern({3.0, 2.5}); // tracejado denso, bem visível
        p.setPen(statePen);
        for (const GeoData::State& st : geo.states()) {
            if (!boundsOnScreen(st.bounds).intersects(view)) continue;
            for (const QPolygonF& ring : st.rings) {
                QPolygonF screen;
                screen.reserve(ring.size());
                for (const QPointF& pt : ring)
                    screen << project(pt.x(), pt.y());
                p.drawPolygon(screen);
            }
        }
    }

    // Destaque do hover (estado em zoom, senão país sob o cursor).
    {
        auto drawHi = [&](const QVector<QPolygonF>& rings) {
            p.setBrush(QColor(255, 255, 255, 48));
            p.setPen(QPen(QColor(Theme::accentDefault()), 2.0));
            for (const QPolygonF& ring : rings) {
                QPolygonF sc;
                sc.reserve(ring.size());
                for (const QPointF& pt : ring) sc << project(pt.x(), pt.y());
                p.drawPolygon(sc);
            }
        };
        if (m_hoverState >= 0 && m_hoverState < geo.states().size())
            drawHi(geo.states().at(m_hoverState).rings);
        else if (m_hoverCountry >= 0 && m_hoverCountry < geo.countries().size())
            drawHi(geo.countries().at(m_hoverCountry).rings);
    }

    // Cidades/municípios: surgem por população conforme o zoom (capitais têm
    // prioridade). Quanto mais perto, menor a cidade que aparece.
    {
        const qint64 thr =
              (r < 2.0)  ? Q_INT64_C(9000000000)
            : (r < 4.5)  ? Q_INT64_C(3000000)
            : (r < 9.0)  ? Q_INT64_C(700000)
            : (r < 16.0) ? Q_INT64_C(150000)
            : (r < 28.0) ? Q_INT64_C(30000)
                         : Q_INT64_C(0);
        const bool cityLabels = r >= 6.0;

        // Coleta as visíveis (filtro + culling).
        struct Vis { QPointF s; const GeoData::Place* p; };
        QVector<Vis> vis;
        for (const GeoData::Place& place : geo.places()) {
            const bool show = place.population >= thr
                || (place.isCapital() && r >= 2.0)
                || (place.isStateCapital() && r >= 4.5);
            if (!show) continue;
            const QPointF s = project(place.lon, place.lat);
            if (!view.contains(s.toPoint())) continue;
            vis.append({ s, &place });
        }

        // Pontos de todas as visíveis.
        for (const Vis& v : vis) {
            p.setPen(Qt::NoPen);
            if (v.p->isCapital()) {
                p.setBrush(capitalDot);
                p.drawEllipse(v.s, 3.5, 3.5);
            } else if (v.p->isStateCapital()) {
                p.setBrush(bigLabel);
                p.drawEllipse(v.s, 2.8, 2.8);
            } else {
                p.setBrush(cityDot);
                p.drawEllipse(v.s, 2.0, 2.0);
            }
        }

        // Rótulos com anti-sobreposição: prioriza capitais e maiores; só escreve
        // o nome se não colidir com um já escrito.
        if (cityLabels) {
            auto importance = [](const GeoData::Place* pl) -> qint64 {
                qint64 k = pl->population;
                if (pl->isCapital())           k += Q_INT64_C(1000000000000);
                else if (pl->isStateCapital()) k += Q_INT64_C(1000000000);
                return k;
            };
            std::sort(vis.begin(), vis.end(), [&](const Vis& a, const Vis& b) {
                return importance(a.p) > importance(b.p);
            });
            QFont cf = font();
            cf.setPointSize(8);
            QFont cfBold = cf;
            cfBold.setPointSize(9);
            cfBold.setBold(true);
            p.setFont(cf);
            const QFontMetrics fm(cf);
            QVector<QRectF> occupied;
            occupied.reserve(256);
            for (const Vis& v : vis) {
                const QString& nm = v.p->name;
                const QRectF lr(v.s.x() + 6, v.s.y() - fm.height() + 4,
                                fm.horizontalAdvance(nm) + 5, fm.height());
                bool collide = false;
                for (const QRectF& o : occupied)
                    if (o.intersects(lr)) { collide = true; break; }
                if (collide) continue;
                occupied.append(lr);
                p.setFont(v.p->isCapital() ? cfBold : cf);
                haloText(v.s + QPointF(6, 4), nm, v.p->isCapital() ? bigLabel : labelColor);
            }
        }
    }

    // Rótulos de região (continente / país / estado).
    QFont f = font();
    if (lvlContinents) {
        f.setPointSize(13);
        f.setBold(true);
        p.setFont(f);
        const QFontMetrics fm(f);
        for (const Continent& c : kContinents) {
            const QPointF s = project(c.lon, c.lat);
            if (!view.contains(s.toPoint())) continue;
            const QString name = QString::fromUtf8(c.name);
            haloText(s - QPointF(fm.horizontalAdvance(name) / 2.0, 0), name, bigLabel);
        }
    } else if (lvlCountries) {
        f.setPointSize(9);
        f.setBold(true);
        p.setFont(f);
        const QFontMetrics fm(f);
        // Ponto do rótulo: LABEL_X/Y do Natural Earth (sempre dentro do país,
        // trata o antimeridiano); fallback no centro do bounding box.
        auto labelPos = [](const GeoData::Country& c) {
            if (c.labelLon != 0.0 || c.labelLat != 0.0)
                return QPointF(c.labelLon, c.labelLat);
            return c.bounds.center();
        };
        // Maiores primeiro têm prioridade na anti-sobreposição.
        QVector<const GeoData::Country*> cands;
        for (const GeoData::Country& c : geo.countries()) {
            if (!view.contains(project(labelPos(c).x(), labelPos(c).y()).toPoint())) continue;
            cands.append(&c);
        }
        std::sort(cands.begin(), cands.end(), [](const GeoData::Country* a, const GeoData::Country* b) {
            return a->bounds.width() * a->bounds.height() > b->bounds.width() * b->bounds.height();
        });
        QVector<QRectF> occ;
        for (const GeoData::Country* c : cands) {
            const QPointF s = project(labelPos(*c).x(), labelPos(*c).y());
            const QPointF at = s - QPointF(fm.horizontalAdvance(c->name) / 2.0, 0);
            const QRectF lr(at.x(), at.y() - fm.height() + 4,
                            fm.horizontalAdvance(c->name) + 4, fm.height());
            bool col = false;
            for (const QRectF& o : occ) if (o.intersects(lr)) { col = true; break; }
            if (col) continue;
            occ.append(lr);
            haloText(at, c->name, bigLabel);
        }
    } else if (lvlStates) {
        f.setPointSize(8);
        f.setBold(true);
        p.setFont(f);
        const QFontMetrics fm(f);
        for (const GeoData::State& st : geo.states()) {
            if (st.bounds.width() * m_scale < 45.0) continue;
            const QPointF s = project(st.bounds.center().x(), st.bounds.center().y());
            if (!view.contains(s.toPoint())) continue;
            haloText(s - QPointF(fm.horizontalAdvance(st.name) / 2.0, 0), st.name, labelColor);
        }
    }

    // Régua: marcadores e linha entre os dois pontos.
    if (m_ruler && m_rulerCount >= 1) {
        const QColor rc(Theme::accentDefault());
        const QPointF a = project(m_rulerA.x(), m_rulerA.y());
        p.setPen(Qt::NoPen);
        p.setBrush(rc);
        p.drawEllipse(a, 4.0, 4.0);
        if (m_rulerCount == 2) {
            const QPointF b = project(m_rulerB.x(), m_rulerB.y());
            p.drawEllipse(b, 4.0, 4.0);
            QPen lp(rc, 1.6, Qt::DashLine);
            p.setPen(lp);
            p.setBrush(Qt::NoBrush);
            p.drawLine(a, b);
        }
    }

    // Pins de referência do projeto (sempre por cima).
    if (m_pins) {
        QFont pf = font();
        pf.setPointSize(9);
        pf.setBold(true);
        p.setFont(pf);
        const QColor pinColor(Theme::accentDefault());
        for (const MapPinsStore::Pin& pin : m_pins->pins()) {
            const QPointF s = project(pin.lon, pin.lat);
            if (!view.contains(s.toPoint())) continue;
            // Gota: círculo com um vértice apontando pra baixo.
            p.setPen(QPen(water, 1.0));
            p.setBrush(pinColor);
            p.drawEllipse(s + QPointF(0, -6), 6.0, 6.0);
            QPolygonF tip;
            tip << s + QPointF(-3.5, -3) << s + QPointF(3.5, -3) << s;
            p.drawPolygon(tip);
            p.setPen(Qt::NoPen);
            p.setBrush(water);
            p.drawEllipse(s + QPointF(0, -6), 2.0, 2.0);
            if (!pin.label.isEmpty())
                haloText(s + QPointF(9, -4), pin.label, bigLabel);
        }
    }

    // Tooltip do hover (nome do que está sob o cursor).
    if (m_hoverState >= 0 || m_hoverCountry >= 0) {
        QString name;
        if (m_hoverState >= 0 && m_hoverState < geo.states().size())
            name = geo.states().at(m_hoverState).name;
        else if (m_hoverCountry >= 0 && m_hoverCountry < geo.countries().size())
            name = geo.countries().at(m_hoverCountry).name;
        if (!name.isEmpty()) {
            QFont tf = font();
            tf.setPointSize(9);
            tf.setBold(true);
            p.setFont(tf);
            const QFontMetrics fm(tf);
            const qreal tw = fm.horizontalAdvance(name) + 14;
            const qreal th = fm.height() + 6;
            QRectF box(m_hoverPos.x() + 14, m_hoverPos.y() - th - 6, tw, th);
            if (box.right() > width() - 2) box.moveLeft(m_hoverPos.x() - tw - 8);
            if (box.top() < 2) box.moveTop(m_hoverPos.y() + 10);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(18, 18, 20, 215));
            p.drawRoundedRect(box, 5, 5);
            p.setPen(QColor(0xff, 0xff, 0xff));
            p.drawText(box, Qt::AlignCenter, name);
        }
    }
}

void MapView::paintGlobe(QPainter& p)
{
    const bool tex = m_textured;
    const QColor appBg(Theme::appBackground());
    const QColor textP(Theme::textPrimary());
    const QColor space  = tex ? QColor(0x0a, 0x0f, 0x18) : mix(appBg, QColor(0, 0, 0), 0.35);
    const QColor ocean  = tex ? QColor(0x9f, 0xbd, 0xd0) : appBg;
    const QColor land   = tex ? QColor(0x6e, 0x9a, 0x54) : mix(appBg, textP, 0.26);
    const QColor stroke = tex ? QColor(0x3c, 0x3c, 0x3c) : mix(appBg, textP, 0.92);
    const QColor capitalDot = tex ? QColor(0xb0, 0x3a, 0x2e) : QColor(Theme::accentDefault());
    const QColor labelColor = tex ? QColor(0x16, 0x16, 0x16) : QColor(Theme::textBright());
    const QColor haloColor  = tex ? QColor(0xff, 0xff, 0xff) : ocean;
    const QColor pinColor(Theme::accentDefault());

    p.fillRect(rect(), space);

    const QPointF cen(width() / 2.0, height() / 2.0);
    const double R = globeRadius();

    p.setPen(QPen(mix(ocean, QColor(0, 0, 0), 0.4), 1.5));
    p.setBrush(ocean);
    p.drawEllipse(cen, R, R);

    const GeoData& geo = GeoData::instance();

    // Projeta um anel pro globo, grampeando vértices "de trás" na borda do disco.
    auto ringToPoly = [&](const QPolygonF& ring) {
        QPolygonF poly;
        poly.reserve(ring.size());
        for (const QPointF& pt : ring) {
            bool vis;
            QPointF s = projectGlobe(pt.x(), pt.y(), &vis);
            if (!vis) {
                QPointF d = s - cen;
                const double l = std::hypot(d.x(), d.y());
                if (l > 0.0) s = cen + d * (R / l);
            }
            poly << s;
        }
        return poly;
    };
    auto haloText = [&](const QPointF& at, const QString& t, const QColor& fg) {
        p.setPen(haloColor);
        p.drawText(at + QPointF(1, 0), t);  p.drawText(at + QPointF(-1, 0), t);
        p.drawText(at + QPointF(0, 1), t);  p.drawText(at + QPointF(0, -1), t);
        p.setPen(fg); p.drawText(at, t);
    };

    QPainterPath clip;
    clip.addEllipse(cen, R, R);
    p.setClipPath(clip);
    p.setBrush(land);
    p.setPen(QPen(stroke, 1.0));
    for (const GeoData::Country& c : geo.countries())
        for (const QPolygonF& ring : c.rings)
            p.drawPolygon(ringToPoly(ring));

    if (m_hoverCountry >= 0 && m_hoverCountry < geo.countries().size()) {
        p.setBrush(QColor(255, 255, 255, 48));
        p.setPen(QPen(pinColor, 2.0));
        for (const QPolygonF& ring : geo.countries().at(m_hoverCountry).rings)
            p.drawPolygon(ringToPoly(ring));
    }
    p.setClipping(false);

    for (const GeoData::Place& place : geo.places()) {
        if (!place.isCapital()) continue;
        bool vis;
        const QPointF s = projectGlobe(place.lon, place.lat, &vis);
        if (!vis) continue;
        p.setPen(Qt::NoPen);
        p.setBrush(capitalDot);
        p.drawEllipse(s, 3.0, 3.0);
    }

    // Rótulos de país (cull pelo horizonte + anti-sobreposição).
    QFont lf = font();
    lf.setPointSize(9);
    lf.setBold(true);
    p.setFont(lf);
    const QFontMetrics fm(lf);
    QVector<const GeoData::Country*> cands;
    for (const GeoData::Country& c : geo.countries()) cands.append(&c);
    std::sort(cands.begin(), cands.end(), [](const GeoData::Country* a, const GeoData::Country* b) {
        return a->bounds.width() * a->bounds.height() > b->bounds.width() * b->bounds.height();
    });
    QVector<QRectF> occ;
    for (const GeoData::Country* c : cands) {
        const double lon = (c->labelLon != 0.0 || c->labelLat != 0.0) ? c->labelLon : c->bounds.center().x();
        const double lat = (c->labelLon != 0.0 || c->labelLat != 0.0) ? c->labelLat : c->bounds.center().y();
        bool vis;
        const QPointF s = projectGlobe(lon, lat, &vis);
        if (!vis) continue;
        const QPointF at = s - QPointF(fm.horizontalAdvance(c->name) / 2.0, 0);
        const QRectF lr(at.x(), at.y() - fm.height() + 4, fm.horizontalAdvance(c->name) + 4, fm.height());
        bool col = false;
        for (const QRectF& o : occ) if (o.intersects(lr)) { col = true; break; }
        if (col) continue;
        occ.append(lr);
        haloText(at, c->name, labelColor);
    }

    if (m_pins) {
        QFont pf = font();
        pf.setPointSize(9);
        pf.setBold(true);
        p.setFont(pf);
        for (const MapPinsStore::Pin& pin : m_pins->pins()) {
            bool vis;
            const QPointF s = projectGlobe(pin.lon, pin.lat, &vis);
            if (!vis) continue;
            p.setPen(QPen(haloColor, 1.0));
            p.setBrush(pinColor);
            p.drawEllipse(s + QPointF(0, -6), 6, 6);
            QPolygonF tip;
            tip << s + QPointF(-3.5, -3) << s + QPointF(3.5, -3) << s;
            p.drawPolygon(tip);
            p.setPen(Qt::NoPen);
            p.setBrush(haloColor);
            p.drawEllipse(s + QPointF(0, -6), 2, 2);
            if (!pin.label.isEmpty()) haloText(s + QPointF(9, -4), pin.label, labelColor);
        }
    }

    if (m_hoverCountry >= 0 && m_hoverCountry < geo.countries().size()) {
        const QString name = geo.countries().at(m_hoverCountry).name;
        QFont tf = font();
        tf.setPointSize(9);
        tf.setBold(true);
        p.setFont(tf);
        const QFontMetrics tfm(tf);
        const qreal tw = tfm.horizontalAdvance(name) + 14, th = tfm.height() + 6;
        QRectF box(m_hoverPos.x() + 14, m_hoverPos.y() - th - 6, tw, th);
        if (box.right() > width() - 2) box.moveLeft(m_hoverPos.x() - tw - 8);
        if (box.top() < 2) box.moveTop(m_hoverPos.y() + 10);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(18, 18, 20, 215));
        p.drawRoundedRect(box, 5, 5);
        p.setPen(QColor(0xff, 0xff, 0xff));
        p.drawText(box, Qt::AlignCenter, name);
    }
}

void MapView::wheelEvent(QWheelEvent* event)
{
    if (m_globe) {
        const double f = (event->angleDelta().y() > 0) ? 1.12 : 1.0 / 1.12;
        m_globeZoom = std::clamp(m_globeZoom * f, 0.6, 6.0);
        update();
        return;
    }

    const double factor = (event->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
    const QPointF cursor = event->position();
    const QPointF worldUnder = (cursor - m_offset) / m_scale;

    double newScale = m_scale * factor;
    // Mínimo = enquadramento (sem zoom out exagerado); máximo = ~40x.
    newScale = std::clamp(newScale, m_fitScale, m_fitScale * 40.0);
    m_offset = cursor - worldUnder * newScale;
    m_scale = newScale;
    update();
}

void MapView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastMouse = event->pos();
        m_pressPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void MapView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        if (m_globe) {
            const double k = 0.25 / m_globeZoom;
            m_globeLon0 -= (event->pos().x() - m_lastMouse.x()) * k;
            m_globeLat0 = std::clamp(m_globeLat0 + (event->pos().y() - m_lastMouse.y()) * k, -89.0, 89.0);
            m_lastMouse = event->pos();
            update();
        } else {
            m_offset += QPointF(event->pos() - m_lastMouse);
            m_lastMouse = event->pos();
            update();
        }
        return;
    }

    // Hover no globo: país sob o cursor (via projeção inversa).
    if (m_globe) {
        bool ok;
        const QPointF gl = invGlobe(QPointF(event->pos()), &ok);
        const int hc = ok ? countryAt(gl) : -1;
        const bool had = (m_hoverCountry >= 0);
        m_hoverCountry = hc;
        m_hoverState = -1;
        m_hoverPos = event->pos();
        if (had || hc >= 0) update();
        return;
    }

    // Hover ao vivo: destaca o estado (em zoom) ou o país sob o cursor.
    const QPointF ll = screenToLonLat(QPointF(event->pos()));
    const double r = (m_fitScale > 0.0) ? m_scale / m_fitScale : 1.0;
    const int hs = (r >= 4.5) ? stateAt(ll) : -1;
    const int hc = (hs < 0) ? countryAt(ll) : -1;
    const bool had = (m_hoverState >= 0 || m_hoverCountry >= 0);
    m_hoverState = hs;
    m_hoverCountry = hc;
    m_hoverPos = event->pos();
    if (had || hs >= 0 || hc >= 0) update();
}

void MapView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor((m_ruler || m_addPin) ? Qt::CrossCursor : Qt::OpenHandCursor);
        if ((event->pos() - m_pressPos).manhattanLength() >= 5) return; // foi arrasto

        // No globo: pin existente > fixar pin > país.
        if (m_globe) {
            bool ok;
            const QPointF gl = invGlobe(QPointF(event->pos()), &ok);
            if (!ok) return;
            if (m_addPin) { emit pinRequestedAt(gl.x(), gl.y()); return; }
            if (m_pins) {
                const MapPinsStore::Pin* best = nullptr;
                double bd = 12.0;
                for (const MapPinsStore::Pin& pin : m_pins->pins()) {
                    bool vis;
                    const QPointF s = projectGlobe(pin.lon, pin.lat, &vis);
                    if (!vis) continue;
                    const double d = QLineF(s, QPointF(event->pos())).length();
                    if (d < bd) { bd = d; best = &pin; }
                }
                if (best) { emit pinClicked(best->id); return; }
            }
            const int ci = countryAt(gl);
            if (ci >= 0) emitCountryFicha(ci);
            return;
        }

        // Modo "fixar pin": clique cria um pin no ponto.
        if (m_addPin) {
            const QPointF p = screenToLonLat(QPointF(event->pos()));
            emit pinRequestedAt(p.x(), p.y());
            return;
        }

        if (!m_ruler) {
            // Pin existente tem prioridade sobre a feição geográfica.
            if (m_pins) {
                const MapPinsStore::Pin* best = nullptr;
                double bd = 12.0;
                for (const MapPinsStore::Pin& pin : m_pins->pins()) {
                    const double d = QLineF(project(pin.lon, pin.lat),
                                            QPointF(event->pos())).length();
                    if (d < bd) { bd = d; best = &pin; }
                }
                if (best) { emit pinClicked(best->id); return; }
            }
            hitTest(event->pos()); // clique normal: identifica a feição
            return;
        }

        // Modo régua: marca A, depois B, e mede.
        const QPointF ll = screenToLonLat(QPointF(event->pos()));
        if (m_rulerCount == 0 || m_rulerCount == 2) {
            m_rulerA = ll;
            m_rulerCount = 1;
            update();
        } else {
            m_rulerB = ll;
            m_rulerCount = 2;
            const double km = haversineKm(m_rulerA.y(), m_rulerA.x(), m_rulerB.y(), m_rulerB.x());
            const double dh = (utcOffsetSec(m_rulerB.x(), m_rulerB.y())
                             - utcOffsetSec(m_rulerA.x(), m_rulerA.y())) / 3600.0;
            QStringList det;
            det << tr("Distância: %1 km").arg(fmtNum(qRound64(km)));
            if (qAbs(dh) < 0.01)
                det << tr("Mesmo fuso horário");
            else
                det << tr("Diferença de fuso: %1%2 h")
                        .arg(dh > 0 ? QStringLiteral("+") : QString())
                        .arg(QString::number(dh, 'g', 3));
            emit featureClicked(tr("Medição"), tr("Régua"), det, QString());
            update();
        }
    }
}
