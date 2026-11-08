#include "commandhistory.h"

#include <QtTest>

class TestCommandHistory : public QObject
{
    Q_OBJECT

private slots:
    void mostRecentFirstWithoutDuplicates()
    {
        CommandHistory history;
        history.add("a");
        history.add("b");
        history.add("a");
        history.add("");
        QCOMPARE(history.items(), (QStringList{"a", "b"}));
    }

    void upDownRestoresDraft()
    {
        CommandHistory history;
        history.setItems({"newest", "older", "oldest"});

        QCOMPARE(history.older("typing"), QString("newest"));
        QCOMPARE(history.older("newest"), QString("older"));
        QCOMPARE(history.older("older"), QString("oldest"));
        QCOMPARE(history.older("oldest"), QString("oldest")); // stays at the end
        QCOMPARE(history.newer("oldest"), QString("older"));
        QCOMPARE(history.newer("older"), QString("newest"));
        QCOMPARE(history.newer("newest"), QString("typing"));
        QCOMPARE(history.newer("typing"), QString("typing")); // already at the draft
    }

    void emptyHistoryKeepsText()
    {
        CommandHistory history;
        QCOMPARE(history.older("abc"), QString("abc"));
        QCOMPARE(history.newer("abc"), QString("abc"));
    }

    void sizeIsLimited()
    {
        CommandHistory history(3);
        for (const char *command : {"1", "2", "3", "4"})
            history.add(command);
        QCOMPARE(history.items(), (QStringList{"4", "3", "2"}));
    }

    void addResetsNavigation()
    {
        CommandHistory history;
        history.setItems({"x", "y"});
        history.older("");
        history.add("z");
        QCOMPARE(history.older(""), QString("z"));
    }
};

QTEST_GUILESS_MAIN(TestCommandHistory)
#include "tst_commandhistory.moc"
