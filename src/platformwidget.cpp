#include "platformwidget.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>
#include <QElapsedTimer>
#include "BCIIA/UI/src/bcimonitor.h"

PlatFormWidget::PlatFormWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::PlatFormWidget),
    //m_preFcm(25, 2.0),  // 使用25个聚类中心
    m_fcmTrained(false), // 初始化训练状态为false
    m_trainingCounter(0),  // 初始化训练计数器为0
    protocolSender(nullptr),  // 初始化协议发送器指针
    blinkDetector(256, 5.0),
    blinkCount(0)  // 初始化眨眼计数器
{
    ui->setupUi(this);
    initWidget();
    //pre.setSize(512000, 1);
    setConnect();

    // 初始化标签索引
    label_index = 0;
    // 配置眨眼检测器
    blinkDetector.setThresholdMultiplier(4.5);      // 阈值倍数：5.0（更严格）
    blinkDetector.setCorrelationThreshold(0.5);     // 相关性阈值：0.6
    blinkDetector.setWindowSize(256);               // 窗口大小：256
    initProtocolSender();
}

PlatFormWidget::~PlatFormWidget()
{
    delete ui;
    if (protocolSender) {
        delete protocolSender;
    }
}

void PlatFormWidget::initWidget()
{
    this->setWindowFlag(Qt::FramelessWindowHint);
    //mainwidget = new MainWidget;
    //fatiguereswidget = new FatigueResWidget;
    //chart_index = new Chart;
    //mathgame = new MathGame;
    // fileStorage = new FileStorage;
    fileStorage = FileStorage::instance(this->parent());

    // 配置TCP数据转发（作为客户端）
    // fileStorage->setTcpServerAddress("127.0.0.1", 8888);
    // fileStorage->enableTcpForwarding(true);
    // qDebug() << "TCP转发已配置: 127.0.0.1:8888";


    ui->Indexwidget->addWidget("采集", bciia.getMonitorWidget());
    //ui->Indexwidget->addWidget("分析", fatiguereswidget);

    QPalette palette;
    QPixmap pix(":/image/bg.png");
    palette.setBrush(QPalette::Window, pix);
    this->setPalette(palette);
}

void PlatFormWidget::initProtocolSender()
{
    protocolSender = new ProtocolSender(this);

    // TCP服务器，端口8082
    quint16 port = 8082;
    bool started = protocolSender->initTcpServer(port);

    if (started) {
        qDebug() << "TCP服务器初始化成功，监听端口:" << port;
    }
    else {
        qDebug() << "TCP服务器初始化失败";
    }

    connect(protocolSender, &ProtocolSender::connected, this, []() {
        qDebug() << "TCP服务器已启动";
        });
    connect(protocolSender, &ProtocolSender::sendSuccess, this, []() {
        qDebug() << "发送成功";
        });
    connect(protocolSender, &ProtocolSender::sendFailed, this, [](const QString& error) {
        qDebug() << "发送失败:" << error;
        });
}

void PlatFormWidget::resetBlinkDetection()
{
    blinkCount = 0;
    qDebug() << "眨眼检测已重置";
}

int PlatFormWidget::convert_original_label_1(int coarse, int fine) {
    int result = 0;

    if (coarse == 1 && fine == 0) {
        result = 1;
    }
    if (coarse == 1 && fine == 1) {
        result = 2;
    }
    return result;
}

int PlatFormWidget::convert_original_label_2(int coarse, int fine) {
    int result = 0;
    if (coarse == 0 && fine == 0) {
        result = 0;
    }
    if (coarse == 0 && fine == 1) {
        result = 1;
    }
    if (coarse == 1) {
        result = 2;
    }
    return result;
}

void PlatFormWidget::setConnect()
{
    // 在信号槽中使用
    connect(&bciia, &BCIIA::preproDatafinished, this, [=](QList<double> data) {
        // 添加数据点（data[0]=FP1, data[1]=FP2）
        QList<double> fpData = { data[0], data[1] };
        blinkDetector.addDataPoint(fpData);

        sampleCounter++;

        // 每隔一定样本数检测一次
        if (sampleCounter >= DETECT_INTERVAL) {
            auto blinkResult = blinkDetector.detectBlink();

            // 如果检测到眨眼
            if (blinkResult.is_blink_detected) {
                
                QString channelName;
                if (blinkResult.channel == 2) {
                    channelName = "FP1+FP2(双通道)";
                }
                else {
                    channelName = (blinkResult.channel == 0) ? "FP1主导" : "FP2主导";
                }

                // 检查是否是第一次眨眼
                if (blinkCount == 0) {
                    // 第一次眨眼，启动计时器，不发送mark
                    blinkTimer.start();
                    blinkCount = 1;
                    
                    qDebug() << "==========================================";
                    qDebug() << "【检测到第1次眨眼】";
                    qDebug() << "时间:" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
                    qDebug() << "通道:" << channelName;
                    qDebug() << "FP1强度:" << QString::number(blinkResult.fp1_intensity, 'f', 2);
                    qDebug() << "FP2强度:" << QString::number(blinkResult.fp2_intensity, 'f', 2);
                    qDebug() << "等待第2次眨眼 (需在2秒内)...";
                    qDebug() << "==========================================";
                }
                else if (blinkCount == 1) {
                    // 第二次眨眼，检查时间间隔
                    qint64 elapsed = blinkTimer.elapsed();
                    
                    if (elapsed <= BLINK_TIMEOUT_MS) {
                        // 在2秒内检测到第二次眨眼，执行发送刺激和mark
                        qDebug() << "==========================================";
                        qDebug() << "【检测到第2次眨眼 - 满足条件，触发刺激】";
                        qDebug() << "两次眨眼间隔:" << elapsed << "ms";
                        qDebug() << "通道:" << channelName;
                        qDebug() << "FP1强度:" << QString::number(blinkResult.fp1_intensity, 'f', 2);
                        qDebug() << "FP2强度:" << QString::number(blinkResult.fp2_intensity, 'f', 2);
                        qDebug() << "平均强度:" << QString::number(blinkResult.blink_intensity, 'f', 2);
                        qDebug() << "阈值比:" << QString::number(blinkResult.threshold_ratio, 'f', 2);
                        
                        // 使用 QElapsedTimer 进行高精度时间测量
                        QElapsedTimer timer;
                        timer.start();

                        // 记录眨眼检测的基准时间 t0
                        qint64 t0 = QDateTime::currentMSecsSinceEpoch();
                        // 使用 QElapsedTimer 获取精确的经过时间
                        
                        
                        
                        if (protocolSender && protocolSender->isConnected()) {
                            // 使用 QElapsedTimer 获取精确的经过时间
                            //qint64 delta = timer.elapsed();
                            
                            const bool ok = protocolSender->sendStart();
                            qint64 deltaNs = timer.nsecsElapsed();
                            double delta = deltaNs / 1000000.0;  // 
                            //qint64 delta = timer.elapsed();
                                    // 在曲线图上添加眨眼标记
                            auto monitor = qobject_cast<BCIMonitor*>(bciia.getMonitorWidget());
                       
                            // 在曲线图上添加刺激标记（基于 t0 + delta）
                            if (monitor) {
                                QDateTime dt = QDateTime::fromMSecsSinceEpoch(t0);
                                QString ts = dt.toString("yyyy-MM-dd hh:mm:ss.zzz");
                                monitor->appendMark("眨眼 " + ts);
                            }

                            if (monitor) {
                                QDateTime dt2 = QDateTime::fromMSecsSinceEpoch(t0 + delta);
                                QString ts2 = dt2.toString("yyyy-MM-dd hh:mm:ss.zzz");
                                monitor->appendMark(QString("\n\n发送刺激 %1 (延迟: %2ms)").arg(ts2).arg(delta));
                            }
                            
                            
                            
                            if (ok) {
                                qDebug() << "✓ 已发送设备控制开始命令 (0x01)";
                                qDebug() << "  - 发送延迟:" << delta << "ms";
                            }
                            else {
                                qDebug() << "✗ 发送设备控制开始命令失败";
                            }
                        }
                        else {
                            qDebug() << "✗ 无TCP连接，无法发送设备控制开始命令";
                        }
                        
                        // 发射眨眼检测信号
                        emit blinkDetected();

                        // 记录到FileStorage
                        if (fileStorage) {
                            fileStorage->appendEvent(0x0C);
                        }
                        
                        qDebug() << "==========================================";
                        
                        // 重置眨眼计数器
                        resetBlinkDetection();
                    }
                    else {
                        // 超过2秒，重置计数器
                        qDebug() << "==========================================";
                        qDebug() << "【第2次眨眼超时 - 未触发】";
                        qDebug() << "两次眨眼间隔:" << elapsed << "ms (超过" << BLINK_TIMEOUT_MS << "ms)";
                        qDebug() << "重置眨眼检测，当前眨眼作为新的第1次";
                        qDebug() << "==========================================";
                        
                        // 重置后，将当前这次眨眼作为新的第一次眨眼
                        blinkTimer.start();
                        blinkCount = 1;
                    }
                }

            }
            else {
                // 未检测到眨眼，检查是否需要超时重置
                if (blinkCount > 0 && blinkTimer.elapsed() > BLINK_TIMEOUT_MS) {
                    qDebug() << "【眨眼检测超时 - 自动重置】已等待: " << blinkTimer.elapsed() << "ms";
                    resetBlinkDetection();
                }
            }

            sampleCounter = 0;
        }

    });

    connect(ui->Indexwidget, &IndexWidget::closeSingal, this, [=]() {
        this->close();
        QApplication::quit();
        });
    connect(ui->Indexwidget, &IndexWidget::minSingal, this, [=]() {
        this->showMinimized();
        });
    connect(ui->Indexwidget, &IndexWidget::maxSingal, this, [=]() {
        this->showMaximized();
        });
}