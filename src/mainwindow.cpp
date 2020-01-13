/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     AaronZhang <ya.zhang@archermind.com>
 *
 * Maintainer: AaronZhang <ya.zhang@archermind.com>
 * Maintainer: Yaobin <yao.bin@archermind.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "devicelistview.h"
#include <QHBoxLayout>
#include "DStackedWidget"

#include "computeroverviewwidget.h"
#include "motherboardwidget.h"
#include "cpuwidget.h"
#include "memorywidget.h"
#include "diskwidget.h"
#include "displayadapterwidget.h"
#include "monitorwidget.h"
#include "audiodevicewidget.h"
#include "networkadapterwidget.h"
#include "bluetoothwidget.h"
#include "camerawidget.h"
#include "keyboardwidget.h"
#include "mousewidget.h"
#include "usbdevicewidget.h"
#include "otherdevicewidget.h"
#include "powerwidget.h"
#include <QStandardItemModel>
#include "otherpcidevice.h"
#include "printerwidget.h"
#include "deviceinfoparser.h"
#include "DApplication"
#include "DApplicationHelper"
#include <QSplashScreen>
#include <DWidgetUtil>
#include <QDir>
#include "DFileDialog"
#include <QDateTime>
#include "DTitlebar"
#include "DSpinner"
#include "cdromwidget.h"
#include <thread>
#include "commondefine.h"
#include "QStatusBar"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QScreen>
#include <QSettings>

#include <QDebug>

DWIDGET_USE_NAMESPACE

QList<ArticleStruct> staticArticles;

MainWindow::MainWindow(QWidget *parent) :
    DMainWindow(parent),
    m_sizeForQSetting(mainWindowMinWidth_,mainWindowMinHeight_)
{
    if(false == DeviceInfoParserInstance.getRootPassword())
    {
        exit(-1);
    }
    setMinimumSize(mainWindowMinWidth_,mainWindowMinHeight_);
    loadSettings();

    initLoadingWidget();

    setCentralWidget(loadingWidget_);

    connect( &DeviceInfoParserInstance, &DeviceInfoParser::loadFinished, this, &MainWindow::showSplashMessage);

    refreshDatabase();

    setAttribute(Qt::WA_AcceptDrops, false);

    statusBar()->setSizeGripEnabled(true);
    statusBar()->hide();

}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::initLoadingWidget()
{
    loadingWidget_ = new DWidget(this);
    DFontSizeManager::instance()->bind( loadingWidget_, DFontSizeManager::T6);

    QVBoxLayout* vly = new QVBoxLayout;
    //vly->setMargin(0);

    vly->addStretch(1);
    auto spinner_ = new DSpinner(loadingWidget_);
    spinner_->setFixedSize(spinnerWidth, spinnerHeight);

    QHBoxLayout* hly1 = new QHBoxLayout;
    hly1->addStretch();
    hly1->addWidget(spinner_);
    hly1->addStretch();

    QHBoxLayout* hly2 = new QHBoxLayout;
    hly2->addStretch();

    loadLabel_ = new DLabel("Loading...", this);

    hly2->addWidget(loadLabel_);
    hly2->addStretch();

    vly->addLayout(hly1);
    vly->addSpacing(5);
    vly->addLayout(hly2);

    vly->addStretch(1);

    spinner_->start();
    loadingWidget_->setLayout(vly);
}

void MainWindow::loadDeviceWidget()
{
    mainWidget_ = new DWidget(this);
    QHBoxLayout* ly = new QHBoxLayout;
    ly->setMargin(0);
    ly->setSpacing(0);
    DApplication::processEvents();

    leftDeviceView_ = new DeviceListView(mainWidget_);

    leftDeviceView_->setFixedWidth(leftDeviceListViewMinWidth_);

    DApplication::processEvents();

    ly->addWidget(leftDeviceView_, leftDeviceListViewMinWidth_);

    DApplication::processEvents();

    rightDeviceInfoWidget_ = new DStackedWidget(mainWidget_);

    DApplication::processEvents();

    addAllDeviceinfoWidget();

    ly->addWidget(rightDeviceInfoWidget_, mainWindowMinWidth_ - leftDeviceListViewMinWidth_);

    DApplication::processEvents();

    mainWidget_->setLayout(ly);

    DApplication::processEvents();

    setCentralWidget(mainWidget_);

    if(loadingWidget_)
    {
        loadingWidget_ = nullptr;
        loadLabel_ = nullptr;
    }

    return;
}

void MainWindow::refreshDeviceWidget()
{
    QString currentDevice = leftDeviceView_->currentDevice();

    QMap<QString, DeviceInfoWidgetBase*> oldWidgetMap;
    std::swap(deviceInfoWidgetMap_, oldWidgetMap);

    addAllDeviceinfoWidget();

    if(  deviceInfoWidgetMap_.contains(currentDevice) )
    {
        rightDeviceInfoWidget_->setCurrentWidget(deviceInfoWidgetMap_[currentDevice]);
    }

    foreach(const QString& widgetName, oldWidgetMap.keys())
    {
        rightDeviceInfoWidget_->removeWidget(oldWidgetMap[widgetName]);
        delete oldWidgetMap[widgetName];
    }

    if(loadingWidget_)
    {
        rightDeviceInfoWidget_->removeWidget(loadingWidget_);
        delete loadingWidget_;
        loadingWidget_ = nullptr;
        loadLabel_ = nullptr;
    }

}

void MainWindow::addAllDeviceinfoWidget()
{
    staticArticles.clear();

    auto overviewWidget = new ComputerOverviewWidget(mainWidget_);
    addDeviceWidget(overviewWidget, "overview.svg");
    addDeviceWidget(new CpuWidget(mainWidget_), "cpu.svg");
    addDeviceWidget(new MotherboardWidget(mainWidget_), "motherboard.svg");
    addDeviceWidget(new MemoryWidget(mainWidget_), "memory.svg");
    addDeviceWidget(new DiskWidget(mainWidget_), "storage.svg");
    addDeviceWidget(new DisplayadapterWidget(mainWidget_), "displayadapter.svg");
    addDeviceWidget(new MonitorWidget(mainWidget_), "monitor.svg");
    addDeviceWidget(new NetworkadapterWidget(mainWidget_), "networkadapter.svg");
    addDeviceWidget(new AudiodeviceWidget(mainWidget_), "audiodevice.svg");

    auto keyboardWidget = new KeyboardWidget(mainWidget_);
    auto mouseWidget = new MouseWidget(mainWidget_);        //提前占用蓝牙键盘鼠标

    addDeviceWidget(new BluetoothWidget(mainWidget_), "bluetooth.svg");
    addDeviceWidget(new OtherPciDeviceWidget(mainWidget_), "otherpcidevices.svg");
    addDeviceWidget(new PowerWidget(mainWidget_), "battery.svg");

    if(firstAdd_ == true)
    {
        leftDeviceView_->addSeperator();
    }

    addDeviceWidget(keyboardWidget, "keyboard.svg");
    addDeviceWidget(mouseWidget, "mouse.svg");
    addDeviceWidget(new PrinterWidget(mainWidget_), "printer.svg");
    addDeviceWidget(new CameraWidget(mainWidget_), "camera.svg");
    addDeviceWidget(new CDRomWidget(mainWidget_), "cdrom.svg");

    if(firstAdd_ == true)
    {
        leftDeviceView_->addSeperator();
    }

    addDeviceWidget(new UsbdeviceWidget(mainWidget_), "usbdevice.svg");
    addDeviceWidget(new OtherDevicesWidget(mainWidget_), "otherdevices.svg");

    overviewWidget->setOverviewInfos(staticArticles);

    if(firstAdd_ == true)
    {
        leftDeviceView_->setFistSelected();
        firstAdd_ = false;
    }


    DApplication::restoreOverrideCursor();

    //leftDeviceView_->setEnabled(true);
}

void MainWindow::addDeviceWidget(DeviceInfoWidgetBase* w,  const QString& icon)
{
    if(w==nullptr||icon.isNull()||icon.isEmpty())
        return;
    QString iconName = icon;
    if(iconName.endsWith(".svg"))
       iconName.remove(".svg");

    if(firstAdd_ == true)
    {
        leftDeviceView_->addDevice(w->getDeviceName(), iconName);
    }

    ArticleStruct overviweInfo;
    if( true == w->getOverViewInfo(overviweInfo) )
    {
        staticArticles.push_back(overviweInfo);
    }

    rightDeviceInfoWidget_->addWidget(w);
    deviceInfoWidgetMap_[w->getDeviceName()] = w;

    DApplication::processEvents();
}


void MainWindow::insertDeviceWidget(int index, DeviceInfoWidgetBase* w)
{
    if(firstAdd_ == true)
    {
        leftDeviceView_->addDevice(w->getDeviceName(), ":images/cpu.svg");
    }

    rightDeviceInfoWidget_->insertWidget(index, w);
    rightDeviceInfoWidget_->setCurrentWidget(w);
    deviceInfoWidgetMap_[w->getDeviceName()] = w;
}

void MainWindow::refresh()
{
    if(refreshing_ == true)
    {
        return;
    }

    if(false == DeviceInfoParserInstance.getRootPassword())
    {
        return;
    }

    leftDeviceView_->setEnabled(false);

    refreshing_ = true;

    initLoadingWidget();

    rightDeviceInfoWidget_->addWidget(loadingWidget_);
    rightDeviceInfoWidget_->setCurrentWidget(loadingWidget_);

    refreshDatabase();
}

void MainWindow::refreshDatabase()
{
    DApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    std::thread thread(
            []()
            {
                DeviceInfoParserInstance.refreshDabase();
            }
    );

    thread.detach();
}


bool MainWindow::exportTo(/*const QString& file, const QString& selectFilter*/)
{
    QString selectFilter;

    static QString saveDir = [](){
        QString dirStr = "./";
        QDir dir( QDir::homePath() + "/Documents/");
        if(dir.exists())
        {
            dirStr = QDir::homePath() + "/Documents/";
        }
        return dirStr;
    }();

    QString file = DFileDialog::getSaveFileName(
                      this,
                      DApplication::translate("Main", "Export"), saveDir + DApplication::translate("Main", "deviceInfo") + \
                      QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") .remove(QRegExp("\\s")) + ".txt", \
                      tr("Text (*.txt);; Doc (*.docx);; Xls (*.xls);; Html (*.html)"), &selectFilter);

    if( file.isEmpty() == true )
    {
        return true;
    }

    QFileInfo fileInfo(file);
    saveDir = fileInfo.absolutePath() + "/";

    if( selectFilter == "Text (*.txt)" )
    {
        QFile textFile( file );
        if( false == textFile.open(QIODevice::WriteOnly))
        {
            return false;
        }

        for(int i = 0; i < leftDeviceView_->count(); ++i)
        {
            QString device = leftDeviceView_->indexString(i);
            if( deviceInfoWidgetMap_.contains(device) )
            {
                deviceInfoWidgetMap_[device]->exportToTxt(textFile);
            }
        }

        textFile.close();

        return true;
    }

    if(selectFilter == "Html (*.html)")
    {
        QFile htmlFile( file );
        if( false == htmlFile.open(QIODevice::WriteOnly))
        {
            return false;
        }

        for(int i = 0; i < leftDeviceView_->count(); ++i)
        {
            QString device = leftDeviceView_->indexString(i);
            if( deviceInfoWidgetMap_.contains(device) )
            {
                deviceInfoWidgetMap_[device]->exportToHtml(htmlFile);
            }
        }

        htmlFile.close();

        return true;
    }

    if(selectFilter == "Doc (*.docx)")
    {
        Docx::Document doc(":/thirdlib/docx/doc_template/template.docx");

        for(int i = 0; i < leftDeviceView_->count(); ++i)
        {
            QString device = leftDeviceView_->indexString(i);
            if( deviceInfoWidgetMap_.contains(device) )
            {
                deviceInfoWidgetMap_[device]->exportToDoc(doc);
            }
        }

        doc.save(file);
        return true;
    }

    if( selectFilter == "Xls (*.xls)")
    {
        QXlsx::Document xlsx;

        DeviceInfoWidgetBase::resetXlsRowCount();

        for(int i = 0; i < leftDeviceView_->count(); ++i)
        {
            QString device = leftDeviceView_->indexString(i);
            if( deviceInfoWidgetMap_.contains(device) )
            {
                deviceInfoWidgetMap_[device]->exportToXls(xlsx);
            }
        }

        xlsx.saveAs(file);
        return true;
    }

    return false;
}

void MainWindow::showDisplayShortcutsHelpDialog()
{
    QRect rect = window()->geometry();
    QPoint pos(rect.x() + rect.width() / 2,
               rect.y() + rect.height() / 2);

    QJsonObject shortcutObj;
    QJsonArray jsonGroups;

    QJsonObject windowJsonGroup;
    windowJsonGroup.insert("groupName", DApplication::translate("Main","System"));
    QJsonArray windowJsonItems;

    QJsonObject shortcutItem;
    shortcutItem.insert("name", DApplication::translate("Main","Show Shortcut Keyboard"));
    shortcutItem.insert("value", "Ctrl+Shift+/");
    windowJsonItems.append(shortcutItem);

    QJsonObject jsonItem;
    jsonItem.insert("name", DApplication::translate("Main","Window Maximize/Minimize"));
    jsonItem.insert("value", "Ctrl+Alt+F");
    windowJsonItems.append(jsonItem);

    QJsonObject closeItem;
    closeItem.insert("name", DApplication::translate("Main","Close"));
    closeItem.insert("value", "Alt+F4");
    windowJsonItems.append(closeItem);

    QJsonObject helpItem;
    helpItem.insert("name", DApplication::translate("Main","Help"));
    helpItem.insert("value", "F1");
    windowJsonItems.append(helpItem);

    QJsonObject copyItem;
    copyItem.insert("name", DApplication::translate("Main","Copy"));
    copyItem.insert("value", "Ctrl+C");
    windowJsonItems.append(copyItem);

    windowJsonGroup.insert("groupItems", windowJsonItems);
    jsonGroups.append(windowJsonGroup);

    QStringList editorKeymaps;

    QJsonObject editorJsonGroup;
    editorJsonGroup.insert("groupName", DApplication::translate("Main","DeviceManager"));
    QJsonArray editorJsonItems;

    QJsonObject exportItem;
    exportItem.insert("name", DApplication::translate("Main","Export"));
    exportItem.insert("value", "Ctrl+E");
    editorJsonItems.append(exportItem);

    QJsonObject refreshItem;
    refreshItem.insert("name", DApplication::translate("Main","Refresh"));
    refreshItem.insert("value", "F5");
    editorJsonItems.append(refreshItem);

    editorJsonGroup.insert("groupItems", editorJsonItems);
    jsonGroups.append(editorJsonGroup);

    shortcutObj.insert("shortcut", jsonGroups);

    QJsonDocument doc(shortcutObj);

    QProcess* shortcutViewProcess = new QProcess();
    QStringList shortcutString;
    QString param1 = "-j=" + QString(doc.toJson().data());
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
}

void MainWindow::windowMaximizing()
{
    if (isMaximized()) {
        showNormal();
    }  else {
        //setWindowState(Qt::WindowMaximized);
        showMaximized();
    }
}

void MainWindow::currentDeviceChanged(const QString& device)
{
    if( false == deviceInfoWidgetMap_.contains(device) )
    {
        return;
    }

    rightDeviceInfoWidget_->setCurrentWidget(deviceInfoWidgetMap_[device]);
    leftDeviceView_->setCurrentDevice(device);
}

void MainWindow::showSplashMessage(const QString& message)
{
    if( message == "finish" )
    {
        if(firstAdd_ == true)
        {
            loadDeviceWidget();
        }
        else
        {
            refreshDeviceWidget();
        }

        refreshing_ = false;
        leftDeviceView_->setEnabled(true);
        return;
    }

    if( loadLabel_ )
    {
        loadLabel_->setText(DApplication::translate("Main", message.toStdString().data()));
    }
}

void MainWindow::saveSettings()
{
    QSettings setting(qApp->organizationName(),qApp->applicationName());
    setting.beginGroup("mainwindow");
    setting.setValue("size",m_sizeForQSetting);
    setting.endGroup();
}

void MainWindow::loadSettings()
{   
    QSettings setting(qApp->organizationName(),qApp->applicationName());
    QSize t_size;
    setting.beginGroup("mainwindow");
    if(setting.contains("size")){
        t_size = setting.value("size").toSize();
        if (t_size.isValid()){
            this->resize(t_size);
        }
    }
    setting.endGroup();
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if(  e->key()==Qt::Key_E )
    {
        Qt::KeyboardModifiers modifiers = e->modifiers();
        if (modifiers != Qt::NoModifier)
        {
            if (modifiers.testFlag(Qt::ControlModifier))
            {
                exportTo();
                return;
            }
        }
    }
    if(  e->key()==Qt::Key_F5 )
    {
        refresh();
        return;
    }
    else if(e->key()==Qt::Key_Question)
    {
        Qt::KeyboardModifiers modifiers = e->modifiers();
        if (modifiers != Qt::NoModifier)
        {
            if (modifiers.testFlag(Qt::ControlModifier))
            {
                showDisplayShortcutsHelpDialog();
                return;
            }
        }
    }
    else if(e->key()==Qt::Key_F)
    {
        Qt::KeyboardModifiers modifiers = e->modifiers();
        if (modifiers != Qt::NoModifier)
        {
            if ( modifiers.testFlag(Qt::ControlModifier) && modifiers.testFlag(Qt::AltModifier) )
            {
                windowMaximizing();
                return;
            }
        }
    }


    return DMainWindow::keyPressEvent(e);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if(this->windowState() == Qt::WindowState::WindowNoState)
    {
        m_sizeForQSetting = this->size();
    }
    DMainWindow::resizeEvent(event);
}
