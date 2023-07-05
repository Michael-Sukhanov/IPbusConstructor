#include "packetviewer.h"

packetViewer::packetViewer(QWidget* parent, const QColor* pallete):QTreeWidget(parent),
    transactions(0), packetWords(0), expectedWords(0){
    this->setAcceptDrops(true);
    this->setDragEnabled(true);
    this->pallete = pallete;
    this->setFont(QFont("Consolas", 10));
    //to work with ontext menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    //validate text changes in item
    connect(this, &packetViewer::itemChanged, this, [=](QTreeWidgetItem *item, int col){
        if(item->parent() && !col)
            item->setText(0, hexFormatFor(item->text(0)));
    });
    //allow edit word in coloumn 0 but not 1 and 2
    connect(this, &packetViewer::itemDoubleClicked, this, [=](QTreeWidgetItem* item, int col){
            if(!col && item->parent()) item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
            else if(item->parent())item->setFlags(item->flags() == Qt::NoItemFlags ? Qt::NoItemFlags : Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    });
    //copying event handler
    connect(copyShortcut, &QShortcut::activated, [this](){
       QList<QTreeWidgetItem*> selectedItems = itemsSort(this->selectedItems());
       if(selectedItems.size() > 0){
           QString buffer;
           for(quint16 i = 0; i < selectedItems.size(); ++i){
               auto item = selectedItems.at(i);
               buffer.append(item->text(0) +'\n');
           }
           clipboard->setText(buffer);
       }else{
           copyWholePacket();
       }
    });
    connect(this, &QTreeWidget::customContextMenuRequested, this, &packetViewer::preapreMenu);
}

//add IPbus Header Item in tree
void packetViewer::addIPbusPacketHeader(){
    //all counters set to zero as only one packet is able
    this->packetWords = 0, this->transactions = 0; this->expectedWords = 0;
    //Create IPbus Packet Header
    PacketHeader header = PacketHeader(control);
    //Create parent Item with header
    QTreeWidgetItem* headerItem = createNewTreeWidgetItem(nullptr, new QStringList({QString::asprintf("   0x%08X: Packet header", quint32(header)), "", QString::number(this->packetWords++)}), true, pallete[6]);
    //This flag needed to show, that this item is not editable and drageble, but enabled for watching
    headerItem->setFlags(Qt::ItemIsEnabled);
    ++expectedWords;
}

//add IPbusTransaction to the tree
void packetViewer::addIPbusTransaction(TransactionType type, const quint8 nWords, const IPbusWord address, const QVector<quint32>* wordData, const IPbusWord ANDterm, const IPbusWord ORterm){
    counter internalTransactionWords = 0;
    TransactionHeader header = TransactionHeader(type, nWords, transactions);
    //Creating transaction item
    QTreeWidgetItem* headerItem = createNewTreeWidgetItem(nullptr,
                                      new QStringList({QString::asprintf("[%u]0x%08X: %s %u word%s", transactions++, quint32(header),
                                                       header.typeIDString().toUtf8().data(), header.Words, (header.Words % 10 == 1 ? "" : "s")), "", QString::number(packetWords++)}),/*counter transactions go forward*/
                                      true, pallete[type]);                                                                                                                          /*as it counts amount of transactions*/
    //Creating address item, common for all transactions                                                                                                  /*counters packet words and internal words go forward*/
    QTreeWidgetItem* addressItem = createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(address), "addr", QString::number(packetWords++)})); /*as here we fill transaction*/
    addressItem->setForeground(0, Qt::blue);
    addressItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    //response will have mandatory header, but won't have info about address, that's why increase by only one
    this->expectedWords++;
    //Orders what to do while different transactions
    switch (type) {
        case nonIncrementingWrite:
        case write: {for(quint8 i = 0; i < nWords; ++i)
                       createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(wordData ? wordData->at(i) :QRandomGenerator::global()->generate()),
                                                                            QString::number(internalTransactionWords++),
                                                                            QString::number(packetWords++)}))->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                    break;}
        case RMWsum: { createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(ANDterm),
                                                                            "ADD",
                                                                            QString::number(packetWords++)}))->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                    this->expectedWords++;
                    break;}
        case RMWbits:{ createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(ANDterm),
                                                                            "AND",
                                                                            QString::number(packetWords++)}))->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                       createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(ORterm),
                                                                            "OR",
                                                                            QString::number(packetWords++)}))->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                    this->expectedWords++;
                    break;}
        case read:
        case nonIncrementingRead:{
                    this->expectedWords += nWords;
    }
        default: break;
    }
    emit wordsAmountChanged();
}

void packetViewer::displayResponse(IPbusWord * const response, const quint16 size){
    QString erString;
    this->display = true;
    this->transactions = 0; this->packetWords = 0;
    QTreeWidgetItem* packetHeader = createNewTreeWidgetItem(nullptr, new QStringList({QString::asprintf("   0x%08X: Packet header", response[0]), "", QString::number(this->packetWords++)}), true, pallete[6]);
    packetHeader->setFlags(Qt::ItemIsEnabled);
    while(this->packetWords < size){
        quint16 internalTransactionWords = 0;
        TransactionHeader header = TransactionHeader(response[this->packetWords]);
        QTreeWidgetItem* parent;
        if(errorTransaction(header, erString))
            parent = createNewTreeWidgetItem(nullptr, new QStringList({QString::asprintf("[%u]0x%08X: %s", transactions++, quint32(header),
                                                                       erString.toUtf8().data()), "", QString::number(this->packetWords++)}), true, pallete[header.TypeID]);
        else
            parent = createNewTreeWidgetItem(nullptr, new QStringList({QString::asprintf("[%u]0x%08X: %s %u word%s", transactions++, quint32(header),
                                                                                    header.typeIDString().toUtf8().data(), header.Words, (header.Words % 10 == 1 ? "" : "s")), "", QString::number(this->packetWords++)}), true, pallete[header.TypeID]);
        parent->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        switch(header.TypeID){
        case nonIncrementingRead:
        case read: {for(quint16 i = 0; i < header.Words; ++i)
                    createNewTreeWidgetItem(parent, new QStringList({hexFormatFor(response[this->packetWords]), QString::number(internalTransactionWords++), QString::number(this->packetWords++)}))->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                    break;}
        case RMWbits:
        case RMWsum:{if(header.Words) createNewTreeWidgetItem(parent, new QStringList({hexFormatFor(response[this->packetWords]), QString::number(internalTransactionWords++), QString::number(this->packetWords++)}))->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                    break;}
        case write:
        case nonIncrementingWrite:
        default: break;
        }
    }
}


//reinit tree widget - make the numeration correct
void packetViewer::reinit(){
    this->transactions = 0;
    this->packetWords = this->topLevelItem(0) ? 1 : 0;
    this->expectedWords = this->topLevelItem(0) ? 1 : 0;
    QList<QTreeWidgetItem*> transactions = this->findItems("[", Qt::MatchContains);
    if(!transactions.isEmpty())
        for(quint16 i = 0; i < transactions.size(); ++i)
            changeTransactionPosition(transactions.at(i), this->transactions++, this->packetWords);
    emit wordsAmountChanged();
}


//Drop event processor
void packetViewer::dropEvent(QDropEvent *event){
      //getting Dropped items
    QList<QTreeWidgetItem*> kids = this->selectedItems();
    //in case of dropping into children or parent - dropping will be ignored
    if (kids.size() == 0 || event->source() != this) return;
    //getting indexes of first and last indexes of items
    quint16 start = static_cast<quint16>(this->indexFromItem(kids.first()).row());
    quint16 end   = static_cast<quint16>(this->indexFromItem(kids.last()).row());
    QTreeWidgetItem* parent = kids.first()->parent();
    QTreeWidget::dropEvent(event);
        /* get new index */
    quint16 startAfterDropped = static_cast<quint16>(this->indexFromItem(kids.first()).row());
    QTreeWidgetItem* destination = kids.first()->parent();
    //this construction is returning everything back in case of bad hierarchical move or when item goes to first position in tree
    if(parent != destination || !startAfterDropped || destination)
                for(quint16 i = 0 ; i <= end - start; ++i){
                    if(parent)parent->insertChild(start + i,(destination ? destination->takeChild(startAfterDropped) : this->takeTopLevelItem(startAfterDropped)));
                    else this->insertTopLevelItem(start + i, (destination ? destination->takeChild(startAfterDropped) : this->takeTopLevelItem(startAfterDropped)));}
    reinit();
}


//creates new Item and place it in tree. If level of item zero input parent = nullptr. Can be filled and colored according to arguments. returns pointer on created item.
QTreeWidgetItem *packetViewer::createNewTreeWidgetItem(QTreeWidgetItem *parent, QStringList * const list, const bool needToColor, QColor color){
    QTreeWidgetItem* item;
    if(parent) item = new QTreeWidgetItem(parent);
    else       item = new QTreeWidgetItem(this);
    setText(item, list);
    if(needToColor) brushItem(item, color);
    item->setForeground(1, this->unediatble);
    item->setForeground(2, this->unediatble);
    return item;
}

//returns string with IPbus word in Hex readable format
QString packetViewer::hexFormatFor(const IPbusWord word){
    return QString::asprintf("0x%08X", word);
}

//return validated string in case of changing word in Tree widget
QString packetViewer::hexFormatFor(QString word){
    const quint8 usefulPositions = 8;
    if(word.left(2) == "0x")
        word.remove(0,2);
    if(word.size() < usefulPositions)
        word.prepend(QString("0").repeated(usefulPositions - word.size()));
    else if(word.size() > usefulPositions)
            word.remove(0, word.size() - usefulPositions);
    return word.replace(QRegExp("[^0-9a-fA-F]"), "0").toUpper().prepend("0x");
}

//brush an Item in specified color
void packetViewer::brushItem(QTreeWidgetItem * const item, const QColor color){
    for(quint8 i = 0; i < 3; ++i)
        item->setBackground(i, color);
}

//Set text in Items coloumns as it simplier
void packetViewer::setText(QTreeWidgetItem * const item, QStringList * const list){
    for(quint8 i = 0; i < 3; ++i)
        item->setText(i, list->at(i));
}


//changes indexes in transaction after reiniting NEED TO BE CHECKED здесь забыто о сузествовании заголовка как такогого внутри итема
void packetViewer::changeTransactionPosition(QTreeWidgetItem * const headerItem, const counter transactionNo, counter &packetWordNo){
    QString previoustext = headerItem->text(0);
    //Change header item text
    TransactionHeader header = static_cast<quint32>(headerItem->text(0).split('x').at(1).left(8).toUInt(nullptr, 16));
    header.TransactionID = transactionNo;
    setText(headerItem, new QStringList({QString::asprintf("[%u]0x%08X: %s %u word%s", transactionNo, quint32(header),
                                         header.typeIDString().toUtf8().data(), header.Words, (header.Words % 10 == 1 ? "" : "s")), "", QString::number(packetWordNo++)}));
    this->expectedWords++;
    //amount of expected words is changing too
    switch(header.TypeID){
    case nonIncrementingRead:
    case read:{this->expectedWords +=header.Words;
               break;}
    case RMWbits:
    case RMWsum:{this->expectedWords++;
               break;}
    case write:
    case nonIncrementingWrite:
    default: break;
    }
    for(quint16 i = 0; i < headerItem->childCount(); ++i)
        setText(headerItem->child(i),new QStringList({headerItem->child(i)->text(0), headerItem->child(i)->text(1), QString::number(packetWordNo++)}));

}

QList<QTreeWidgetItem *> packetViewer::itemsSort(const QList<QTreeWidgetItem *> Itemlist){
    if(!Itemlist.size()) return Itemlist;
    QList<QTreeWidgetItem *> sortedList;
    quint16 itemIndex = 1;
    while(this->topLevelItem(itemIndex)){
        if(Itemlist.contains(this->topLevelItem(itemIndex))) sortedList.append(topLevelItem(itemIndex));
        for(quint16 i = 0; i < this->topLevelItem(itemIndex)->childCount(); ++i){
            if(Itemlist.contains(this->topLevelItem(itemIndex)->child(i)))
                sortedList.append(this->topLevelItem(itemIndex)->child(i));
        }
        itemIndex++;
    }
    return sortedList;
}

//function which set all selected packet in window to clipboard
void packetViewer::copyWholePacket(){
    quint16 itemIndex = 0;
    QString message;
    message.append(this->topLevelItem(itemIndex++)->text(0).mid(3,10) + '\n');
    while (this->topLevelItem(itemIndex)) {
        message.append(this->topLevelItem(itemIndex)->text(0).mid(3,10) + '\n');
        for(quint16 i = 0; i < this->topLevelItem(itemIndex)->childCount(); ++i)
            message.append(this->topLevelItem(itemIndex)->child(i)->text(0) + '\n');
        itemIndex++;
    }
    clipboard->setText(message);
}

void packetViewer::preapreMenu(const QPoint &pos){
    auto item = this->itemAt(pos);
    if(!item || !item->text(0).contains('[')) return;
    if(this->display && (item->text(0).contains("Writ"))) return;
    if(!this->display && (item->text(0).contains("Rea"))) return;

    QAction *copyDataAction = new QAction(tr("&Copy Data"), this);
    copyDataAction->setStatusTip(tr("Copy data from the packet to the buffer"));
    connect(copyDataAction, &QAction::triggered, [this, item](){
        QString message;
        for(quint16 i = item->child(0)->text(1).contains("addr"); i < item->childCount(); ++i)
            message.append(item->child(i)->text(0) + '\n');
        this->clipboard->setText(message);
    });

    QMenu menu(this);
    menu.addAction(copyDataAction);
    menu.exec(this->mapToGlobal(pos));

}

bool packetViewer::errorTransaction(TransactionHeader header, QString &erInfo){
    switch(header.InfoCode){
    case 4: erInfo = "READ ERROR";
            return true;
    case 5: erInfo = "WRITE ERROR";
            return true;
    case 6: erInfo = "READ TIMEOUT";
            return true;
    case 7: erInfo = "WRITE TIMEOUT";
            return true;
    default: return false;}
}
