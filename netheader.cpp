
#include "netheader.h"
#include "logqueue.h"
#include <QDebug>
#include <QDateTime>



QUEUE_DATA<MESG> queue_send; //文本，视频发送队列
QUEUE_DATA<MESG> queue_recv; //接收队列
QUEUE_DATA<MESG> audio_recv; //音频接收队列

LogQueue *logqueue = nullptr;

void log_print(const char *filename, const char *funcname, int line, const char *fmt, ...)
{
    Log *log = (Log *) malloc(sizeof (Log));
    if(log == nullptr)
    {
        qDebug() << "malloc log fail";
    }
    else
    {
        memset(log, 0, sizeof (Log));
        log->ptr = (char *) malloc(1 * KB);
        if(log->ptr == nullptr)
        {
            free(log);
            qDebug() << "malloc log.ptr fail";
            return;
        }
        else
        {
            memset(log->ptr, 0, 1 * KB);
            QDateTime current_time = QDateTime::currentDateTime();
            QString time = current_time.toString("yyyy-MM-dd hh:mm:ss ");
            int pos = 0;
            memcpy_s(log->ptr + pos, 1 * KB - 2 - pos, time.data(), time.size());
            pos += time.size();

            int m = snprintf(log->ptr + pos, 1 * KB - 2 - pos, "%s:%s::%d--", filename, funcname, line);
            pos += m;

            va_list ap;
            va_start(ap, fmt);
            vsnprintf(log->ptr + pos, 1 * KB - 2 - pos, fmt, ap);
            strcat_s(log->ptr + pos, 1 * KB - 2 - pos, "\n");
            va_end(ap);
            logqueue->pushLog(log);
        }
    }
}
