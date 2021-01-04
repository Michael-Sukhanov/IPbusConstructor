#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "packetviewer.h"
#include <QtNetwork/QUdpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* , QEvent*);

private:
    QUdpSocket* socket;
    IPbusWord request[maxWordsPerPacket], response[maxWordsPerPacket];
    char* const Crequest = reinterpret_cast<char*>(request), * const Cresponse = reinterpret_cast<char*>(response);
    Ui::MainWindow *ui;
    TransactionType currentType = read;
    QHash<QString, TransactionType> CoresspondingTypes;
    //become true after sending packet
    bool sendFlag = false;

private slots:
    void selectedTransactionChanged(const TransactionType type);
    void packetSizeChanged();
    void nWordsChanged();
    void sendPacket();
    void getResponse();
};
#endif // MAINWINDOW_H
