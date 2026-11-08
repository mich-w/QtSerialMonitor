#include "commandhistory.h"

CommandHistory::CommandHistory(int maxSize) : maxSize(qMax(1, maxSize))
{
}

void CommandHistory::add(const QString &command)
{
    resetNavigation();
    if (command.isEmpty())
        return;

    entries.removeAll(command);
    entries.prepend(command);
    while (entries.count() > maxSize)
        entries.removeLast();
}

void CommandHistory::setItems(const QStringList &items)
{
    entries.clear();
    for (const auto &item : items)
        if (!item.isEmpty() && !entries.contains(item) && entries.count() < maxSize)
            entries.append(item);
    resetNavigation();
}

QStringList CommandHistory::items() const
{
    return entries;
}

void CommandHistory::clear()
{
    entries.clear();
    resetNavigation();
}

QString CommandHistory::older(const QString &currentText)
{
    if (index + 1 >= entries.count())
        return index < 0 ? currentText : entries[index];

    if (index < 0)
        draft = currentText;
    ++index;
    return entries[index];
}

QString CommandHistory::newer(const QString &currentText)
{
    if (index < 0)
        return currentText;

    --index;
    return index < 0 ? draft : entries[index];
}

void CommandHistory::resetNavigation()
{
    index = -1;
    draft.clear();
}
