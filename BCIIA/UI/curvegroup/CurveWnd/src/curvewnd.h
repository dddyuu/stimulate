#ifndef CURVEWND_H
#define CURVEWND_H

#include <QWidget>
//#include <QCustomPlot>
#include <QHBoxLayout>
#include <QTimer>
#include <QMouseEvent>
#include "mark.h"
#include "maxwidget.h"
#include "seriesdata.h"

class CurveWnd : public QWidget
{
    Q_OBJECT
public:
    explicit CurveWnd(QWidget* parent = nullptr);
    ~CurveWnd();

    void setCurveNum(int num);
    void updateCurveNum(int num);
    void setMaxPoint(int num);
    void setCurveLabels(QStringList);
    void updateCurveLabels(QStringList);
    void appendMark(QString);
    void append(QList<double> data);
    void clear();
    void test_chart();
    void increaseScaleFactor();
    void reduceScaleFactor();
    void increaseXRange();
    void reduceXRange();
    int getCurveNum();

    // 获取当前位置
    quint16 getCurrentPos() const { return current_pos; }

    // 新增：从指定位置开始修改通道数据
    void modifyChannelDataAtPosition(int channelIndex, quint16 startPos, int count, double value);

private:
    QCustomPlot* chartview;
    QCPAxis* axis_x, * axis_y;
    QList<SeriesData*> seriesdata;
    int min_y, max_y;
    int max_point_num;
    int curve_num;
    QPen pen;
    QStringList label;
    void initChartView();
    QHBoxLayout* chart_layout;
    void initLayout();
    void update();
    QTimer* timer;
    void initTimer();
    void timerEvent(QTimerEvent* event);
    Mark mark;
    quint16 current_pos;
    Maxwidget* maxwidget;
    void initMaxwidget();
    bool max_status = false;
    quint8 max_id = 0;
    void Maxseries(QMouseEvent* event);
};

#endif // CURVEWND_H