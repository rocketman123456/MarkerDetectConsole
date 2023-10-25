#include <QCoreApplication>

#include <QFile>

#include "datapack.h"
#include "camthread.h"
#include "serialthread.h"

CamThread       *_cam_thread[CAM_NUM] = { nullptr };
SerialThread    *_serial_thread = nullptr;
QTimer          *_timer = nullptr;

static const int widths[] = {
    640,
    1280,
};
static const int heights[] = {
    360,
    720,
};
static const int ids[] = {
    4,
    6,
};
static const int frame_width = 640;
static const int frame_height = 480;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // register msg data type
    qRegisterMetaType<FrameMsg>("FrameMsg");
    qRegisterMetaType<FrameMsg>("FrameMsg&");
    qRegisterMetaType<SerialMsg>("SerialMsg&");
    qRegisterMetaType<SerialMsg>("SerialMsg");
    qRegisterMetaType<CamMsg>("CamMsg");
    qRegisterMetaType<CamMsg>("CamMsg&");
    qRegisterMetaType<CamCheckInfo>("CamCheckInfo");
    qRegisterMetaType<CamCheckInfo>("CamCheckInfo&");

    bool state[CAM_NUM] = {false};

    for (int i = 0; i < CAM_NUM; ++i)
    {
        _cam_thread[i] = new CamThread(widths[i], heights[i], ids[i], true, &app);
        // on destory
        QObject::connect(&app, &QCoreApplication::destroyed, _cam_thread[i], &CamThread::StopSlot);
        QObject::connect(_cam_thread[i], &CamThread::finished, _cam_thread[i], &CamThread::deleteLater);
    }

    bool _state = true;
    for (int i = 0; i < CAM_NUM; ++i)
    {
        state[i] = _cam_thread[i]->Init();
        if(!state[i])
            qDebug() << "Cam " << i << " Init Failed.";
        _state = _state & state[i];
    }
    if(!_state)
        exit(1);
    for (int i = 0; i < CAM_NUM; ++i)
    {
        _cam_thread[i]->start();
    }

    QFile file("/home/developer/com_name.txt");
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
       qDebug() << "Can't open the file!";
       exit(1);
    }
    QTextStream in(&file);
    QString line = in.readLine();
    qDebug() << line;

    _serial_thread = new SerialThread(line, false, &app);
    QObject::connect(_serial_thread, &SerialThread::finished, _serial_thread, &SerialThread::deleteLater);
    for (int i = 0; i < CAM_NUM; ++i)
    {
        QObject::connect(_serial_thread, &SerialThread::SerialSignal, _cam_thread[i], &CamThread::ReceiveMsgSlot);
        QObject::connect(_cam_thread[i], &CamThread::CamSignal, _serial_thread, &SerialThread::ReceiveMsgSlot);
        QObject::connect(_serial_thread, &SerialThread::CamCheckSignal, _cam_thread[i], &CamThread::CheckStateSlot);
    }

    _state = _serial_thread->Init();
    if(!_state)
    {
        qDebug() << "Serial Init Failed.";
        exit(1);
    }
    _serial_thread->start();

    bool self_check = true;
    if(self_check)
    {
        // 定时自检程序
        _timer = new QTimer(&app);
        QObject::connect(_timer, &QTimer::timeout, _serial_thread, &SerialThread::CheckResult);
        _timer->start(1*1000*3);
    }

    return app.exec();
}
