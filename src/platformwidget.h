#ifndef PLATFORMWIDGET_H
#define PLATFORMWIDGET_H

#include <QWidget>
#include <vector> 
#include <QElapsedTimer>

#include "ui_platformwidget.h"

#include "bciia.h"

#include "filestorage.h"

#include "alphadetector.h"
#include "protocolsender.h"  // 包含协议发送器
#include "alphadetector.h"
namespace Ui {
    class PlatFormWidget;
}

class PlatFormWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlatFormWidget(QWidget* parent = nullptr);
    ~PlatFormWidget();
signals:
    void result_send(quint8 result, uint8_t localresult);
    void resultsave_send(QList<uint8_t> resultsave);
    void blinkDetected();
private:
    Ui::PlatFormWidget* ui;

    // 主页
    //MainWidget* mainwidget;

    // 采集
    BCIIA bciia;

    // 结果
    //FatigueResWidget* fatiguereswidget;
    //MathGame* mathgame;
    //registerInfo* regis;
    //Chart* chart_index;
    FileStorage* fileStorage;
    void initProtocolSender();
	// EEG预处理
    // ★ 协议发送器（仅串口）
    ProtocolSender* protocolSender;


    int label_index;

    int coarse_predict_1;
    int fine_predict_1;
    int coarse_predict_2;
    int fine_predict_2;
    int origin_predict1;
    int origin_predict2;
    int convert_original_label_1(int coarse, int fine);
    int convert_original_label_2(int coarse, int fine);


    BlinkDetector blinkDetector;  
    int sampleCounter = 0;
    const int DETECT_INTERVAL = 10;  // 每10个样本检测一次

    // 连续眨眼检测相关
    int blinkCount = 0;                    // 眨眼计数器
    QElapsedTimer blinkTimer;              // 眨眼间隔计时器
    const qint64 BLINK_TIMEOUT_MS = 2000;  // 2秒超时



    // FCM训练状态
    bool m_fcmTrained;                         // 训练完成标志
    int m_trainingCounter;                     // 训练数据计数器
    std::vector<std::vector<float>> m_trainingData; // 训练数据存储

    // 私有方法
    void initWidget();
    void setConnect();
    void resetBlinkDetection();  // 重置眨眼检测状态
    //void initFcmCenters();  // 初始化FCM聚类中心
    //void saveFcmCenters();  // 保存FCM聚类中心
    QList<uint8_t>xx1;
    QList<uint8_t>saveResult;
    int is_ceived = 0;
    int localLabelIndex;

};

#endif // PLATFORMWIDGET_H
