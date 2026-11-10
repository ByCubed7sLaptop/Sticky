#ifndef NOTE_H
#define NOTE_H

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

class Note : public QWidget
{
public:
    QString metadataId;
    QString createdAt;
    QString updatedAt;
    bool archived = false;

    QLineEdit *title;
    QPlainTextEdit *editor;
    QPushButton *pinButton;
    int id;

    bool pinned = false;
    bool dragging = false;
    QPoint dragOffset;

    Note(int number, QString noteTitle, QString text,
         QRect geometry, bool isPinned = false)
        : id(number), pinned(isPinned)
   {
        setAttribute(Qt::WA_DeleteOnClose);

        setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
                       (pinned ? Qt::WindowStaysOnTopHint
                               : Qt::WindowFlags()));

        resize(geometry.size());
        move(geometry.topLeft());

        setStyleSheet(
            "QWidget { background: #fff29b; }"
            "QLineEdit {"
            " background: #f9e98a;"
            " border: none;"
            " color: #333333;"
            " font: bold 15px Sans;"
            "}"
            "QPlainTextEdit {"
            " background: #fff29b;"
            " border: none;"
            " color: #333333;"
            " font: 15px Sans;"
            "}"
            "QPushButton {"
            " background: #e9d66b;"
            " color: #333333;"
            " border: none;"
            "}"
            "QPushButton:hover {"
            " color: #333333;"
            " background: #d8c354;"
            "}"
        );

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(5, 5, 5, 5);
        layout->setSpacing(3);

        // Custom title bar
        auto *header = new QWidget;
        header->setStyleSheet("background: #f9e98a;");
        header->setMouseTracking(true);
        header->installEventFilter(this);

        auto *headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(3, 2, 3, 2);
        headerLayout->setSpacing(3);

        title = new QLineEdit;
        title->setText(noteTitle);
        title->setPlaceholderText("Note title...");
        title->installEventFilter(this);

        pinButton = new QPushButton;
        auto *minimize = new QPushButton("—");
        auto *close = new QPushButton("×");

        pinButton->setToolTip("Toggle always on top");
        minimize->setToolTip("Minimize note");
        close->setToolTip("Close note");

        updatePinButton();

        headerLayout->addWidget(title);
        headerLayout->addWidget(pinButton);
        headerLayout->addWidget(minimize);
        headerLayout->addWidget(close);

        // Note content
        editor = new QPlainTextEdit;

        editor->setPlainText(text);
        editor->setPlaceholderText("Write something...");

        layout->addWidget(header);
        layout->addWidget(editor);

        connect(title, &QLineEdit::textChanged,
                this, [this](const QString &value) {
            setWindowTitle(value.isEmpty()
                           ? "Sticky Note" : value);
        });

        connect(pinButton, &QPushButton::clicked,
                this, [this]() {
            togglePin();
        });

        connect(minimize, &QPushButton::clicked,
                this, [this]() {
            showMinimized();
        });

        connect(close, &QPushButton::clicked,
                this, &QWidget::close);

        setWindowTitle(noteTitle.isEmpty()
                       ? "Sticky Note" : noteTitle);

        resize(geometry.size());


        deserialize(text);
    }

    void updatePinButton()
    {
        pinButton->setText(pinned ? "📌" : "○");
        pinButton->setToolTip(
            pinned ? "Unpin from top" : "Pin on top");
    }

    void togglePin()
    {
        pinned = !pinned;

        setWindowFlag(Qt::WindowStaysOnTopHint, pinned);
        updatePinButton();

        show();
    }

    QString serialize() const
    {
        QString content = editor->toPlainText();

        QByteArray hash = QCryptographicHash::hash(
            content.toUtf8(),
            QCryptographicHash::Sha256
        ).toHex();

        QString metadata =
            "---\n"
            "markleaf_id: " + metadataId + "\n" +
            "created_at: " + createdAt + "\n" +
            "updated_at: " + updatedAt + "\n" +
            "pinned: " + QString(pinned ? "true" : "false") + "\n" +
            "archived: " + QString(archived ? "true" : "false") + "\n" +
            "body_sha256: " + QString::fromLatin1(hash) + "\n"
            "---\n";

        return metadata + content;
    }

    void deserialize(const QString &data)
    {
        static const QRegularExpression frontMatter(
            R"(^---\r?\n([\s\S]*?)\r?\n---(?:\r?\n|$))"
        );

        auto match = frontMatter.match(data);

        if (!match.hasMatch()) {
            editor->setPlainText(data);
            return;
        }

        QString metadata = match.captured(1);

        auto getValue = [&metadata](const QString &key) {
            QRegularExpression regex(
                "^" + QRegularExpression::escape(key) +
                R"(:\s*(.*?)\s*$)",
                QRegularExpression::MultilineOption
            );

            auto result = regex.match(metadata);
            return result.hasMatch() ? result.captured(1) : QString();
        };

        metadataId = getValue("markleaf_id");
        createdAt = getValue("created_at");
        updatedAt = getValue("updated_at");

        pinned = getValue("pinned") == "true";
        archived = getValue("archived") == "true";

        QString content = data.mid(match.capturedLength());

        editor->setPlainText(content);
    }

protected:

    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj == title || event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease) {

            if (event->type() == QEvent::MouseButtonPress) {
                auto *mouse = static_cast<QMouseEvent*>(event);

                if (mouse->button() == Qt::LeftButton) {
                    dragging = true;
                    dragOffset =
                        mouse->globalPosition().toPoint()
                        - frameGeometry().topLeft();
                }
            }

            if (event->type() == QEvent::MouseMove && dragging) {
                auto *mouse = static_cast<QMouseEvent*>(event);

                if (mouse->buttons() & Qt::LeftButton) {
                    move(mouse->globalPosition().toPoint()
                         - dragOffset);
                }
            }

            if (event->type() == QEvent::MouseButtonRelease) {
                dragging = false;
            }
        }

        return QWidget::eventFilter(obj, event);
    }
};

#endif
