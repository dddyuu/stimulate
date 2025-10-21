#ifndef BCIMONITOR_H
#define BCIMONITOR_H

#include <QMainWindow>
#include "curvegroup.h"
#include "monitortoolbar.h"
QT_BEGIN_NAMESPACE
namespace Ui { class BCIMonitor; }
QT_END_NAMESPACE
class BCIMonitor : public QMainWindow
{
    Q_OBJECT
public:
    BCIMonitor(QWidget* parent = nullptr);
    ~BCIMonitor();
    void setCurveLabels(QStringList);
    void monitorEnable(bool);
    void appendMark(QString mark);  // 添加公共方法以调用 CurveGroup 的 appendMark

public slots:
    void append(QList<double>);
    // void locallabelFinished(QList<uint8_t>);
signals:
    void startMonitor();
    void stopMonitor();
    void openPulgin();
    void openConnect();
    void startSave();
    void pauseSave();
    void stopSave();
    void decodeSignal();
    void encodeSignal();
private:
    Ui::BCIMonitor* ui;
    void init();
    void setConnect();
    //工具栏
    MonitorToolBar* toolbar;
    void initTool();
    void setToolConnect();
    //状态栏
    QStatusBar* statusbar;
    void initStatus();
    //曲线
    CurveGroup* curvegroup;
    void initCurveGroup();
};
#endif // BCIMONITOR_H
