#ifndef UTF8DECODER_H
#define UTF8DECODER_H

#include <QByteArray>
#include <QString>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringDecoder>
#else
#include <QTextCodec>
#include <QTextDecoder>
#include <memory>
#endif

// Stateful UTF-8 decoder: multi-byte characters split across reads are
// carried over to the next chunk instead of turning into U+FFFD.
class Utf8Decoder
{
public:
    QString decode(const QByteArray &bytes)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        return decoder.decode(bytes);
#else
        return decoder->toUnicode(bytes);
#endif
    }

    void reset()
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        decoder.resetState();
#else
        decoder.reset(QTextCodec::codecForName("UTF-8")->makeDecoder());
#endif
    }

private:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QStringDecoder decoder{QStringDecoder::Utf8};
#else
    std::unique_ptr<QTextDecoder> decoder{QTextCodec::codecForName("UTF-8")->makeDecoder()};
#endif
};

#endif // UTF8DECODER_H
