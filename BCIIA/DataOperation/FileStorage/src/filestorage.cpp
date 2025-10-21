#include "filestorage.h"
#include "QFileInfo"
#include "QFileDialog"
#include "QDebug"
#include "QDateTime"
#include "storageconfig.h"
#include "matstorage.h"
#include "QTimer"
#include "QElapsedTimer"
#include <QMutex>
#include <QCoreApplication>
FileStorage* FileStorage::m_instance = nullptr;
QMutex mutex;
QWaitCondition condition;
bool dataReady = false;
bool labelReady = false;

//FileStorage* FileStorage::instance(QObject* parent)
//{
//    static QMutex instanceMutex;
//    QMutexLocker locker(&instanceMutex);
//    if (!m_instance) {
//        m_instance = new FileStorage(parent);
//    }
//    return m_instance;
//}
FileStorage* FileStorage::instance(QObject* /*parent*/)
{
    static QMutex instanceMutex;
    QMutexLocker locker(&instanceMutex);
    if (!m_instance) {
        // 绑定到应用，确保生命周期贯穿整个进程
        m_instance = new FileStorage(qApp);
    }
    return m_instance;
}

FileStorage::FileStorage(QObject* parent) :QObject(parent)
{
    qRegisterMetaType<QList<double>>("QList<double>");
    qRegisterMetaType<QList<QList<double> >>("QList<QList<double>>");
    qRegisterMetaType<QList<QList<double>>>("QList<QList<double>>");
    init();
    //setConnect();
}

FileStorage::~FileStorage()
{
    if (storage != nullptr)
    {
        delete storage;
        storage = nullptr;
    }
    if (elapsedTimer != nullptr)
    {
        delete elapsedTimer;
        elapsedTimer = nullptr;
    }
    m_instance = nullptr;
}

void FileStorage::append(QList<QList<double> > data)
{
    if (start_flag && !pause_flag)
    {
        //原始正常存储
        amplifer_data.append(data);
        // 更新总采样点计数
        if (!data.isEmpty()) {
            totalSampleCount += data[0].size(); // 假设所有通道的采样点数相同
        }
        dataReady = true;
        checkReady();
        if (amplifer_data.bufferFull())
        {
            save();
        }
    }
}

void FileStorage::appendLabel(quint8 label1)
{
    if (start_flag && !pause_flag && elapsedTimer->isValid())
    {
        // 创建标签信息
        LabelInfo labelInfo;
        labelInfo.label = label1;
        labelInfo.timestamp = elapsedTimer->elapsed(); // 获取毫秒时间戳
        labelInfo.samplePoint = totalSampleCount;      // 当前采样点

        labelInfoList.append(labelInfo);
        label.append(label1);

        // 输出调试信息
        qDebug() << "Label appended:"
            << "Label=" << label1
            << "Time=" << labelInfo.timestamp << "ms"
            << "Sample Point=" << labelInfo.samplePoint;

        labelReady = true;
        checkReady();
    }
}

void FileStorage::checkReady()
{
    if (dataReady && labelReady)
    {
        QMutexLocker locker(&mutex);
        condition.wakeOne();  // 唤醒保存操作
    }
}

void FileStorage::initTimer()
{
    timer = new QTimer();
    elapsedTimer = new QElapsedTimer();
    connect(timer, &QTimer::timeout, this, &FileStorage::save);
}

void FileStorage::start()
{
    //QString filename = storage->getFilename();
    QString filename = QFileDialog::getSaveFileName(
        nullptr,
        tr("选择数据保存位置"),
        this->dir,  // 使用配置的默认保存目录作为起始位置
        tr("MAT 文件 (*.mat);;所有文件 (*.*)")
    );

    // 如果用户取消了对话框，直接返回
    if (filename.isEmpty())
    {
        qDebug() << "用户取消了文件选择";
        return;
    }
    // 确保文件扩展名为.mat
    if (!filename.endsWith(".mat", Qt::CaseInsensitive))
    {
        filename += ".mat";
    }
    // 设置选择的文件名到存储对象
    storage->setFilename(filename);

    qDebug() << "开始保存到:" << filename;

    if (!filename.isEmpty())
    {
        qDebug() << "开始保存";
        if (stop_flag)
        {
            start_flag = true;
            pause_flag = false;
            stop_flag = false;
        }
        else
        {
            stop_flag = false;
            pause_flag = true;
            start_flag = true;
        }
        if (pause_flag)
        {
            start_flag = true;
            pause_flag = false;
        }

        // 启动计时器并重置计数器
        elapsedTimer->start();
        totalSampleCount = 0;
        labelInfoList.clear();

        qDebug() << "Timer started, sample rate:" << currentSampleRate;
    }
}

void FileStorage::pause()
{
    pause_flag = true;
    save();
}

void FileStorage::stop()
{
    if (mode == 0)
    {
        timer->stop();
    }
    start_flag = false;
    stop_flag = true;

    // 停止计时器
    if (elapsedTimer->isValid()) {
        qint64 totalTime = elapsedTimer->elapsed();
        qDebug() << "Recording stopped. Total time:" << totalTime << "ms";
        qDebug() << "Total samples recorded:" << totalSampleCount;
    }

    save();
    storage->stop();
}

void FileStorage::setChannelNum(quint8 value)
{
    storage->setChannelNum(value);
}

void FileStorage::setSrate(quint16 rate)
{
    currentSampleRate = rate;
    storage->setSrate(rate);
}

void FileStorage::setChanlocs(QVariantList value)
{
    storage->setChanlocs(value);
}

void FileStorage::setFileName(QString filenane)
{
    storage->setFilename(filenane);
}

void FileStorage::appendEvent(int type)
{
    qDebug() << "getevernt" << type << ":" << amplifer_data.getLen()[0] / 2;
    int len = amplifer_data.getLen()[0];
    storage->appendEvent(type, len / 2);

    // 更新当前事件类型
    currentEventType = type;

    // 特殊处理事件类型 51：开始 3 分钟数据缓存
    if (type == 51) {
        startCaching();
        qDebug() << "Event 51: start caching only (TCP removed)";
    }
    // 特殊处理事件类型 54：停止缓存
    else if (type == 54) {
        cachingActive = false;
        qDebug() << "Event 54: stop caching";
        save();
    }
}

void FileStorage::startCaching()
{
    // 计算 3 分钟需要的采样点数 (3分钟 * 60秒 * 采样率 * 通道数)
    quint64 cacheDurationSamples = 3 * 60 * currentSampleRate * amplifer_data.getSignalsChannelNum()[0];

    // 设置缓存标志和参数
    cachingActive = true;
    cacheStartSampleCount = totalSampleCount;
    cacheTargetSamples = cacheDurationSamples;

    // 清空之前的缓存数据
    cachedData.clear();

    qDebug() << "Event 51 received - Starting 3-minute data caching";
    qDebug() << "Cache duration samples:" << cacheDurationSamples;
    qDebug() << "Current sample count:" << totalSampleCount;
}

void FileStorage::init()
{
    this->start_flag = false;
    this->pause_flag = false;
    this->stop_flag = true;
    this->mode = 1;
    this->totalSampleCount = 0;
    this->currentSampleRate = 500; // 默认采样率500Hz
    this->currentEventType = 0;    // 初始化当前事件类型

    // 初始化缓存相关变量
    this->cachingActive = false;
    this->cacheStartSampleCount = 0;
    this->cacheTargetSamples = 0;
    this->cachedData.clear();

    storage = new MatStorage;
    initTimer();

    //初始化配置
    StorageConfig::init();
    initStorageConfigWidget();
    //获取文件保存目录
    this->dir = StorageConfig::getSavePath();
    quint16 storage_time = StorageConfig::getTime();
    //毫秒级精确度
    this->timer->setTimerType(Qt::PreciseTimer);
    this->timer->setInterval(storage_time * 1000);

    QList<quint8> channel_num = { 2 };
    QList<unsigned int> srate = { 500 }; // 修改为500Hz
    QStringList signals_name = { "eeg" };
    amplifer_data.setSignalsChannelNum(channel_num);
    amplifer_data.setSignalsName(signals_name);
    amplifer_data.setSignalsSrate(srate);
    amplifer_data.setTimeLen(600);
    amplifer_data.alloc_data();
}

void FileStorage::setConnect()
{
    setStorageConnect();
}

void FileStorage::setStorageConnect()
{
    connect(this, &FileStorage::stopSignal, storage, &Storage::stop, Qt::DirectConnection);
    connect(storage, &Storage::saveFinish, this, &FileStorage::saveFinish, Qt::DirectConnection);
    connect(storage, &Storage::mergeMsg, this, &FileStorage::mergeMsg, Qt::DirectConnection);
}

void FileStorage::save()
{
    qDebug() << "Saving" << this->label << "labels";
    qDebug() << "Label info count:" << labelInfoList.size();

    // 输出标签详细信息
    for (const LabelInfo& info : labelInfoList) {
        double timeInSeconds = info.timestamp / 1000.0;
        qDebug() << "Label:" << info.label
            << "Time:" << timeInSeconds << "s"
            << "Sample:" << info.samplePoint;
    }

    qDebug() << this->label;
    storage->save(amplifer_data.getData(), this->label, amplifer_data.getLen(),
        amplifer_data.getSignalsChannelNum(), amplifer_data.getSignalsName());
    amplifer_data.reset();

    // 重置就绪标志
    {
        QMutexLocker locker(&mutex);
        dataReady = false;
        labelReady = false;
    }
}

StorageConfigWidget* FileStorage::getStorageconfigwidget() const
{
    return storageconfigwidget;
}

void FileStorage::initStorageConfigWidget()
{
    storageconfigwidget = new StorageConfigWidget;
}