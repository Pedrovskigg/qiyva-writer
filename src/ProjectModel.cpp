#include "ProjectModel.h"
#include "SceneUtils.h"

#include <QJsonValue>
#include <QJsonDocument>
#include <QRandomGenerator>

namespace {

QString jsonString(const QJsonValue& v) {
    return v.isString() ? v.toString() : QString();
}

QString jsonStringOrEmpty(const QJsonValue& v) {
    if (v.isString()) return v.toString();
    return QString();
}

QJsonValue stringOrNull(const QString& s) {
    return s.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(s);
}

QJsonObject variationToJson(const Variation& v) {
    QJsonObject o;
    o.insert(QStringLiteral("id"), v.id);
    o.insert(QStringLiteral("label"), v.label);
    o.insert(QStringLiteral("isPrimary"), v.isPrimary);
    return o;
}

Variation variationFromJson(const QJsonObject& o) {
    Variation v;
    v.id = jsonString(o.value(QStringLiteral("id")));
    v.label = jsonString(o.value(QStringLiteral("label")));
    v.isPrimary = o.value(QStringLiteral("isPrimary")).toBool(false);
    return v;
}

QJsonObject sceneToJson(const Scene& s) {
    QJsonObject o;
    o.insert(QStringLiteral("id"), s.id);
    o.insert(QStringLiteral("title"), s.title);
    o.insert(QStringLiteral("order"), s.order);
    if (!s.variations.isEmpty()) {
        QJsonArray arr;
        for (const auto& v : s.variations) arr.append(variationToJson(v));
        o.insert(QStringLiteral("variations"), arr);
    }
    if (!s.activeVariationId.isEmpty()) {
        o.insert(QStringLiteral("activeVariationId"), s.activeVariationId);
    }
    return o;
}

Scene sceneFromJson(const QJsonObject& o) {
    Scene s;
    s.id = jsonString(o.value(QStringLiteral("id")));
    s.title = jsonString(o.value(QStringLiteral("title")));
    s.order = o.value(QStringLiteral("order")).toInt(0);
    const QJsonArray vars = o.value(QStringLiteral("variations")).toArray();
    for (const auto& vv : vars) s.variations.append(variationFromJson(vv.toObject()));
    s.activeVariationId = jsonString(o.value(QStringLiteral("activeVariationId")));
    return s;
}

QJsonObject chapterToJson(const Chapter& c, int fallbackOrder) {
    QJsonObject o;
    o.insert(QStringLiteral("id"), c.id);
    o.insert(QStringLiteral("manuscriptId"), stringOrNull(c.manuscriptId));
    o.insert(QStringLiteral("title"), c.title);
    o.insert(QStringLiteral("file"), c.file);
    o.insert(QStringLiteral("order"), c.order > 0 ? c.order : fallbackOrder);
    if (c.scenes.size() > 1) {
        QJsonArray arr;
        for (const auto& s : c.scenes) arr.append(sceneToJson(s));
        o.insert(QStringLiteral("scenes"), arr);
    }
    return o;
}

Chapter chapterFromJson(const QJsonObject& o) {
    Chapter c;
    c.id = jsonString(o.value(QStringLiteral("id")));
    c.manuscriptId = jsonStringOrEmpty(o.value(QStringLiteral("manuscriptId")));
    c.title = jsonString(o.value(QStringLiteral("title")));
    c.file = jsonString(o.value(QStringLiteral("file")));
    c.order = o.value(QStringLiteral("order")).toInt(0);
    const QJsonArray scenes = o.value(QStringLiteral("scenes")).toArray();
    for (const auto& sv : scenes) c.scenes.append(sceneFromJson(sv.toObject()));
    return c;
}

QJsonObject manuscriptToJson(const Manuscript& m) {
    QJsonObject o;
    o.insert(QStringLiteral("id"), m.id);
    o.insert(QStringLiteral("title"), m.title);
    o.insert(QStringLiteral("html"), m.html);
    return o;
}

Manuscript manuscriptFromJson(const QJsonObject& o) {
    Manuscript m;
    m.id = jsonString(o.value(QStringLiteral("id")));
    m.title = jsonString(o.value(QStringLiteral("title")));
    m.html = jsonString(o.value(QStringLiteral("html")));
    return m;
}

QJsonObject folderToJson(const Folder& f) {
    QJsonObject o;
    o.insert(QStringLiteral("id"), f.id);
    o.insert(QStringLiteral("title"), f.title);
    o.insert(QStringLiteral("parentId"), stringOrNull(f.parentId));
    o.insert(QStringLiteral("markerId"), stringOrNull(f.markerId));
    return o;
}

Folder folderFromJson(const QJsonObject& o) {
    Folder f;
    f.id = jsonString(o.value(QStringLiteral("id")));
    f.title = jsonString(o.value(QStringLiteral("title")));
    f.parentId = jsonStringOrEmpty(o.value(QStringLiteral("parentId")));
    f.markerId = jsonStringOrEmpty(o.value(QStringLiteral("markerId")));
    return f;
}

QJsonObject drawerItemToJson(const DrawerItem& it) {
    QJsonObject o;
    o.insert(QStringLiteral("id"), it.id);
    o.insert(QStringLiteral("title"), it.title);
    o.insert(QStringLiteral("folderId"), stringOrNull(it.folderId));
    o.insert(QStringLiteral("markerId"), stringOrNull(it.markerId));
    o.insert(QStringLiteral("elementId"), stringOrNull(it.elementId));
    o.insert(QStringLiteral("elementType"), stringOrNull(it.elementType));
    o.insert(QStringLiteral("elementIcon"), stringOrNull(it.elementIcon));
    if (!it.file.isEmpty()) {
        o.insert(QStringLiteral("file"), it.file);
    } else {
        o.insert(QStringLiteral("html"), it.hasInlineHtml ? it.html : QStringLiteral("<p></p>"));
    }
    o.insert(QStringLiteral("isChapter"), it.isChapter);
    o.insert(QStringLiteral("chapterRef"), stringOrNull(it.chapterRef));
    o.insert(QStringLiteral("role"), stringOrNull(it.role));
    return o;
}

DrawerItem drawerItemFromJson(const QJsonObject& o) {
    DrawerItem it;
    it.id = jsonString(o.value(QStringLiteral("id")));
    it.title = jsonString(o.value(QStringLiteral("title")));
    it.folderId = jsonStringOrEmpty(o.value(QStringLiteral("folderId")));
    it.markerId = jsonStringOrEmpty(o.value(QStringLiteral("markerId")));
    it.elementId = jsonStringOrEmpty(o.value(QStringLiteral("elementId")));
    it.elementType = jsonStringOrEmpty(o.value(QStringLiteral("elementType")));
    it.elementIcon = jsonStringOrEmpty(o.value(QStringLiteral("elementIcon")));
    it.file = jsonStringOrEmpty(o.value(QStringLiteral("file")));
    if (o.contains(QStringLiteral("html"))) {
        it.html = jsonStringOrEmpty(o.value(QStringLiteral("html")));
        it.hasInlineHtml = true;
    }
    it.isChapter = o.value(QStringLiteral("isChapter")).toBool(false);
    it.chapterRef = jsonStringOrEmpty(o.value(QStringLiteral("chapterRef")));
    it.role = jsonStringOrEmpty(o.value(QStringLiteral("role")));
    return it;
}

QJsonObject drawerToJson(const Drawer& d) {
    QJsonObject o;
    o.insert(QStringLiteral("key"), d.key);
    o.insert(QStringLiteral("title"), d.title);
    o.insert(QStringLiteral("color"), d.color);
    o.insert(QStringLiteral("drawerIcon"), stringOrNull(d.drawerIcon));
    o.insert(QStringLiteral("drawerElementType"), stringOrNull(d.drawerElementType));
    o.insert(QStringLiteral("drawerElementIcon"), stringOrNull(d.drawerElementIcon));
    QJsonArray folders;
    for (const auto& f : d.folders) folders.append(folderToJson(f));
    o.insert(QStringLiteral("folders"), folders);
    QJsonArray items;
    for (const auto& it : d.items) items.append(drawerItemToJson(it));
    o.insert(QStringLiteral("items"), items);
    return o;
}

Drawer drawerFromJson(const QJsonObject& o) {
    Drawer d;
    d.key = jsonString(o.value(QStringLiteral("key")));
    d.title = jsonString(o.value(QStringLiteral("title")));
    d.color = jsonString(o.value(QStringLiteral("color")));
    d.drawerIcon = jsonStringOrEmpty(o.value(QStringLiteral("drawerIcon")));
    d.drawerElementType = jsonStringOrEmpty(o.value(QStringLiteral("drawerElementType")));
    d.drawerElementIcon = jsonStringOrEmpty(o.value(QStringLiteral("drawerElementIcon")));
    const QJsonArray folders = o.value(QStringLiteral("folders")).toArray();
    for (const auto& fv : folders) d.folders.append(folderFromJson(fv.toObject()));
    const QJsonArray items = o.value(QStringLiteral("items")).toArray();
    for (const auto& iv : items) d.items.append(drawerItemFromJson(iv.toObject()));
    return d;
}

} // namespace

ProjectModel::ProjectModel(QObject* parent)
    : QObject(parent) {}

void ProjectModel::setProjectName(const QString& name) {
    if (m_projectName == name) return;
    m_projectName = name;
    emit projectNameChanged();
}

void ProjectModel::setManuscripts(const QList<Manuscript>& list) {
    m_manuscripts = list;
    emit manuscriptsChanged();
}

void ProjectModel::setChapters(const QList<Chapter>& list) {
    m_chapters = list;
    emit chaptersChanged();
}

void ProjectModel::setDrawers(const QList<Drawer>& list) {
    m_drawers = list;
    emit drawersChanged();
}

void ProjectModel::setActiveManuscriptId(const QString& id) {
    if (m_activeManuscriptId == id) return;
    m_activeManuscriptId = id;
    emit activeManuscriptChanged();
}

void ProjectModel::setActiveChapterId(const QString& id) {
    if (m_activeChapterId == id) return;
    m_activeChapterId = id;
    emit activeChapterChanged();
}

void ProjectModel::setSettings(const QJsonObject& s) {
    m_settings = s;
    emit settingsChanged();
}

void ProjectModel::setUi(const QJsonObject& u) {
    m_ui = u;
    emit uiChanged();
}

void ProjectModel::setDataExtras(const QJsonObject& d) {
    m_dataExtras = d;
}

void ProjectModel::addDrawer(const Drawer& drawer) {
    m_drawers.append(drawer);
    emit drawersChanged();
}

bool ProjectModel::addDrawerItem(const QString& drawerKey, const DrawerItem& item) {
    for (auto& d : m_drawers) {
        if (d.key == drawerKey) {
            d.items.append(item);
            emit drawersChanged();
            return true;
        }
    }
    return false;
}

bool ProjectModel::addDrawerFolder(const QString& drawerKey, const Folder& folder) {
    for (auto& d : m_drawers) {
        if (d.key == drawerKey) {
            d.folders.append(folder);
            emit drawersChanged();
            return true;
        }
    }
    return false;
}

bool ProjectModel::moveDrawerItem(const QString& drawerKey, const QString& itemId, const QString& newFolderId) {
    for (auto& d : m_drawers) {
        if (d.key != drawerKey) continue;
        for (auto& it : d.items) {
            if (it.id == itemId) {
                if (it.folderId == newFolderId) return true;
                it.folderId = newFolderId;
                emit drawersChanged();
                return true;
            }
        }
    }
    return false;
}

bool ProjectModel::moveDrawerFolder(const QString& drawerKey, const QString& folderId, const QString& newParentId) {
    if (folderId == newParentId) return false; // não pode ser pai de si
    for (auto& d : m_drawers) {
        if (d.key != drawerKey) continue;
        for (auto& f : d.folders) {
            if (f.id == folderId) {
                if (f.parentId == newParentId) return true;
                f.parentId = newParentId;
                emit drawersChanged();
                return true;
            }
        }
    }
    return false;
}

const Drawer* ProjectModel::findDrawer(const QString& key) const {
    for (const auto& d : m_drawers) {
        if (d.key == key) return &d;
    }
    return nullptr;
}

bool ProjectModel::updateDrawerItemHtml(const QString& itemId, const QString& html) {
    for (auto& d : m_drawers) {
        for (auto& it : d.items) {
            if (it.id != itemId) continue;
            if (!it.file.isEmpty()) return false; // file-backed não muda inline
            if (it.html == html && it.hasInlineHtml) return true;
            it.html = html;
            it.hasInlineHtml = true;
            return true;
        }
    }
    return false;
}

bool ProjectModel::updateDrawerItemMeta(const QString& itemId, const QString& title, const QString& role) {
    for (auto& d : m_drawers) {
        for (auto& it : d.items) {
            if (it.id != itemId) continue;
            bool changed = false;
            if (!title.isNull() && it.title != title) { it.title = title; changed = true; }
            if (it.role != role) { it.role = role; changed = true; }
            if (changed) emit drawersChanged();
            return true;
        }
    }
    return false;
}

const DrawerItem* ProjectModel::findDrawerItem(const QString& itemId, QString* outDrawerKey) const {
    for (const auto& d : m_drawers) {
        for (const auto& it : d.items) {
            if (it.id == itemId) {
                if (outDrawerKey) *outDrawerKey = d.key;
                return &it;
            }
        }
    }
    return nullptr;
}

void ProjectModel::addManuscript(const Manuscript& manuscript) {
    m_manuscripts.append(manuscript);
    emit manuscriptsChanged();
}

void ProjectModel::addChapter(const Chapter& chapter) {
    Chapter c = chapter;
    if (c.file.isEmpty() && !c.id.isEmpty()) {
        c.file = chapterDefaultFile(c.manuscriptId, c.id);
    }
    if (c.order == 0) {
        int maxOrder = 0;
        for (const auto& existing : m_chapters) {
            if (existing.manuscriptId == c.manuscriptId && existing.order > maxOrder) {
                maxOrder = existing.order;
            }
        }
        c.order = maxOrder + 1;
    }
    m_chapters.append(c);
    emit chaptersChanged();
}

bool ProjectModel::updateChapterScenes(const QString& chapterId, const QList<Scene>& scenes) {
    for (auto& c : m_chapters) {
        if (c.id == chapterId) {
            if (c.scenes.size() == scenes.size()) {
                bool same = true;
                for (int i = 0; i < scenes.size(); ++i) {
                    if (c.scenes[i].id != scenes[i].id || c.scenes[i].order != scenes[i].order) {
                        same = false; break;
                    }
                }
                if (same) return true;
            }
            c.scenes = scenes;
            emit chaptersChanged();
            return true;
        }
    }
    return false;
}

const Chapter* ProjectModel::findChapter(const QString& chapterId) const {
    for (const auto& c : m_chapters) {
        if (c.id == chapterId) return &c;
    }
    return nullptr;
}

const Scene* ProjectModel::findScene(const QString& chapterId, int sceneIndex) const {
    const Chapter* ch = findChapter(chapterId);
    if (!ch || sceneIndex < 0 || sceneIndex >= ch->scenes.size()) return nullptr;
    return &ch->scenes.at(sceneIndex);
}

QString ProjectModel::createSceneVariation(const QString& chapterId, int sceneIndex, const QString& label) {
    for (auto& c : m_chapters) {
        if (c.id != chapterId) continue;
        if (sceneIndex < 0 || sceneIndex >= c.scenes.size()) return QString();
        Scene& scene = c.scenes[sceneIndex];

        Variation v;
        v.id = ProjectModel::uid();
        v.label = label.isEmpty() ? QStringLiteral("Variação %1").arg(scene.variations.size() + 1) : label;
        v.isPrimary = scene.variations.isEmpty();
        scene.variations.append(v);
        if (scene.activeVariationId.isEmpty() || scene.variations.size() == 1) {
            scene.activeVariationId = v.id;
        }
        emit chaptersChanged();
        return v.id;
    }
    return QString();
}

bool ProjectModel::switchActiveVariation(const QString& chapterId, int sceneIndex, const QString& variationId) {
    for (auto& c : m_chapters) {
        if (c.id != chapterId) continue;
        if (sceneIndex < 0 || sceneIndex >= c.scenes.size()) return false;
        Scene& scene = c.scenes[sceneIndex];
        for (const auto& v : scene.variations) {
            if (v.id == variationId) {
                if (scene.activeVariationId == variationId) return true;
                scene.activeVariationId = variationId;
                emit chaptersChanged();
                return true;
            }
        }
        return false;
    }
    return false;
}

bool ProjectModel::setPrimaryVariation(const QString& chapterId, int sceneIndex, const QString& variationId) {
    for (auto& c : m_chapters) {
        if (c.id != chapterId) continue;
        if (sceneIndex < 0 || sceneIndex >= c.scenes.size()) return false;
        Scene& scene = c.scenes[sceneIndex];
        bool found = false;
        for (auto& v : scene.variations) {
            v.isPrimary = (v.id == variationId);
            if (v.isPrimary) found = true;
        }
        if (found) emit chaptersChanged();
        return found;
    }
    return false;
}

bool ProjectModel::removeVariation(const QString& chapterId, int sceneIndex, const QString& variationId) {
    for (auto& c : m_chapters) {
        if (c.id != chapterId) continue;
        if (sceneIndex < 0 || sceneIndex >= c.scenes.size()) return false;
        Scene& scene = c.scenes[sceneIndex];
        if (scene.variations.size() <= 1) return false; // não permite remover a única
        const bool wasActive = (scene.activeVariationId == variationId);
        const bool wasPrimary = std::any_of(scene.variations.cbegin(), scene.variations.cend(),
            [&](const Variation& v) { return v.id == variationId && v.isPrimary; });
        const int before = scene.variations.size();
        scene.variations.erase(std::remove_if(scene.variations.begin(), scene.variations.end(),
            [&](const Variation& v) { return v.id == variationId; }), scene.variations.end());
        if (scene.variations.size() == before) return false;

        if (wasActive) scene.activeVariationId = scene.variations.first().id;
        if (wasPrimary) scene.variations.first().isPrimary = true;
        emit chaptersChanged();
        return true;
    }
    return false;
}

QList<Scene> ProjectModel::buildScenesFromHtml(const QString& html, const QList<Scene>& existing) {
    const int segmentCount = SceneUtils::countSceneDelimiters(html) + 1;
    if (segmentCount <= 1) return {};
    QList<Scene> scenes;
    scenes.reserve(segmentCount);
    for (int i = 0; i < segmentCount; ++i) {
        if (i < existing.size()) {
            Scene s = existing.at(i);
            s.order = i;
            scenes.append(s);
        } else {
            Scene s;
            s.id = ProjectModel::uid();
            s.title = QStringLiteral("Cena %1").arg(i + 1);
            s.order = i;
            scenes.append(s);
        }
    }
    return scenes;
}

QJsonObject ProjectModel::toJson() const {
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("projectName"), m_projectName.isEmpty() ? QStringLiteral("Projeto") : m_projectName);

    QJsonArray chapters;
    int idx = 0;
    for (const auto& c : m_chapters) {
        chapters.append(chapterToJson(c, idx + 1));
        ++idx;
    }
    root.insert(QStringLiteral("chapters"), chapters);

    QJsonArray drawers;
    for (const auto& d : m_drawers) drawers.append(drawerToJson(d));
    root.insert(QStringLiteral("drawers"), drawers);

    root.insert(QStringLiteral("settings"), m_settings);
    root.insert(QStringLiteral("ui"), m_ui);

    QJsonObject data = m_dataExtras;
    QJsonArray manuscripts;
    for (const auto& m : m_manuscripts) manuscripts.append(manuscriptToJson(m));
    data.insert(QStringLiteral("manuscripts"), manuscripts);
    data.insert(QStringLiteral("activeManuscriptId"), stringOrNull(m_activeManuscriptId));
    data.insert(QStringLiteral("activeChapterId"), stringOrNull(m_activeChapterId));
    root.insert(QStringLiteral("data"), data);

    return root;
}

void ProjectModel::loadFromJson(const QJsonObject& root) {
    m_projectName = jsonString(root.value(QStringLiteral("projectName")));

    m_chapters.clear();
    const QJsonArray chapters = root.value(QStringLiteral("chapters")).toArray();
    for (const auto& cv : chapters) m_chapters.append(chapterFromJson(cv.toObject()));

    m_drawers.clear();
    const QJsonArray drawers = root.value(QStringLiteral("drawers")).toArray();
    for (const auto& dv : drawers) m_drawers.append(drawerFromJson(dv.toObject()));

    m_settings = root.value(QStringLiteral("settings")).toObject();
    m_ui = root.value(QStringLiteral("ui")).toObject();

    QJsonObject data = root.value(QStringLiteral("data")).toObject();
    m_manuscripts.clear();
    const QJsonArray manuscripts = data.value(QStringLiteral("manuscripts")).toArray();
    for (const auto& mv : manuscripts) m_manuscripts.append(manuscriptFromJson(mv.toObject()));
    m_activeManuscriptId = jsonStringOrEmpty(data.value(QStringLiteral("activeManuscriptId")));
    m_activeChapterId = jsonStringOrEmpty(data.value(QStringLiteral("activeChapterId")));

    QJsonObject extras = data;
    extras.remove(QStringLiteral("manuscripts"));
    extras.remove(QStringLiteral("activeManuscriptId"));
    extras.remove(QStringLiteral("activeChapterId"));
    m_dataExtras = extras;

    emit projectNameChanged();
    emit manuscriptsChanged();
    emit chaptersChanged();
    emit drawersChanged();
    emit activeManuscriptChanged();
    emit activeChapterChanged();
    emit settingsChanged();
    emit uiChanged();
    emit loaded();
}

void ProjectModel::clear() {
    m_projectName.clear();
    m_manuscripts.clear();
    m_chapters.clear();
    m_drawers.clear();
    m_activeManuscriptId.clear();
    m_activeChapterId.clear();
    m_settings = QJsonObject();
    m_ui = QJsonObject();
    m_dataExtras = QJsonObject();
    emit projectNameChanged();
    emit manuscriptsChanged();
    emit chaptersChanged();
    emit drawersChanged();
    emit activeManuscriptChanged();
    emit activeChapterChanged();
    emit settingsChanged();
    emit uiChanged();
}

QString ProjectModel::uid() {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    constexpr int alen = sizeof(alphabet) - 1;
    QString out;
    out.reserve(9);
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < 9; ++i) {
        out.append(QLatin1Char(alphabet[rng->bounded(alen)]));
    }
    return out;
}

QString ProjectModel::chapterDefaultFile(const QString& manuscriptId, const QString& chapterId) {
    if (!manuscriptId.isEmpty()) {
        return QStringLiteral("content/manuscripts/ms_%1/chapters/ch_%2.html")
            .arg(manuscriptId, chapterId);
    }
    return QStringLiteral("content/chapters/ch_%1.html").arg(chapterId);
}
