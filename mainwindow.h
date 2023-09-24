#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include "packetviewer.h"
#include "writedata.h"
#include <QtNetwork/QUdpSocket>
#include <QSettings>

const QRegExp IP("(0?0?[1-9]|0?[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.(([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.){2}(0?0?[1-9]|0?[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])");

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
	IPbusWord request[maxWordsPerPacket] = {0}, response[maxWordsPerPacket] = {0};
    char* const Crequest = reinterpret_cast<char*>(request), * const Cresponse = reinterpret_cast<char*>(response);
    quint16 responseSize = 0;
    Ui::MainWindow *ui;
    QVector<quint32> writeData;
    TransactionType currentType = read;
    QHash<QString, TransactionType> coresspondingTypes;
    //become true after sending packet
    void setMultiMode(bool);
    void setMask(quint32);
    void makeRequestFromArray(const quint16 requestSize);

    bool sendFlag = false;
    bool expanded = false;
    bool hiddenHeaders = false;
    bool multiMode = false;
    quint32 moduleMask = 0x0;
    quint8 amountOfActiveModules();

private slots:

    void selectedTransactionChanged(const TransactionType type);
    void packetSizeChanged();
    void changeProgressBar(QProgressBar* const bar, const quint16 value);

    void nWordsChanged();
	void preparePacket();
	void sendPacket();
    void getResponse();
    void clear();

    void getConfiguration();
    void saveConfiguration();

    void on_checkBox_expandAll_clicked();
    void on_checkBox_showHeaders_clicked(bool);
    void on_MultipleTransactionsMode_clicked(bool);
};
#endif // MAINWINDOW_H
