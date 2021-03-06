#include "Timer.h"

Timer::Timer(QObject *parent, int timeout, std::function<void(void)> functor) : QTimer(parent)//传入函数封装
{
    setInterval(timeout);
    if (timeout < 50)
        setTimerType(Qt::PreciseTimer);//精确计时器
    setSingleShot(true);
    connect(this, &Timer::timeout, [this, functor] { functor(); deleteLater(); });//绑定timeout信号与lambda函数,一次性函数，用完即析构
}

TimeLine::TimeLine(QObject *parent, int duration, int interval, std::function<void(qreal)> onChanged, std::function<void(void)> onFinished, CurveShape shape)
        : QTimeLine(duration, parent)
{
    if (duration == 0) {
        int i = 1;
        ++i;
    }
    setUpdateInterval(interval);
    setCurveShape(shape);
    connect(this, &TimeLine::valueChanged, onChanged);
    connect(this, &TimeLine::finished, [this, onFinished] { onFinished(); deleteLater(); });
}
