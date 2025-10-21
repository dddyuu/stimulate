#ifndef BLINKDETECTOR_H
#define BLINKDETECTOR_H

#include <QList>
#include <QQueue>
#include <QDebug>
#include <cmath>

struct BlinkResult {
    bool is_blink_detected;      // �Ƿ��⵽գ��
    double blink_intensity;       // գ��ǿ�ȣ���ͨ��ƽ����
    int channel;                  // ����ͨ�� (0=FP1, 1=FP2, 2=��ͨ��)
    double threshold_ratio;       // ��ֵ����
    double fp1_intensity;         // FP1ͨ��ǿ��
    double fp2_intensity;         // FP2ͨ��ǿ��
};

class BlinkDetector {
public:
    BlinkDetector(int windowSize = 256, double thresholdMultiplier = 5.0);  // �� ���Ĭ����ֵ��5.0

    void addDataPoint(const QList<double>& data);  // data[0]=FP1, data[1]=FP2
    BlinkResult detectBlink();
    void reset();

    // ��������
    void setThresholdMultiplier(double multiplier);
    void setWindowSize(int size);
    void setCorrelationThreshold(double threshold);  // �� ����ͨ���������ֵ

private:
    struct ChannelData {
        QQueue<double> dataWindow;
        double baseline_mean;
        double baseline_std;
    };

    ChannelData fp1_data;
    ChannelData fp2_data;

    int window_size;              // �������ڴ�С
    double threshold_multiplier;   // ��ֵ��������ߵ�5.0��
    double correlation_threshold;  // �� ͨ���������ֵ��Ĭ��0.6��

    int cooldown_counter;          // ��ȴ�������������ظ����
    const int COOLDOWN_SAMPLES = 100; // �� ������ȴ��������100

    // ��������
    void updateBaseline(ChannelData& channel);
    double calculateMean(const QQueue<double>& data);
    double calculateStd(const QQueue<double>& data, double mean);
    bool detectBlinkInChannel(const ChannelData& channel, double& intensity);
    double calculateCorrelation(const QQueue<double>& data1, const QQueue<double>& data2, int samples);  // �� ���������������
};

#endif // BLINKDETECTOR_H