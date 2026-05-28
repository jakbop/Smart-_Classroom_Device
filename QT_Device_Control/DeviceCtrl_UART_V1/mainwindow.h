#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateSerialPort();
    void recvedData();

    void on_btnOpenSerialPort_clicked();

    void on_btnLampOn_clicked();

    void on_btnBeep_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort sp;
};
#endif // MAINWINDOW_H
