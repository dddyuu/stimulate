#ifndef BLINKDETECTOR_H
#define BLINKDETECTOR_H

#include <QList>
#include <QQueue>
#include <QDebug>
#include <cmath>

struct BlinkResult {
    bool is_blink_detected;      // 是否检测到眨眼
    double blink_intensity;       // 眨眼强度（两通道平均）
    int channel;                  // 主导通道 (0=FP1, 1=FP2, 2=两通道)
    double threshold_ratio;       // 阈值比率
    double fp1_intensity;         // FP1通道强度
    double fp2_intensity;         // FP2通道强度
};

class BlinkDetector {
public:
    BlinkDetector(int windowSize = 256, double thresholdMultiplier = 5.0);  // ★ 提高默认阈值到5.0

    void addDataPoint(const QList<double>& data);  // data[0]=FP1, data[1]=FP2
    BlinkResult detectBlink();
    void reset();

    // 参数设置
    void setThresholdMultiplier(double multiplier);
    void setWindowSize(int size);
    void setCorrelationThreshold(double threshold);  // ★ 设置通道相关性阈值

private:
    struct ChannelData {
        QQueue<double> dataWindow;
        double baseline_mean;
        double baseline_std;
    };

    ChannelData fp1_data;
    ChannelData fp2_data;

    int window_size;              // 滑动窗口大小
    double threshold_multiplier;   // 阈值倍数（提高到5.0）
    double correlation_threshold;  // ★ 通道相关性阈值（默认0.6）

    int cooldown_counter;          // 冷却计数器，避免重复检测
    const int COOLDOWN_SAMPLES = 100; // ★ 增加冷却样本数到100

    // 辅助函数
    void updateBaseline(ChannelData& channel);
    double calculateMean(const QQueue<double>& data);
    double calculateStd(const QQueue<double>& data, double mean);
    bool detectBlinkInChannel(const ChannelData& channel, double& intensity);
    double calculateCorrelation(const QQueue<double>& data1, const QQueue<double>& data2, int samples);  // ★ 新增：计算相关性
};

#endif // BLINKDETECTOR_H