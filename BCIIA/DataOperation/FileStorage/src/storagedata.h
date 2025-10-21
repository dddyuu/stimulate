#ifndef STORAGEDATA_H
#define STORAGEDATA_H
#include "vector"
#include "QString"
#include "QList"
#include <QMutex>
#include <QMutexLocker>
namespace File {
	class Data;
}
class File::Data
{
public:
	Data();
	~Data();
	void append(QList<QList<double>>);
    void appendlabel(quint8 label);
	void setSignalsName(QStringList);
	void setSignalsChannelNum(QList<quint8>);
	void setSignalsSrate(QList<unsigned int>);
	void setSingals_label(QStringList);
	void setTimeLen(quint64);
	void alloc_data();
	//�������ݵ�ƫ����
	void reset();
	//�������Ƿ�����
	bool bufferFull();

	//��ȡ
	QList<double*> getData();
	QList<quint64> getLen();
    QList<quint8> getlabel() const;  // �޸�Ϊconst
	QStringList getSignalsName();
	QList<quint8> getSignalsChannelNum();
	QList<unsigned int> getSignalsSrate();
	QStringList getSignalsLabel();
private:

	//���ݴ洢����
	QList<double*> data;
	//���ݶ�������
	QList<double>  data2;
	//���ݵ�ƫ����
	QList<quint64> data_bias;
	void clear();
	//�ź����
	QStringList signals_name;
	//�źŵ�ͨ����
	QList<quint8> signals_channel_num;
	//�źŲ�����
	QList<unsigned int> signals_srate;
	//�źŵı�ǩ
	QStringList signals_label;
	//������������ʱ�䳤��
	quint64 time_len;
    // ��ӻ�����������ǩ
    mutable QMutex labelMutex;  // �޸ĵ�1����ӻ�����
    QList<quint8>label;

};
#endif // !STORAGEDATA_H

