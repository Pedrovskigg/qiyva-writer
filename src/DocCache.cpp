#include "DocCache.h"

DocCache::DocCache(QObject* parent)
    : QObject(parent) {}

QString DocCache::chapterKey(const QString& manuscriptId, const QString& chapterId) {
    const QString ms = manuscriptId.isEmpty() ? QStringLiteral("root") : manuscriptId;
    return QStringLiteral("ch:%1:%2").arg(ms, chapterId);
}

QString DocCache::itemKey(const QString& itemId) {
    return QStringLiteral("it:%1").arg(itemId);
}

bool DocCache::has(const QString& key) const {
    return m_content.contains(key);
}

QString DocCache::get(const QString& key) const {
    return m_content.value(key);
}

void DocCache::set(const QString& key, const QString& html, bool markDirtyFlag) {
    const bool existed = m_content.contains(key);
    const bool changed = !existed || m_content.value(key) != html;
    m_content.insert(key, html);
    if (changed) emit contentChanged(key);
    if (markDirtyFlag) {
        markDirty(key);
    }
}

void DocCache::markClean(const QString& key) {
    if (m_dirty.remove(key)) {
        emit dirtyChanged(key, false);
    }
}

void DocCache::markDirty(const QString& key) {
    if (!m_dirty.contains(key)) {
        m_dirty.insert(key);
        emit dirtyChanged(key, true);
    }
}

bool DocCache::isDirty(const QString& key) const {
    return m_dirty.contains(key);
}

QStringList DocCache::dirtyKeys() const {
    return QStringList(m_dirty.cbegin(), m_dirty.cend());
}

void DocCache::evict(const QString& key) {
    const bool removed = m_content.remove(key) > 0;
    const bool wasDirty = m_dirty.remove(key) > 0;
    if (removed) emit contentChanged(key);
    if (wasDirty) emit dirtyChanged(key, false);
}

void DocCache::clear() {
    const QList<QString> keys = m_content.keys();
    m_content.clear();
    const QList<QString> dirties = QList<QString>(m_dirty.cbegin(), m_dirty.cend());
    m_dirty.clear();
    for (const auto& k : keys) emit contentChanged(k);
    for (const auto& k : dirties) emit dirtyChanged(k, false);
}
