#include "cmduplugin.h"
#include <QLabel>
#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QPushButton>

CMDUPlugin::CMDUPlugin(QObject *parent) : QObject(parent), m_tipsLabel(new QLabel), m_refershTimer(new QTimer(this)), m_settings("deepin", "LIBCMDU")
{
    i=db=ub=dbt=ubt=dbt1=ubt1=dbt0=ubt0=0;
    m_tipsLabel->setObjectName("cmdu");
    m_tipsLabel->setStyleSheet("color:white; padding:0px 1px;");
    m_refershTimer->setInterval(1000);
    m_refershTimer->start();
    m_centralWidget = new CMDUWidget;
    connect(m_centralWidget, &CMDUWidget::requestUpdateGeometry, [this] { m_proxyInter->itemUpdate(this, pluginName()); });
    connect(m_refershTimer, &QTimer::timeout, this, &CMDUPlugin::updateCMDU);

    // 开机时长
    QProcess *process = new QProcess;
    process->start("systemd-analyze");
    process->waitForFinished();
    QString PO = process->readAllStandardOutput();
    QString SD = PO.mid(PO.indexOf("=") + 1, PO.indexOf("\n") - PO.indexOf("=") - 1);
    startup = "SUT: " + SD;
}

const QString CMDUPlugin::pluginName() const
{
    return "LIBCMDU";
}

const QString CMDUPlugin::pluginDisplayName() const
{
    return "LIBCMDU";
}

void CMDUPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;
    if (m_centralWidget->enabled())
        m_proxyInter->itemAdded(this, pluginName());
}

void CMDUPlugin::pluginStateSwitched()
{
    m_centralWidget->setEnabled(!m_centralWidget->enabled());
    if (m_centralWidget->enabled())
        m_proxyInter->itemAdded(this, pluginName());
    else
        m_proxyInter->itemRemoved(this, pluginName());
}

bool CMDUPlugin::pluginIsDisable()
{
    return !m_centralWidget->enabled();
}

int CMDUPlugin::itemSortKey(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    const QString key = QString("pos_%1").arg(displayMode());
    return m_settings.value(key, 0).toInt();
}

void CMDUPlugin::setSortKey(const QString &itemKey, const int order)
{
    Q_UNUSED(itemKey);

    const QString key = QString("pos_%1").arg(displayMode());
    m_settings.setValue(key, order);
}

QWidget *CMDUPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_centralWidget;
}

QWidget *CMDUPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_tipsLabel;
}

const QString CMDUPlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return "";
}

const QString CMDUPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    QList<QVariant> items;
    items.reserve(1);

    QMap<QString, QVariant> about;
    about["itemId"] = "about";
    about["itemText"] = tr("About");
    about["isActive"] = true;
    items.push_back(about);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;
    return QJsonDocument::fromVariant(menu).toJson();
}

void CMDUPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey);
    Q_UNUSED(checked);
    if(menuId == "about"){
        about();
    }
}

void CMDUPlugin::about()
{
    QMessageBox aboutMB(QMessageBox::NoIcon, "HTYCMDU 3.10", "About\n\nDeepin Linux DDE Dock netspeed plugin.\nAuthor：黄颖\nE-mail: sonichy@163.com\nSource：https://github.com/sonichy/CMDU_DDE_DOCK");
    aboutMB.setIconPixmap(QPixmap(":/icon.png"));
    aboutMB.exec();
}

QString CMDUPlugin::KB(long k)
{
    QString s = "";
    if(k > 999999){
        s = QString::number(k/(1024*1024.0),'f',2) + "GB";
    }else{
        if(k > 999){
            s = QString::number(k/1024.0,'f',2) + "MB";
        }else{
            s = QString::number(k/1.0,'f',2) + "KB";
        }
    }
    return s;
}

QString CMDUPlugin::BS(long b)
{
    QString s = "";
    if(b > 999999999){
        //s = QString("%1").arg(b/(1024*1024*1024.0), 6, 'f', 2, QLatin1Char(' ')) + "GB";
        s = QString::number(b/(1024*1024*1024.0), 'f', 2) + "GB";
    }else{
        if(b > 999999){
            //s = QString("%1").arg(b/(1024*1024.0), 6, 'f', 2, QLatin1Char(' ')) + "MB";
            s = QString::number(b/(1024*1024.0), 'f', 2) + "MB";
        }else{
            if(b>999){
                //s = QString("%1").arg(b/1024.0, 6, 'f', 2, QLatin1Char(' ')) + "KB";
                s = QString::number(b/(1024.0), 'f',2) + "KB";
            }else{
                s = b + "B";
            }
        }
    }
    return s;
}

QString CMDUPlugin::NB(long b)
{
    QString s = "";
    if(b>999999){
        s = QString("%1").arg(b/1024/1024.0, 5, 'f', 1, QLatin1Char(' ')) + "M";
    }
    else if(b>999){
        s = QString("%1").arg(b/1024, 5, 'f', 0, QLatin1Char(' ')) + "K";
    }else{
        s = QString("%1").arg(0, 5, 'f', 0, QLatin1Char(' ')) + "B";
    }
    return s;
}

void CMDUPlugin::updateCMDU()
{
    // 开机
    QFile file("/proc/uptime");
    file.open(QIODevice::ReadOnly);
    QString l = file.readLine();
    file.close();
    QTime t(0,0,0);
    t = t.addSecs(l.left(l.indexOf(".")).toInt());
    QString uptime = "UTM: " + t.toString("hh:mm:ss");

    //内存
    file.setFileName("/proc/meminfo");
    file.open(QIODevice::ReadOnly);
    l = file.readLine();
    long mt = l.replace("MemTotal:","").replace("kB","").replace(" ","").toLong();
    l = file.readLine();
    l = file.readLine();
    long ma = l.replace("MemAvailable:","").replace("kB","").replace(" ","").toLong();
    l = file.readLine();
    l = file.readLine();
    file.close();
    long mu = mt - ma;
    int mp = mu*100/mt;
    QString mem = "MEM: " + QString("%1 / %2 = %3").arg(KB(mu)).arg(KB(mt)).arg(QString::number(mp) + "%");

    // CPU
    file.setFileName("/proc/stat");
    file.open(QIODevice::ReadOnly);
    l = file.readLine();
    QByteArray ba;
    ba = l.toLatin1();
    const char *ch;
    ch = ba.constData();
    char cpu[5];
    long user,nice,sys,idle,iowait,irq,softirq,tt;
    sscanf(ch,"%s%ld%ld%ld%ld%ld%ld%ld",cpu,&user,&nice,&sys,&idle,&iowait,&irq,&softirq);
    tt = user + nice + sys + idle + iowait + irq + softirq;
    file.close();
    QString cusage = "";
    int cp = ((tt-tt0)-(idle-idle0))*100/(tt-tt0);
    if(i>0) cusage = "CPU: " + QString::number(cp) + "%";
    idle0 = idle;
    tt0 = tt;

    // 网速
    file.setFileName("/proc/net/dev");
    file.open(QIODevice::ReadOnly);
    l = file.readLine();
    l = file.readLine();
    dbt1 = ubt1 = 0;
    while(!file.atEnd()){
        l = file.readLine();
        QStringList list = l.split(QRegExp("\\s{1,}"));
        db = list.at(1).toLong();
        ub = list.at(9).toLong();
        dbt1 += db;
        ubt1 += ub;
    }
    file.close();
    QString dss = "";
    QString uss = "";
    if (i > 0) {
        long ds = dbt1 - dbt0;
        long us = ubt1 - ubt0;
        dss = NB(ds) + "/s";
        uss = NB(us) + "/s";
        dbt0 = dbt1;
        ubt0 = ubt1;
    }
    QString netspeed = "↑" + uss + "↓" + dss;
    QString net = "UPB: " + BS(ubt1) + "  " + uss + "\nDNB: " + BS(dbt1) + "  " + dss;

    i++;
    if (i>2) i = 2;

    // 绘制
    m_tipsLabel->setText(startup + "\n" + uptime + "\n" + cusage + "\n" + mem + "\n" + net);
    m_centralWidget->text = netspeed;
    m_centralWidget->mp = mp;
    m_centralWidget->cp = cp;
    m_centralWidget->update();
}
