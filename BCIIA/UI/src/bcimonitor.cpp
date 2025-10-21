#include "bcimonitor.h"
#include "bcimonitor.h"
#include "ui_bcimonitor.h"
BCIMonitor::BCIMonitor(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::BCIMonitor)
{
    ui->setupUi(this);
    init();
    setConnect();
}
BCIMonitor::~BCIMonitor()
{
    delete ui;
}
void BCIMonitor::setCurveLabels(QStringList labels)
{
    curvegroup->setCurveLabels(labels);
    curvegroup->setCurveNum(labels.size());
}
void BCIMonitor::monitorEnable(bool status)
{
    if (status)
    {
        toolbar->enableAction();
    }
    else
    {
        toolbar->disabledAction();
    }
    
}
void BCIMonitor::append(QList<double> value)
{
    curvegroup->append(value);
}
void BCIMonitor::init()
{
    initTool();
    initStatus();
    initCurveGroup();
}
void BCIMonitor::appendMark(QString mark)
{
    if (curvegroup) {
        curvegroup->appendMark(mark);
    }
}
void BCIMonitor::setConnect()
{
    setToolConnect();
}
void BCIMonitor::initTool()
{
    toolbar = new MonitorToolBar();
    this->addToolBar(toolbar);
}
void BCIMonitor::setToolConnect()
{
    connect(toolbar, &MonitorToolBar::nextSignal, curvegroup, &CurveGroup::next);
    connect(toolbar, &MonitorToolBar::lastSignal, curvegroup, &CurveGroup::last);
    //ͨ��������
    connect(toolbar, &MonitorToolBar::increaseChannelSignal, curvegroup, &CurveGroup::increaseGroupChannel);
    connect(toolbar, &MonitorToolBar::ReduceChannelSignal, curvegroup, &CurveGroup::reduceGroupChannel);
    //y�������
    connect(toolbar, &MonitorToolBar::increasescaleSignal, curvegroup, &CurveGroup::increaseScale);
    connect(toolbar, &MonitorToolBar::ReducescaleSignal, curvegroup, &CurveGroup::reduceScale);
    //x�������
    connect(toolbar, &MonitorToolBar::increasedotSignal, curvegroup, &CurveGroup::increaseXRange);
    connect(toolbar, &MonitorToolBar::ReducedotSignal, curvegroup, &CurveGroup::reduceScale);

    //�ɼ�������
    connect(toolbar, &MonitorToolBar::monitorSignal, this, &BCIMonitor::startMonitor);
    connect(toolbar, &MonitorToolBar::stopmonitorSignal, this, &BCIMonitor::stopMonitor);
    //�ɼ������
    connect(toolbar, &MonitorToolBar::pluginSignal, this, &BCIMonitor::openPulgin);
    //�ɼ�������
    connect(toolbar, &MonitorToolBar::connectSignal, this, &BCIMonitor::openConnect);
    //�ļ�����
    connect(toolbar, &MonitorToolBar::startSignal, this, &BCIMonitor::startSave);
    connect(toolbar, &MonitorToolBar::pauseSignal, this, &BCIMonitor::pauseSave);
    connect(toolbar, &MonitorToolBar::stopSignal, this, &BCIMonitor::stopSave);

    //����
    connect(toolbar, &MonitorToolBar::decodeSignal, this, &BCIMonitor::decodeSignal);
    connect(toolbar, &MonitorToolBar::encodeSignal, this, &BCIMonitor::encodeSignal);
    

}
void BCIMonitor::initStatus()
{
    this->statusbar = new QStatusBar();
    this->setStatusBar(statusbar);
}
void BCIMonitor::initCurveGroup()
{
    curvegroup = new CurveGroup();
    this->setCentralWidget(curvegroup);
}
