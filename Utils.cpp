
#include "Utils.h"
#include "Note.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QRegularExpression>
#include <QDateTime>

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

    QString directory =
        settings.value("notesDirectory").toString();

    if (directory.isEmpty()) {
        directory = QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation
        );
    }

    QDir().mkpath(directory);

    return QDir(directory).absolutePath();
}

bool Utils::setNotesDirectory(const QString &directory)
{
    if (directory.trimmed().isEmpty())
        return false;

    QDir dir(directory);

    if (!dir.exists() && !dir.mkpath("."))
        return false;

    QSettings settings = createSettings();

    settings.setValue(
        "notesDirectory",
        dir.absolutePath()
    );

    settings.sync();

    return settings.status() == QSettings::NoError;
}

// --------------------------------------------------
// Filename utilities
// --------------------------------------------------

QString Utils::sanitizeFileName(const QString &title)
{
    QString name = title.trimmed();

    // Replace characters invalid in filenames.
    static const QRegularExpression invalidCharacters(
        R"([<>:"/\\|?*])"
    );

    name.replace(invalidCharacters, "_");

    // Remove trailing dots and spaces.
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
    const QString fileName =
        noteFileName(QString("Note_%1").arg(id));

    return QDir(notesDirectory()).filePath(fileName);
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

bool Utils::saveTextFile(
    const QString &path,
    const QString &content)
{
    QFile file(path);

    if (!file.open(
            QIODevice::WriteOnly |
            QIODevice::Text |
            QIODevice::Truncate)) {
        return false;
    }

    QTextStream out(&file);
    out << content;

    return file.error() == QFile::NoError;
}

QString Utils::readTextFile(const QString &path)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    return QString::fromUtf8(file.readAll());
}

bool Utils::removeNoteFiles(int id)
{
    const QString configPath = configFilePath(id);

    QSettings config(configPath, QSettings::IniFormat);

    const QString fileName =
        config.value("Note/fileName").toString();

    return removeNoteFiles(id, fileName);
}

bool Utils::removeNoteFiles(
    int id,
    const QString &fileName)
{
    bool success = true;

    if (!fileName.isEmpty() &&
        isValidNoteFileName(fileName)) {

        const QString markdownPath =
            QDir(notesDirectory()).filePath(fileName);

        if (QFile::exists(markdownPath))
            success = QFile::remove(markdownPath);
    }

    const QString configPath = configFilePath(id);

    if (QFile::exists(configPath))
        success = QFile::remove(configPath) && success;

    return success;
}

// --------------------------------------------------
// Note configuration
// --------------------------------------------------

bool Utils::saveNoteConfig(const Note *note)
{
    if (!note || !note->title)
        return false;

    QSettings config(
        configFilePath(note->id),
        QSettings::IniFormat
    );

    config.beginGroup("Note");

    config.setValue("id", note->id);
    config.setValue("title", note->title->text());
    config.setValue(
        "fileName",
        noteFileName(note->title->text())
    );

    config.setValue("x", note->x());
    config.setValue("y", note->y());
    config.setValue("width", note->width());
    config.setValue("height", note->height());

    config.setValue("pinned", note->pinned);

    config.endGroup();

    config.sync();

    return config.status() == QSettings::NoError;
}

bool Utils::loadNoteConfig(
    int id,
    QString &title,
    QRect &geometry,
    bool &pinned,
    QString &fileName)
{
    const QString configPath = configFilePath(id);

    if (!QFile::exists(configPath))
        return false;

    QSettings config(configPath, QSettings::IniFormat);

    config.beginGroup("Note");

    const int storedId =
        config.value("id", -1).toInt();

    if (storedId != id) {
        config.endGroup();
        return false;
    }

    title = config.value("title", "Untitled").toString();

    fileName = config.value("fileName").toString();

    if (fileName.isEmpty())
        fileName = noteFileName(title);

    if (!isValidNoteFileName(fileName)) {
        config.endGroup();
        return false;
    }

    geometry = QRect(
        config.value("x", 300).toInt(),
        config.value("y", 200).toInt(),
        config.value("width", 280).toInt(),
        config.value("height", 240).toInt()
    );

    pinned = config.value("pinned", true).toBool();

    config.endGroup();

    return true;
}

// --------------------------------------------------
// Note discovery
// --------------------------------------------------

QStringList Utils::configFiles()
{
    QDir directory(notesDirectory());

    return directory.entryList(
        {"note_*.cfg"},
        QDir::Files,
        QDir::Name
    );
}

QVector<int> Utils::existingNoteIds()
{
    QVector<int> ids;

    static const QRegularExpression configRegex(
        R"(^note_(\d+)\.cfg$)"
    );

    const QStringList files = configFiles();

    for (const QString &file : files) {
        const auto match = configRegex.match(file);

        if (!match.hasMatch())
            continue;

        bool ok = false;

        const int id = match.captured(1).toInt(&ok);

        if (ok)
            ids.append(id);
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

    const QFileInfo info(fileName);

    return info.fileName() == fileName &&
           fileName.endsWith(".md", Qt::CaseInsensitive) &&
           fileName != "." &&
           fileName != "..";
}
