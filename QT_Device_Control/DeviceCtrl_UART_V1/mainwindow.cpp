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

    this->setWindowFlags(Qt::Window | Qt::MSWindowsFixedSizeDialogHint);

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
        {
            int idx = ui->comboSerialPort->findText(name);
            if(idx >= 0)
                ui->comboSerialPort->removeItem(idx);
        }
    }

    oldPortNames = newPortNames;
}

void MainWindow::recvedData()
{
    static QByteArray buffer;

    buffer += sp.readAll();

     int index;
     while ((index = buffer.indexOf('\n')) != -1)
     {
         QByteArray data = buffer.left(index);

         auto data2 = data.split(' ');
         auto topic = data2[0].split('/');

         if(topic.size() >= 3 && topic[1] == "sensor")
         {
             if(topic[2] == "Temp" && data2.size() >= 2)
             {
                auto th = data2[1].split('_');
                if(th.size() >= 2)
                {
                    ui->labelTemp->setText(th[0] + QString("°C"));
                    ui->labelHumi->setText(th[1] + QString("%"));
                }
             }
             else if(topic[2] == "Light" && data2.size() >= 2)
             {
                ui->labelLight->setText(data2[1]);
             }
         }
         else if(topic.size() >= 3 && topic[1] == "state")
         {
             if(topic[2] == "Led1" && data2.size() >= 2)
             {
                if(data2[1] == "0")
                {
                    ui->btnLampOn->setText("开 灯");
                    ui->btnLampOn->setStyleSheet(
                        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 8px; font-size: 22px; font-weight: bold; }"
                        "QPushButton:hover { background-color: #2ecc71; }"
                        "QPushButton:pressed { background-color: #1e8449; }"
                    );
                }
                else
                {
                    ui->btnLampOn->setText("关 灯");
                    ui->btnLampOn->setStyleSheet(
                        "QPushButton { background-color: #c0392b; color: white; border: none; border-radius: 8px; font-size: 22px; font-weight: bold; }"
                        "QPushButton:hover { background-color: #e74c3c; }"
                        "QPushButton:pressed { background-color: #a93226; }"
                    );
                }
             }
             else if(topic[2] == "Buzzer" && data2.size() >= 2)
             {
                if(data2[1] == "0")
                {
                    ui->btnBeep->setText("报 警");
                    ui->btnBeep->setStyleSheet(
                        "QPushButton { background-color: #e67e22; color: white; border: none; border-radius: 8px; font-size: 22px; font-weight: bold; }"
                        "QPushButton:hover { background-color: #f39c12; }"
                        "QPushButton:pressed { background-color: #d35400; }"
                    );
                }
                else
                {
                    ui->btnBeep->setText("静 音");
                    ui->btnBeep->setStyleSheet(
                        "QPushButton { background-color: #8e44ad; color: white; border: none; border-radius: 8px; font-size: 22px; font-weight: bold; }"
                        "QPushButton:hover { background-color: #9b59b6; }"
                        "QPushButton:pressed { background-color: #7d3c98; }"
                    );
                }
             }
         }

         buffer = buffer.mid(index + 1);
      }
}


void MainWindow::on_btnOpenSerialPort_clicked()
{
    if(sp.isOpen())
    {
        sp.close();

        ui->comboSerialPort->setEnabled(true);
        ui->btnOpenSerialPort->setText("打开串口");
        ui->btnOpenSerialPort->setStyleSheet(
            "QPushButton { background-color: #3498db; color: white; border: none; border-radius: 5px; font-size: 18px; font-weight: bold; }"
            "QPushButton:hover { background-color: #2980b9; }"
            "QPushButton:pressed { background-color: #2471a3; }"
        );
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
        ui->btnOpenSerialPort->setText("关闭串口");
        ui->btnOpenSerialPort->setStyleSheet(
            "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-size: 18px; font-weight: bold; }"
            "QPushButton:hover { background-color: #c0392b; }"
            "QPushButton:pressed { background-color: #a93226; }"
        );
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
    const char* cmd = "b";

    if(ui->btnLampOn->text() == "开 灯")
    {
        cmd = "a";
        ui->btnLampOn->setText("关 灯");
        ui->btnLampOn->setStyleSheet(
            "QPushButton { background-color: #c0392b; color: white; border: none; border-radius: 8px; font-size: 22px; font-weight: bold; }"
            "QPushButton:hover { background-color: #e74c3c; }"
            "QPushButton:pressed { background-color: #a93226; }"
        );
    }
    else
    {
        cmd = "b";
        ui->btnLampOn->setText("开 灯");
        ui->btnLampOn->setStyleSheet(
            "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 8px; font-size: 22px; font-weight: bold; }"
            "QPushButton:hover { background-color: #2ecc71; }"
            "QPushButton:pressed { background-color: #1e8449; }"
        );
    }

    sp.write(cmd);
}

void MainWindow::on_btnBeep_clicked()
{
    const char* cmd = "d";

    if(ui->btnBeep->text() == "报 警")
    {
        cmd = "c";
        ui->btnBeep->setText("静 音");
        ui->btnBeep->setStyleSheet(
            "QPushButton { background-color: #8e44ad; color: white; border: none; border-radius: 8px; font-size: 22px; font-weight: bold; }"
            "QPushButton:hover { background-color: #9b59b6; }"
            "QPushButton:pressed { background-color: #7d3c98; }"
        );
    }
    else
    {
        cmd = "d";
        ui->btnBeep->setText("报 警");
        ui->btnBeep->setStyleSheet(
            "QPushButton { background-color: #e67e22; color: white; border: none; border-radius: 8px; font-size: 22px; font-weight: bold; }"
            "QPushButton:hover { background-color: #f39c12; }"
            "QPushButton:pressed { background-color: #d35400; }"
        );
    }

    sp.write(cmd);
}
