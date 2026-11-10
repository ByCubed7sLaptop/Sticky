#ifndef NOTE_H
#define NOTE_H

#include <QWidget>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QEvent>
#include <QPoint>
#include <QRegularExpression>
#include <QCryptographicHash>

class Note : public QWidget
{
public:
    // Note metadata
    QString metadataId;
    QString createdAt;
    QString updatedAt;

    bool archived = false;
    bool pinned = false;

    // Note identification
    int id = -1;

    // UI elements
    QLineEdit *title = nullptr;
    QPlainTextEdit *editor = nullptr;
    QPushButton *pinButton = nullptr;

    // Dragging state
    bool dragging = false;
    QPoint dragOffset;

    explicit Note(
        int number,
        const QString &noteTitle,
        const QString &text,
        const QRect &geometry,
        bool isPinned = false,
        QWidget *parent = nullptr)
        : QWidget(parent),
          pinned(isPinned),
          id(number)
    {
        setAttribute(Qt::WA_DeleteOnClose);

        setWindowFlags(
            Qt::Window |
            Qt::FramelessWindowHint |
            (pinned ? Qt::WindowStaysOnTopHint : Qt::WindowFlags())
        );

        resize(geometry.size());
        move(geometry.topLeft());

        // Styling

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
            " background: #d8c354;"
            " color: #333333;"
            "}"
        );

        // Main layout

        auto *layout = new QVBoxLayout(this);

        layout->setContentsMargins(5, 5, 5, 5);
        layout->setSpacing(3);

        // Title bar

        auto *header = new QWidget(this);

        header->setStyleSheet("background: #f9e98a;");
        header->setMouseTracking(true);
        header->installEventFilter(this);

        auto *headerLayout = new QHBoxLayout(header);

        headerLayout->setContentsMargins(3, 2, 3, 2);
        headerLayout->setSpacing(3);

        title = new QLineEdit(header);

        title->setText(noteTitle);
        title->setPlaceholderText("Note title...");
        title->installEventFilter(this);

        pinButton = new QPushButton(header);

        auto *minimizeButton = new QPushButton("—", header);
        auto *closeButton = new QPushButton("×", header);

        pinButton->setToolTip("Toggle always on top");
        minimizeButton->setToolTip("Minimize note");
        closeButton->setToolTip("Close note");

        updatePinButton();

        headerLayout->addWidget(title);
        headerLayout->addWidget(pinButton);
        headerLayout->addWidget(minimizeButton);
        headerLayout->addWidget(closeButton);

        // Note editor

        editor = new QPlainTextEdit(this);

        editor->setPlaceholderText("Write something...");

        layout->addWidget(header);
        layout->addWidget(editor);

        // Connections

        connect(title, &QLineEdit::textChanged,
                this, [this](const QString &value) {
            setWindowTitle(
                value.isEmpty() ? "Sticky Note" : value
            );
        });

        connect(pinButton, &QPushButton::clicked,
                this, &Note::togglePin);

        connect(minimizeButton, &QPushButton::clicked,
                this, &QWidget::showMinimized);

        connect(closeButton, &QPushButton::clicked,
                this, &QWidget::close);

        setWindowTitle(
            noteTitle.isEmpty() ? "Sticky Note" : noteTitle
        );

        // Restore editor content and metadata.
        deserialize(text);
    }

    // Pin management

    void updatePinButton()
    {
        if (!pinButton)
            return;

        pinButton->setText(pinned ? "📌" : "○");

        pinButton->setToolTip(
            pinned ? "Unpin from top" : "Pin on top"
        );
    }

    void togglePin()
    {
        pinned = !pinned;

        setWindowFlag(Qt::WindowStaysOnTopHint, pinned);

        updatePinButton();

        show();
    }

    // Serialization

    QString serialize() const
    {
        const QString content = editor->toPlainText();

        // Markleafs Metadata can be disabled in the config
//        const QByteArray hash =
//            QCryptographicHash::hash(
//                content.toUtf8(),
//                QCryptographicHash::Sha256
//            ).toHex();
//        QString metadata;
//        metadata += "---\n";
//        metadata += "markleaf_id: " + metadataId + "\n";
//        metadata += "created_at: " + createdAt + "\n";
//        metadata += "updated_at: " + updatedAt + "\n";
//        metadata += "pinned: " +
//                    QString(pinned ? "true" : "false") + "\n";
//        metadata += "archived: " +
//                    QString(archived ? "true" : "false") + "\n";
//        metadata += "body_sha256: " +
//                    QString::fromLatin1(hash) + "\n";
//        metadata += "---\n";
//        return metadata + content;
        return title->text() + "\n" + content;
    }

    void deserialize(const QString &data)
    {
        static const QRegularExpression frontMatter(
            R"(^---\r?\n([\s\S]*?)\r?\n---(?:\r?\n|$))"
        );

        const auto match = frontMatter.match(data);

        if (!match.hasMatch()) {
            if (data.startsWith(title->text())) {
                data.remove(0, data.indexOf('\n') + 1);
            }
            editor->setPlainText(data);
            return;
        }

        const QString metadata = match.captured(1);

        const auto getValue = [&metadata](const QString &key) {
            const QRegularExpression regex(
                "^" + QRegularExpression::escape(key) +
                R"(:\s*(.*?)\s*$)",
                QRegularExpression::MultilineOption
            );

            const auto result = regex.match(metadata);

            return result.hasMatch()
                ? result.captured(1)
                : QString();
        };

        metadataId = getValue("markleaf_id");
        createdAt = getValue("created_at");
        updatedAt = getValue("updated_at");

        pinned = getValue("pinned") == "true";
        archived = getValue("archived") == "true";

        const QString content =
            data.mid(match.capturedLength());

        editor->setPlainText(content);

        updatePinButton();
    }

protected:

    // Mouse event handling

    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj != title && obj != parentWidget() &&
            event->type() != QEvent::MouseButtonPress &&
            event->type() != QEvent::MouseMove &&
            event->type() != QEvent::MouseButtonRelease) {
            return QWidget::eventFilter(obj, event);
        }

        switch (event->type()) {

        case QEvent::MouseButtonPress: {
            auto *mouse = static_cast<QMouseEvent*>(event);

            if (mouse->button() == Qt::LeftButton) {
                dragging = true;

                dragOffset =
                    mouse->globalPosition().toPoint() -
                    frameGeometry().topLeft();
            }

            break;
        }

        case QEvent::MouseMove: {
            auto *mouse = static_cast<QMouseEvent*>(event);

            if (dragging &&
                (mouse->buttons() & Qt::LeftButton)) {

                move(
                    mouse->globalPosition().toPoint() -
                    dragOffset
                );
            }

            break;
        }

        case QEvent::MouseButtonRelease:
            dragging = false;
            break;

        default:
            break;
        }

        return QWidget::eventFilter(obj, event);
    }
};

#endif // NOTE_H
