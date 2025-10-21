#include "alphadetector.h"

BlinkDetector::BlinkDetector(int windowSize, double thresholdMultiplier)
    : window_size(windowSize)
    , threshold_multiplier(thresholdMultiplier)
    , correlation_threshold(0.6)  // ★ 默认相关性阈值
    , cooldown_counter(0)
{
    fp1_data.baseline_mean = 0.0;
    fp1_data.baseline_std = 1.0;
    fp2_data.baseline_mean = 0.0;
    fp2_data.baseline_std = 1.0;
}

void BlinkDetector::addDataPoint(const QList<double>& data) {
    if (data.size() < 2) {
        qWarning() << "datano";
        return;
    }

    // 添加FP1数据
    fp1_data.dataWindow.enqueue(data[0]);
    if (fp1_data.dataWindow.size() > window_size) {
        fp1_data.dataWindow.dequeue();
    }

    // 添加FP2数据
    fp2_data.dataWindow.enqueue(data[1]);
    if (fp2_data.dataWindow.size() > window_size) {
        fp2_data.dataWindow.dequeue();
    }

    // 更新基线
    if (fp1_data.dataWindow.size() >= window_size / 2) {
        updateBaseline(fp1_data);
        updateBaseline(fp2_data);
    }

    // 减少冷却计数器
    if (cooldown_counter > 0) {
        cooldown_counter--;
    }
}

BlinkResult BlinkDetector::detectBlink() {
    BlinkResult result;
    result.is_blink_detected = false;
    result.blink_intensity = 0.0;
    result.channel = -1;
    result.threshold_ratio = 0.0;
    result.fp1_intensity = 0.0;
    result.fp2_intensity = 0.0;

    // 如果在冷却期或数据不足，不进行检测
    if (cooldown_counter > 0 || fp1_data.dataWindow.size() < window_size / 2) {
        return result;
    }

    double fp1_intensity = 0.0;
    double fp2_intensity = 0.0;
    bool fp1_blink = detectBlinkInChannel(fp1_data, fp1_intensity);
    bool fp2_blink = detectBlinkInChannel(fp2_data, fp2_intensity);

    result.fp1_intensity = fp1_intensity;
    result.fp2_intensity = fp2_intensity;

    // ★★★ 关键修改：必须两个通道都检测到眨眼 ★★★
    if (fp1_blink || fp2_blink) {
        // ★ 计算两通道最近样本的相关性
        int recent_samples = qMin(30, fp1_data.dataWindow.size());
        double correlation = calculateCorrelation(fp1_data.dataWindow, fp2_data.dataWindow, recent_samples);

        qDebug() << "两通道检测到信号 - FP1强度:" << fp1_intensity
            << "FP2强度:" << fp2_intensity
            << "相关性:" << correlation;

        // ★ 只有当两通道相关性足够高时才认为是真正的眨眼
        if (correlation > correlation_threshold) {
            result.is_blink_detected = true;

            // 计算平均强度
            result.blink_intensity = (fp1_intensity + fp2_intensity) / 2.0;

            // 判断主导通道
            if (std::abs(fp1_intensity - fp2_intensity) < fp1_intensity * 0.3) {
                result.channel = 2;  // 两通道均衡
            }
            else if (fp1_intensity > fp2_intensity) {
                result.channel = 0;  // FP1主导
            }
            else {
                result.channel = 1;  // FP2主导
            }

            result.threshold_ratio = result.blink_intensity;

            // 设置冷却期
            cooldown_counter = COOLDOWN_SAMPLES;

            qDebug() << "★ 确认眨眼事件 - 平均强度:" << result.blink_intensity
                << "相关性:" << correlation;
        }
        else {
            qDebug() << "两通道相关性不足，判定为噪声 - 相关性:" << correlation;
        }
    }
    else if (fp1_blink || fp2_blink) {
        // 只有一个通道检测到，记录日志但不触发
        qDebug() << "单通道检测到信号（未触发）- FP1:" << fp1_blink
            << "(" << fp1_intensity << ") FP2:" << fp2_blink
            << "(" << fp2_intensity << ")";
    }

    return result;
}

void BlinkDetector::updateBaseline(ChannelData& channel) {
    if (channel.dataWindow.isEmpty()) {
        return;
    }

    channel.baseline_mean = calculateMean(channel.dataWindow);
    channel.baseline_std = calculateStd(channel.dataWindow, channel.baseline_mean);

    // 避免标准差为0
    if (channel.baseline_std < 0.001) {
        channel.baseline_std = 0.001;
    }
}

double BlinkDetector::calculateMean(const QQueue<double>& data) {
    if (data.isEmpty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const double& value : data) {
        sum += value;
    }
    return sum / data.size();
}

double BlinkDetector::calculateStd(const QQueue<double>& data, double mean) {
    if (data.size() < 2) {
        return 0.0;
    }

    double sum_squared_diff = 0.0;
    for (const double& value : data) {
        double diff = value - mean;
        sum_squared_diff += diff * diff;
    }

    return std::sqrt(sum_squared_diff / (data.size() - 1));
}

bool BlinkDetector::detectBlinkInChannel(const ChannelData& channel, double& intensity) {
    if (channel.dataWindow.size() < 10) {
        return false;
    }

    // 获取最近的几个样本点（眨眼是瞬时事件）
    int recent_samples = qMin(20, channel.dataWindow.size());
    double max_value = -1e9;
    double min_value = 1e9;

    // 将 QQueue 转换为 QVector
    QVector<double> windowVec;
    windowVec.reserve(channel.dataWindow.size());
    for (const double& value : channel.dataWindow) {
        windowVec.append(value);
    }

    // 从末尾获取最近的样本
    int start_idx = qMax(0, windowVec.size() - recent_samples);
    for (int i = start_idx; i < windowVec.size(); i++) {
        max_value = qMax(max_value, windowVec[i]);
        min_value = qMin(min_value, windowVec[i]);
    }

    // 计算峰峰值
    double peak_to_peak = max_value - min_value;

    // 计算相对于基线标准差的倍数
    intensity = peak_to_peak / channel.baseline_std;

    // 眨眼通常会产生超过阈值的信号
    // 同时检查是否有明显的负向偏移（眨眼特征）
    double current_value = windowVec.last();
    double z_score = std::abs(current_value - channel.baseline_mean) / channel.baseline_std;

    // ★ 提高阈值要求
    return (intensity > threshold_multiplier && z_score > threshold_multiplier * 0.8);
}

// ★ 新增：计算两个通道的相关性
double BlinkDetector::calculateCorrelation(const QQueue<double>& data1, const QQueue<double>& data2, int samples) {
    if (data1.size() < samples || data2.size() < samples) {
        return 0.0;
    }

    // 转换为 QVector
    QVector<double> vec1, vec2;
    for (const double& value : data1) {
        vec1.append(value);
    }
    for (const double& value : data2) {
        vec2.append(value);
    }

    // 获取最近的samples个样本
    int start_idx = qMax(0, vec1.size() - samples);

    // 计算均值
    double mean1 = 0.0, mean2 = 0.0;
    for (int i = start_idx; i < vec1.size(); i++) {
        mean1 += vec1[i];
        mean2 += vec2[i];
    }
    mean1 /= samples;
    mean2 /= samples;

    // 计算协方差和标准差
    double cov = 0.0, std1 = 0.0, std2 = 0.0;
    for (int i = start_idx; i < vec1.size(); i++) {
        double diff1 = vec1[i] - mean1;
        double diff2 = vec2[i] - mean2;
        cov += diff1 * diff2;
        std1 += diff1 * diff1;
        std2 += diff2 * diff2;
    }

    std1 = std::sqrt(std1);
    std2 = std::sqrt(std2);

    // 避免除以0
    if (std1 < 1e-10 || std2 < 1e-10) {
        return 0.0;
    }

    // 计算相关系数
    double correlation = cov / (std1 * std2);

    return correlation;
}

void BlinkDetector::reset() {
    fp1_data.dataWindow.clear();
    fp2_data.dataWindow.clear();
    cooldown_counter = 0;
}

void BlinkDetector::setThresholdMultiplier(double multiplier) {
    threshold_multiplier = multiplier;
    qDebug() << "阈值倍数设置为:" << multiplier;
}

void BlinkDetector::setWindowSize(int size) {
    window_size = size;
    reset();
}

void BlinkDetector::setCorrelationThreshold(double threshold) {
    correlation_threshold = threshold;
    qDebug() << "相关性阈值设置为:" << threshold;
}