#ifndef ICONS_H
#define ICONS_H

#include <QColor>
#include <QIcon>

// Small line icons drawn with QPainter, so they follow the theme colours
// and need no image files or extra Qt modules.
namespace Icons
{
    enum class Shape
    {
        Connect,
        Disconnect,
        Pause,
        Play,
        Clear,
        Save,
        Image,
        Sun,
        Moon,
        Auto,
        Demo
    };

    QIcon make(Shape shape, const QColor &color);
}

#endif // ICONS_H
