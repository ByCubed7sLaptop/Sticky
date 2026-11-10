
#include <QApplication>
#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QVector>
#include <QFileDialog>
#include <QSettings>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QRegularExpression>
#include <QRect>

#include "Utils.h"
#include "Note.h"

class StickyApp : public QWidget
{

private:
    QVector<Note*> notes;
    int nextId = 0;
    bool loading = false;

public:
    explicit StickyApp(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setWindowTitle("Sticky Notes Manager");
        resize(330, 150);

        auto *layout = new QVBoxLayout(this);

        auto *addButton = new QPushButton("+ New Sticky Note", this);
        auto *directoryButton =
            new QPushButton("Save Notes To Directory...", this);

        auto *locationLabel = new QLabel(this);
        locationLabel->setWordWrap(true);

        layout->addWidget(addButton);
        layout->addWidget(directoryButton);
        layout->addWidget(new QLabel("Current save location:", this));
        layout->addWidget(locationLabel);

        connect(addButton, &QPushButton::clicked,
                this, [this]() {
            addNote();
        });

        connect(directoryButton, &QPushButton::clicked,
                this, [this, locationLabel]() {
            const QString directory = QFileDialog::getExistingDirectory(
                this,
                "Choose Notes Directory",
                Utils::notesDirectory()
            );

            if (directory.isEmpty())
                return;

            if (Utils::setNotesDirectory(directory)) {
                locationLabel->setText(directory);
                saveNotes();
            }
        });

        locationLabel->setText(Utils::notesDirectory());

        loadNotes();

        auto *saveTimer = new QTimer(this);

        connect(saveTimer, &QTimer::timeout,
                this, &StickyApp::saveNotes);

        saveTimer->start(30000);
    }

private:
    void addNote(
        const QString &text = QString(),
        const QRect &geometry = QRect(300, 200, 280, 240),
        const QString &title = "New Note",
        bool pinned = true)
    {
        auto *note = new Note(
            nextId++,
            title,
            text,
            geometry,
            pinned
        );

        notes.append(note);

        connect(note->editor, &QPlainTextEdit::textChanged,
                this, &StickyApp::saveNotes);

        connect(note->title, &QLineEdit::editingFinished,
                this, &StickyApp::saveNotes);

        connect(note, &QObject::destroyed,
                this, &StickyApp::saveNotes);

        note->show();

        saveNotes();
    }

    void saveNotes()
    {
        if (loading)
            return;

        const QString directory = Utils::notesDirectory();

        for (Note *note : notes) {
            if (!note)
                continue;

            const QString newFileName =
                Utils::noteFileName(note->title->text());

            const QString configPath =
                Utils::configFilePath(note->id);

            QSettings config(configPath, QSettings::IniFormat);

            const QString oldFileName =
                config.value("Note/fileName").toString();

            const QString newPath =
                QDir(directory).filePath(newFileName);

            // Rename the Markdown file when the note title changes.
            if (!oldFileName.isEmpty() &&
                oldFileName != newFileName) {

                const QString oldPath =
                    QDir(directory).filePath(oldFileName);

                if (QFile::exists(oldPath) &&
                    !QFile::exists(newPath)) {
                    QFile::rename(oldPath, newPath);
                }
            }

            // Update modification timestamp.
            note->updatedAt =
                QDateTime::currentDateTimeUtc()
                    .toString(Qt::ISODateWithMs);

            // Save Markdown content.
            Utils::saveTextFile(
                newPath,
                note->serialize()
            );

            // Save note configuration.
            Utils::saveNoteConfig(note);
        }
    }

    void loadNotes()
    {
        loading = true;

        static const QRegularExpression configRegex(
            R"(^note_(\d+)\.cfg$)"
        );

        const QStringList configFiles = Utils::configFiles();

        for (const QString &configName : configFiles) {
            const auto match = configRegex.match(configName);

            if (!match.hasMatch())
                continue;

            const int id = match.captured(1).toInt();

            QString title;
            QString fileName;
            QRect geometry;
            bool pinned = true;

            if (!Utils::loadNoteConfig(
                    id,
                    title,
                    geometry,
                    pinned,
                    fileName)) {
                continue;
            }

            const QString markdownPath =
                QDir(Utils::notesDirectory()).filePath(fileName);

            const QString content =
                Utils::readTextFile(markdownPath);

            nextId = qMax(nextId, id + 1);

            auto *note = new Note(
                id,
                title,
                content,
                geometry,
                pinned
            );

            notes.append(note);

            connect(note->editor, &QPlainTextEdit::textChanged,
                    this, &StickyApp::saveNotes);

            connect(note->title, &QLineEdit::editingFinished,
                    this, &StickyApp::saveNotes);

            connect(note, &QObject::destroyed,
                    this, &StickyApp::saveNotes);

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
