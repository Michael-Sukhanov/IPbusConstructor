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
    Ui::MainWindow *ui;
    TransactionType currentType = read;
    QHash<QString, TransactionType> CoresspondingTypes;

private slots:
    void selectedTransactionChanged(const TransactionType type);
    void packetSizeChanged();
    void nWordsChanged();
    void SendPacket();
};
#endif // MAINWINDOW_H
