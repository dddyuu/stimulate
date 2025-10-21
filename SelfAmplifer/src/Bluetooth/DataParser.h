#ifndef DATAPARSER_H
#define DATAPARSER_H

#include <QObject>
#include <QByteArray>
#include <QVector>
#include <QVariantMap>
#include <QString>

class DataParser : public QObject
{
    Q_OBJECT

public:
    explicit DataParser(QObject* parent = nullptr);

    struct EEGChannelData {
        QVector<int> fp1;  // FP1ͨ������
        QVector<int> fp2;  // FP2ͨ������
        QVector<int> events; // �¼��������

        // ���size()��������ͨ������
        int size() const {
            return 2; // �̶�����2��ͨ����fp1��fp2��
        }

        // ������ݵ���������
        int dataPointCount() const {
            return fp1.size(); // ����ÿ��ͨ�������ݵ�����
        }
    };

    struct ParsedData {
        bool valid;
        QString order_num;
        int battery;
        QByteArray video_data;
        QByteArray eeg_data;  // ԭʼEEG���ݣ�70�ֽڣ�
        EEGChannelData parsed_eeg_data;  // �������EEG����
        bool is_fall;
        QByteArray four_data;
        int gravity_data;
        QByteArray yuliu;
        QByteArray shine;
        int jiaoyan;
        QByteArray rawdata;
        QString payload;  // ʮ�������ַ�����ʾ
    };

    ParsedData parseData(const QByteArray& data);

    // ��Ӿ�̬�����汾������ֱ�ӵ���
    static ParsedData parseDataStatic(const QByteArray& data);

    // ��������ʵ������
    static DataParser& instance();
    // ADC���ֵ��2^23 = 8388608
    const int adcMaxVal = 8388608; // 2^23

    // ��׼��ѹ������
    const double baseVoltage = 4.5; // ��׼��ѹ
    const int eegFP1Magnification = 12; // FP1ͨ���Ŵ���
    const int eegFP2Magnification = 12; // FP2ͨ���Ŵ���
private:
    EEGChannelData parseEEGChannelData(const QByteArray& eegData);
    int parse24BitLittleEndian(const QByteArray& data, int start);
	//����תԭ�뺯��
    int complement2Original(int rawData, int adcMaxVal);
    // ����ת��ѹ����
    double complementToVoltage(int rawData, int adcMaxVal, double baseVoltage, int gain);



};

#endif // DATAPARSER_H