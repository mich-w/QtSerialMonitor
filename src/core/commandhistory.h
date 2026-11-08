#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include <QStringList>

// Shell-like history of sent commands: most recent first, no duplicates,
// Up/Down navigation that restores the unsent draft when going past the newest entry.
class CommandHistory
{
public:
    explicit CommandHistory(int maxSize = 100);

    void add(const QString &command);
    void setItems(const QStringList &items);
    QStringList items() const;
    void clear();

    QString older(const QString &currentText); // Up
    QString newer(const QString &currentText); // Down
    void resetNavigation();

private:
    QStringList entries;
    QString draft;
    int index = -1; // -1 = editing the draft
    int maxSize;
};

#endif // COMMANDHISTORY_H
