#ifndef Utils_H
#define Utils_H

#include <QString>
#include <QRect>
#include <QSettings>
#include <QVector>

#include "Note.h"

class Note;

class Utils
{
public:

    // Directory management
    static QString notesDirectory();
    static bool setNotesDirectory(const QString &directory);

    // Filename utilities
    static QString sanitizeFileName(const QString &title);
    static QString noteFileName(const QString &title);
    static QString noteConfigFileName(int id);
    static QString noteFilePath(int id);
    static QString configFilePath(int id);

    // File operations
    static bool saveTextFile(const QString &path,
                             const QString &content);

    static QString readTextFile(const QString &path);

    static bool removeNoteFiles(int id);
    static bool removeNoteFiles(int id,
                                const QString &fileName);

    // Note configuration
    static bool saveNoteConfig(const Note *note);
    static bool loadNoteConfig(int id,
                               QString &title,
                               QRect &geometry,
                               bool &pinned,
                               QString &fileName);

    // Note discovery
    static QStringList configFiles();
    static QVector<int> existingNoteIds();

    // Settings
    static QSettings createSettings();

    // Validation
    static bool isValidNoteId(int id);
    static bool isValidNoteFileName(const QString &fileName);
};

#endif
