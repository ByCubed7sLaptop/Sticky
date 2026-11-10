
#include "Utils.h"
#include "Note.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QFileInfoList>
#include <QUuid>
#include <QCryptographicHash>
#include <QDateTime>
#include <QRegularExpression>

// --------------------------------------------------
// Settings
// --------------------------------------------------

QSettings Utils::createSettings()
{
    return QSettings("DesktopApps", "StickyNotes");
}

// --------------------------------------------------
// Directory management
// --------------------------------------------------

QString Utils::notesDirectory()
{
    QSettings settings = createSettings();

    QString dir = settings.value("notesDirectory").toString();

    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);
    }

    QDir().mkpath(dir);

    return QDir(dir).absolutePath();
}

bool Utils::setNotesDirectory(const QString &directory)
{
    if (directory.trimmed().isEmpty())
        return false;

    QDir dir(directory);

    if (!dir.exists() && !dir.mkpath("."))
        return false;

    QSettings settings = createSettings();

    settings.setValue("notesDirectory",
                      dir.absolutePath());

    settings.sync();

    return settings.status() == QSettings::NoError;
}

// --------------------------------------------------
// Filename utilities
// --------------------------------------------------

QString Utils::sanitizeFileName(const QString &title)
{
    QString name = title.trimmed();

    name.replace(
        QRegularExpression(R"([<>:"/\\|?*])"),
        "_"
    );

    while (name.endsWith('.') || name.endsWith(' '))
        name.chop(1);

    if (name.isEmpty())
        name = "Untitled";

    return name;
}

QString Utils::noteFileName(const QString &title)
{
    return sanitizeFileName(title) + ".md";
}

QString Utils::noteConfigFileName(int id)
{
    return QString("note_%1.cfg").arg(id);
}

QString Utils::noteFilePath(int id)
{
    return QDir(notesDirectory()).filePath(
        noteFileName(QString("Note_%1").arg(id))
    );
}

QString Utils::configFilePath(int id)
{
    return QDir(notesDirectory()).filePath(
        noteConfigFileName(id)
    );
}

// --------------------------------------------------
// File operations
// --------------------------------------------------

bool Utils::saveTextFile(const QString &path,
                             const QString &content)
{
    QFile file(path);

    if (file.open(QIODevice::WriteOnly |
                    QIODevice::Text |
                    QIODevice::Truncate)) {

        QTextStream out(&file);
        out << content;

        file.close();
        return true;
    }

    return false;
}

QString Utils::readTextFile(const QString &path)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly |
                   QIODevice::Text)) {
        return {};
    }

    return QString::fromUtf8(file.readAll());
}

bool Utils::removeNoteFiles(int id)
{
    QString cfgPath = configFilePath(id);

    QSettings cfg(cfgPath, QSettings::IniFormat);

    QString fileName = cfg.value("Note/fileName").toString();

    return removeNoteFiles(id, fileName);
}

bool Utils::removeNoteFiles(int id,
                                const QString &fileName)
{
    bool success = true;

    if (!fileName.isEmpty() &&
        isValidNoteFileName(fileName)) {

        QString mdPath =
            QDir(notesDirectory()).filePath(fileName);

        if (QFile::exists(mdPath))
            success &= QFile::remove(mdPath);
    }

    QString cfgPath = configFilePath(id);

    if (QFile::exists(cfgPath))
        success &= QFile::remove(cfgPath);

    return success;
}

// --------------------------------------------------
// Note configuration
// --------------------------------------------------

bool Utils::saveNoteConfig(const Note *note)
{
    if (!note)
        return false;

    QSettings cfg(configFilePath(note->id),
                  QSettings::IniFormat);

    cfg.beginGroup("Note");

    cfg.setValue("id", note->id);
    cfg.setValue("title", note->title->text());
    cfg.setValue("fileName",
                 noteFileName(note->title->text()));

    cfg.setValue("x", note->x());
    cfg.setValue("y", note->y());
    cfg.setValue("width", note->width());
    cfg.setValue("height", note->height());

    cfg.setValue("pinned", note->pinned);

    cfg.endGroup();

    cfg.sync();

    return cfg.status() == QSettings::NoError;
}

bool Utils::loadNoteConfig(int id,
                               QString &title,
                               QRect &geometry,
                               bool &pinned,
                               QString &fileName)
{
    QSettings cfg(configFilePath(id),
                  QSettings::IniFormat);

    if (!QFile::exists(configFilePath(id)))
        return false;

    cfg.beginGroup("Note");

    int storedId = cfg.value("id", -1).toInt();

    if (storedId != id) {
        cfg.endGroup();
        return false;
    }

    title = cfg.value("title", "Untitled").toString();

    fileName = cfg.value("fileName").toString();

    if (fileName.isEmpty())
        fileName = noteFileName(title);

    geometry = QRect(
        cfg.value("x", 300).toInt(),
        cfg.value("y", 200).toInt(),
        cfg.value("width", 280).toInt(),
        cfg.value("height", 240).toInt()
    );

    pinned = cfg.value("pinned", true).toBool();

    cfg.endGroup();

    return true;
}

// --------------------------------------------------
// Note discovery
// --------------------------------------------------

QStringList Utils::configFiles()
{
    QDir dir(notesDirectory());

    return dir.entryList(
        QStringList() << "note_*.cfg",
        QDir::Files,
        QDir::Name
    );
}

QVector<int> Utils::existingNoteIds()
{
    QVector<int> ids;

    const QStringList files = configFiles();

    QRegularExpression regex(R"(^note_(\d+)\.cfg$)");

    for (const QString &file : files) {

        QRegularExpressionMatch match =
            regex.match(file);

        if (match.hasMatch()) {
            bool ok = false;

            int id = match.captured(1).toInt(&ok);

            if (ok)
                ids.append(id);
        }
    }

    return ids;
}

// --------------------------------------------------
// Validation
// --------------------------------------------------

bool Utils::isValidNoteId(int id)
{
    return id >= 0;
}

bool Utils::isValidNoteFileName(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;

    QFileInfo info(fileName);

    return info.fileName() == fileName &&
           fileName.endsWith(".md", Qt::CaseInsensitive) &&
           fileName != "." &&
           fileName != "..";
}
