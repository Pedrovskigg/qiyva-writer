#pragma once

#include <QObject>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

class DocCache : public QObject {
    Q_OBJECT
public:
    explicit DocCache(QObject* parent = nullptr);

    static QString chapterKey(const QString& manuscriptId, const QString& chapterId);
    static QString itemKey(const QString& itemId);

    bool has(const QString& key) const;
    QString get(const QString& key) const;

    void set(const QString& key, const QString& html, bool markDirty = true);
    void markClean(const QString& key);
    void markDirty(const QString& key);

    bool isDirty(const QString& key) const;
    QStringList dirtyKeys() const;
    int size() const { return m_content.size(); }

    void evict(const QString& key);
    void clear();

signals:
    void contentChanged(QString key);
    void dirtyChanged(QString key, bool dirty);

private:
    QHash<QString, QString> m_content;
    QSet<QString> m_dirty;
};
