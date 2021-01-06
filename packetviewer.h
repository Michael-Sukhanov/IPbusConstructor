#ifndef PACKETVIEWER_H
#define PACKETVIEWER_H

#include <QObject>
#include <QWidget>
#include <QTreeWidget>
#include <QtGui>
#include <QDrag>
#include "IPbusHeaders.h"

const QColor colors[] = {QColor("#D2CEDF"), //read
                         QColor("#519872"), //write
                         QColor("#99B2DD"), //nonIncrementingRead
                         QColor("#758E4F"), //nonIncrementingWrite
                         QColor("#F7F06D"), //RMWbits
                         QColor("#FFB140"), //RMWsum
                         QColor("#FF7070")};//Packet Header


const quint16 maxWordsPerPacket = 368;

using  IPbusWord = quint32;
using  counter = quint16;

class packetViewer : public QTreeWidget
{
    Q_OBJECT
public:
    packetViewer(QWidget *parent = nullptr, const QColor* pallete = colors);
    void addIPbusPacketHeader();
    void addIPbusTransaction(TransactionType type, const quint8 nWords, const IPbusWord address, const IPbusWord ANDterm = 0, const IPbusWord ORterm = 0);
    void displayResponse(IPbusWord * const response, const quint16 size);
    void reinit();

    const QColor* getPallete(){return this->pallete;}
    counter packetSize(){return this->packetWords;}
    counter expextedResponseSize(){return this->expectedWords;}
    counter transactionsAmount(){return this->transactions;}

signals:
    void wordsAmountChanged();

protected:
    void dropEvent(QDropEvent*);
    
private:
    //pallete to color the header items of the tree elements
    const QColor* pallete;
    const QColor unediatble = QColor("#474747");

    //counters to count number of transactions, number of word and number of expected words in response
    counter transactions, packetWords, expectedWords;

    QTreeWidgetItem* createNewTreeWidgetItem(QTreeWidgetItem* parent, QStringList* const list = new QStringList{"???", "???", "???"},
                                             const bool needToColor = false, QColor color = colors[6]);

    QString hexFormatFor(const IPbusWord);
    QString hexFormatFor(QString);

    void brushItem(QTreeWidgetItem* const, const QColor);
    void setText(QTreeWidgetItem* const, QStringList* const);
    void changeTransactionPosition(QTreeWidgetItem* const, const counter transactionNo, counter& packetWordNo);



};

#endif // PACKETVIEWER_H
