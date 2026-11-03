#include "utf8decoder.h"

#include <QtTest>

class TestUtf8Decoder : public QObject
{
    Q_OBJECT

private slots:
    void multiByteSplitAcrossChunks()
    {
        const QByteArray bytes = QString::fromUtf8("zażółć °C").toUtf8();

        for (auto split = 1; split < bytes.size(); ++split)
        {
            Utf8Decoder decoder;
            QString text = decoder.decode(bytes.left(split)); // separate statements: operand order of '+' is unspecified
            text += decoder.decode(bytes.mid(split));
            QCOMPARE(text, QString::fromUtf8("zażółć °C"));
        }
    }

    void resetDropsPartialCharacter()
    {
        Utf8Decoder decoder;
        decoder.decode(QByteArray("\xc5", 1)); // first byte of 'ż'
        decoder.reset();
        QCOMPARE(decoder.decode("abc"), QString("abc"));
    }
};

QTEST_GUILESS_MAIN(TestUtf8Decoder)
#include "tst_utf8decoder.moc"
