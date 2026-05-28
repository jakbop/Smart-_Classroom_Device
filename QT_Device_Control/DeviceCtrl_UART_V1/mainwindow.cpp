#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QTimer>
#include <QMessageBox>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->btnLampOn->setEnabled(false);
    ui->btnBeep->setEnabled(false);

    connect(&sp, &QSerialPort::readyRead, this, &MainWindow::recvedData);

    updateSerialPort();

    QTimer* timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &MainWindow::updateSerialPort);

    timer->setInterval(1000);
    timer->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateSerialPort()
{
    // 遍历系统中所有有效串口
    static QStringList oldPortNames;

    QStringList newPortNames;
    auto ports = QSerialPortInfo::availablePorts();

    for(auto& p : ports)
    {
        newPortNames.append(p.portName());
    }

    for(auto& name : newPortNames)
    {
        if(!oldPortNames.contains(name))
            ui->comboSerialPort->addItem(name);
    }

    for(auto& name : oldPortNames)
    {
        if(!newPortNames.contains(name))
            ui->comboSerialPort->removeItem(ui->comboSerialPort->findText(name));
    }

    oldPortNames = newPortNames;
}

void MainWindow::recvedData()
{
    static QByteArray buffer;

    buffer += sp.readAll();

     // 将 buffer 中最后一个 \n 字符之前的部分作为完整的消息进行处理，剩余的部分继续保存在 buffer 中等待下一次接收
     int index;
     while ((index = buffer.indexOf('\n')) != -1) // 查找 \n 字符的位置
     {
         QByteArray data = buffer.left(index); // 获取到完整的一帧数据就解析处理一次

         auto data2 = data.split(' ');
         auto topic = data2[0].split('/');

         if(topic[1] == "sensor")
         {
             if(topic[2] == "Temp")
             {
                auto th = data2[1].split('_');
                if(th.size() >= 2)
                {
                    ui->labelTemp->setText(QString("温度：") + th[0] + QString("°C"));
                    ui->labelHumi->setText(QString("湿度：") + th[1] + QString("%"));
                }
             }
             else if(topic[2] == "Light")
             {
                ui->labelLight->setText(QString("光照强度：") + data2[1]);
             }
         }
         else if(topic[1] == "state")
         {
             if(topic[2] == "Led1")
             {
                if(data2[1] == "0")
                {
                    ui->btnLampOn->setText("开 灯");
                }
                else
                {
                    ui->btnLampOn->setText("关 灯");
                }
             }
             else if(topic[2] == "Buzzer")
             {
                if(data2[1] == "0")
                {
                    ui->btnBeep->setText("报 警");
                }
                else
                {
                    ui->btnBeep->setText("静 音");
                }
             }
         }


         buffer = buffer.mid(index + 1); // 将剩余的部分保存在 buffer 中
      }
}


void MainWindow::on_btnOpenSerialPort_clicked()
{
    if(sp.isOpen())
    {
        sp.close();

        ui->comboSerialPort->setEnabled(true);
        ui->btnOpenSerialPort->setText("打 开");
        ui->btnLampOn->setEnabled(false);
        ui->btnBeep->setEnabled(false);

        return;
    }

    QString portName = ui->comboSerialPort->currentText();

    sp.setPortName(portName);
    sp.setBaudRate(QSerialPort::Baud115200);
    sp.setDataBits(QSerialPort::Data8);
    sp.setParity(QSerialPort::NoParity);
    sp.setStopBits(QSerialPort::OneStop);
    sp.setFlowControl(QSerialPort::NoFlowControl);

    if(sp.open(QIODevice::ReadWrite))
    {
        ui->comboSerialPort->setEnabled(false);
        ui->btnOpenSerialPort->setText("关 闭");
        ui->btnLampOn->setEnabled(true);
        ui->btnBeep->setEnabled(true);
    }
    else
    {
        QMessageBox::critical(this, "提示", "打开串口失败，请检查串口连接是否正常或是否被其他程序占用！");
    }
}

void MainWindow::on_btnLampOn_clicked()
{
//    uint8_t cmd[] = {0xA5, 0x5A, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 50, 0, 0xC3, 0x3C};
//    sp.write((const char*)cmd, sizeof(cmd));

    const char* cmd = nullptr;

    if(ui->btnLampOn->text() == "开 灯")
    {
        cmd = "a";

        ui->btnLampOn->setText("关 灯");
    }
    else
    {
        cmd = "b";

        ui->btnLampOn->setText("开 灯");
    }

    sp.write(cmd);
}

void MainWindow::on_btnBeep_clicked()
{
    const char* cmd = nullptr;

    if(ui->btnBeep->text() == "报 警")
    {
        cmd = "c";

        ui->btnBeep->setText("静 音");
    }
    else
    {
        cmd = "d";

        ui->btnBeep->setText("报 警");
    }

    sp.write(cmd);
}
