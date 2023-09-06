#ifndef PACKETVIEWER_H
#define PACKETVIEWER_H

#include <QObject>
#include <QWidget>
#include <QTreeWidget>
#include <QtGui>
#include <QDrag>
#include <QAction>
#include <QMenu>
#include <QApplication>
#include <QShortcut>
#include "IPbusHeaders.h"

const QColor colors[] = {QColor("#DDDAE7"), //read
                         QColor("#CAE2D5"), //write
                         QColor("#D7ECFE"), //nonIncrementingRead
                         QColor("#CFDBBD"), //nonIncrementingWrite
                         QColor("#FDFBD8"), //RMWbits
                         QColor("#FFEED6"), //RMWsum
                         QColor("#FFADAD")};//Packet Header


const quint16 maxWordsPerPacket = 368;


using  IPbusWord = quint32;
using  counter = quint16;

class packetViewer : public QTreeWidget
{
    Q_OBJECT
public:
    packetViewer(QWidget *parent = nullptr, const QColor* pallete = colors);
    void addIPbusPacketHeader(IPbusWord h = 0);
    void addIPbusTransaction(TransactionType type, const quint8 nWords, const IPbusWord address, const QVector<quint32>* writeData = nullptr, const IPbusWord ANDterm = 0, const IPbusWord ORterm = 0);
    void showPacket(IPbusWord * const response, const quint16 size, const bool expanded = false, const bool hiddenHeaders = false);

    void expandAllTopLevelItems();
    void collapseAllTopLEvelItems();

    void hideHeaders();

    void reinit();

    const QColor* getPallete(){return this->pallete;}
    counter packetSize(){return this->packetWords;}
    counter expectedResponseSize(){return this->expectedWords;}
    counter transactionsAmount(){return this->transactions;}

signals:
    void wordsAmountChanged();

protected:
    void dropEvent(QDropEvent*);
    
private:
    bool display = false;
    //pallete to color the header items of the tree elements
    const QColor* pallete;
    const QColor unediatble = QColor("#474747");

    QClipboard *clipboard = QApplication::clipboard();

    //counters to count number of transactions, number of word and number of expected words in response
    counter transactions, packetWords, expectedWords;

    QShortcut* copyShortcut = new QShortcut(QKeySequence("Ctrl+C"), this, nullptr, nullptr, Qt::WidgetShortcut);

    QTreeWidgetItem* createNewTreeWidgetItem(QTreeWidgetItem* parent, QStringList* const list = new QStringList{"???", "???", "???"},
                                             const bool needToColor = false, QColor color = colors[6]);

    QString hexFormatFor(const IPbusWord);
    QString hexFormatFor(QString);

    void brushItem(QTreeWidgetItem* const, const QColor);
    void setText(QTreeWidgetItem* const, QStringList* const);
    void changeTransactionPosition(QTreeWidgetItem* const, const counter transactionNo, counter& packetWordNo);

//    void setFlagsAllParents(Qt::ItemFlags flags = Qt::ItemIsEnabled, QTreeWidgetItem* item = nullptr);
//    void setFlagsAllChildren(Qt::ItemFlags flags = Qt::ItemIsEnabled);
//    void restoreAllItemsFlags();

    QList<QTreeWidgetItem*> itemsSort(const QList<QTreeWidgetItem*> Itemlist);
    QList<QTreeWidgetItem*> itemsToHide;
    void copyWholePacket();
    void preapreMenu(const QPoint& pos);
    bool errorTransaction(TransactionHeader header, QString& erInfo);
    QList<QTreeWidgetItem*> getExpandebleItems();



};

#endif // PACKETVIEWER_H
