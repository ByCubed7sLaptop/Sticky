
#include <QApplication>
#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>
#include <QTimer>
#include <QLabel>
#include <QVector>
#include <QLineEdit>
#include <QMouseEvent>
#include <QEvent>
#include <QPoint>
#include <QTextStream>
#include <QSettings>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>
#include <QCryptographicHash>
#include <QDateTime>
#include <QRegularExpression>

#include "Utils.h"
#include "Note.h"

static QString notesDirectory()
{
    QSettings settings("DesktopApps", "StickyNotes");

    QString dir = settings.value("notesDirectory").toString();

    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);
    }

    QDir().mkpath(dir);
    return dir;
}

static QString noteFileName(const QString &title)
{
    QString name = title.trimmed();

    // Replace characters that are invalid in filenames.
    name.replace(QRegularExpression(R"([<>:"/\\|?*])"), "_");

    // Remove trailing dots and spaces.
    while (name.endsWith('.') || name.endsWith(' '))
        name.chop(1);

    if (name.isEmpty())
        name = "Untitled";

    return name + ".md";
}





class StickyApp : public QWidget
{
    QVector<Note*> notes;
    int nextId = 0;
    bool loading = false;

public:
    StickyApp()
    {
        setWindowTitle("Sticky Notes Manager");
        resize(330, 150);

        auto *layout = new QVBoxLayout(this);

        auto *add = new QPushButton("+ New Sticky Note");
        auto *choose = new QPushButton("Save Notes To Directory...");

        auto *location = new QLabel;
        location->setWordWrap(true);

        layout->addWidget(add);
        layout->addWidget(choose);
        layout->addWidget(new QLabel("Current save location:"));
        layout->addWidget(location);

        connect(add, &QPushButton::clicked,
                this, [this]() {
            addNote();
        });

        connect(choose, &QPushButton::clicked,
                this, [this, location]() {

            QString dir = QFileDialog::getExistingDirectory(
                this,
                "Choose Notes Directory",
                Utils::notesDirectory()
            );

            if (!dir.isEmpty() &&
                Utils::setNotesDirectory(dir)) {

                location->setText(dir);
                saveNotes();
            }
        });

        location->setText(notesDirectory());

        loadNotes();

        //if (notes.isEmpty())
        //    addNote();

        auto *timer = new QTimer(this);

        connect(timer, &QTimer::timeout,
                this, [this]() {
            saveNotes();
        });

        timer->start(30000);
    }


    void addNote(QString text = "",
                 QRect geometry = QRect(300, 200, 280, 240),
                 QString noteTitle = "New Note",
                 bool pinned = true)
    {
        Note *note = new Note(
            nextId++, noteTitle, text, geometry, pinned);

        notes.append(note);

        //connect(note->editor, &QPlainTextEdit::textChanged, this, [this]() { saveNotes(); });

        //connect(note->title, &QLineEdit::textChanged, this, [this]() { saveNotes(); });

        int id = note->id;

        connect(note, &QObject::destroyed,
                this, [this, id]() {
//            notes.removeIf(
//                [id](Note *n) {
//                    return !n || n->id == id;
//                }
//            );
//
//            if (!QApplication::closingDown()) {
//                QString base = notesDirectory() + QString("/note_%1").arg(id);
//                QFile::remove(base + ".md");
//                QFile::remove(base + ".cfg");
//            }

            saveNotes();
        });

        note->show();
        saveNotes();
    }




    void saveNotes()
    {
        if (loading)
            return;

        for (Note *note : notes) {

            if (!note)
                continue;

            QString newFileName = Utils::noteFileName(note->title->text());

            QString cfgPath = Utils::configFilePath(note->id);

            QSettings cfg(cfgPath, QSettings::IniFormat);

            QString oldFileName = cfg.value("Note/fileName").toString();

            QString newPath = QDir(Utils::notesDirectory()).filePath(newFileName);

            // Rename old Markdown file if necessary.
            if (!oldFileName.isEmpty() &&
                oldFileName != newFileName) {

                QString oldPath =
                    QDir(Utils::notesDirectory())
                        .filePath(oldFileName);

                if (QFile::exists(oldPath) &&
                    !QFile::exists(newPath)) {
                    QFile::rename(oldPath, newPath);
                }
            }

            note->updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

            // Save Markdown content.
            Utils::saveTextFile(
                newPath,
                note->serialize()
            );

            // Save configuration.
            Utils::saveNoteConfig(note);
        }
    }

    void loadNotes()
    {
        loading = true;

        const QStringList configs =
            Utils::configFiles();

        for (const QString &cfgName : configs) {

            QRegularExpression regex(
                R"(^note_(\d+)\.cfg$)"
            );

            auto match = regex.match(cfgName);

            if (!match.hasMatch())
                continue;

            int id = match.captured(1).toInt();

            QString noteTitle;
            QString fileName;
            QRect geometry;
            bool pinned;

            if (!Utils::loadNoteConfig(
                    id,
                    noteTitle,
                    geometry,
                    pinned,
                    fileName)) {
                continue;
            }

            QString mdPath =
                QDir(Utils::notesDirectory())
                    .filePath(fileName);

            QString content = Utils::readTextFile(mdPath);

            nextId = qMax(nextId, id + 1);

            Note *note = new Note(
                id,
                noteTitle,
                content,
                geometry,
                pinned
            );

            notes.append(note);

            connect(note->editor,
                    &QPlainTextEdit::textChanged,
                    this,
                    [this]() { saveNotes(); });

            connect(note->title,
                    &QLineEdit::editingFinished,
                    this,
                    [this]() { saveNotes(); });

            connect(note, &QObject::destroyed,
                    this, [this, id]() {

//                notes.removeIf(
//                    [id](Note *n) {
//                        return !n || n->id == id;
//                    }
//                );

                //if (!QApplication::closingDown())
                //    Utils::removeNoteFiles(id);

                saveNotes();
            });

            note->show();
        }

        loading = false;
    }


};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("StickyNotes");
    app.setOrganizationName("DesktopApps");

    StickyApp window;
    window.show();

    return app.exec();
}
