#include "packetviewer.h"

packetViewer::packetViewer(QWidget* parent, const QColor* pallete):QTreeWidget(parent),
    transactions(0), packetWords(0){
    this->setAcceptDrops(true);
    this->setDragEnabled(true);
    this->pallete = pallete;
    this->setFont(QFont("Consolas", 10));
    connect(this, &packetViewer::itemChanged, this, [=](QTreeWidgetItem *item, int col){
        Q_UNUSED(col)
        if(item->parent())
            item->setText(0, hexFormatFor(item->text(0)));
    });
}

//add IPbus Header Item in tree
void packetViewer::addIPbusPacketHeader(){
    //Create IPbus Packet Header
    PacketHeader header = PacketHeader(control);
    //Create parent Item with header
    QTreeWidgetItem* headerItem = createNewTreeWidgetItem(nullptr, new QStringList({"IPbus Packet Header", "", "0"}), true, pallete[6]);
    //This flag needed to show, that this item is not editable and drageble, but enabled for watching
    headerItem->setFlags(Qt::ItemIsEnabled);
    //Create child Item with content of header
    createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(header), "0","0"}))->setFlags(Qt::ItemIsEnabled);
    ++packetWords;
}

//add IPbusTransaction to the tree
void packetViewer::addIPbusTransaction(TransactionType type, const quint8 nWords, const IPbusWord address, const IPbusWord ANDterm, const IPbusWord ORterm){
    counter internalTransactionWords = 0;
    TransactionHeader header = TransactionHeader(type, nWords, transactions);
    //Creating transaction item
    QTreeWidgetItem* headerItem = createNewTreeWidgetItem(nullptr,
                                      new QStringList({QString::asprintf("[%u] ", transactions++) + header.typeIDString() + " transaction", "", QString::number(packetWords)}),/*counter transactions go forward*/
                                      true, pallete[type]);
    //    headerItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    /*as it counts amount of transactions*/
    //Creating children items, which are common for all transactions
    createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(header),  QString::number(internalTransactionWords++), QString::number(packetWords++)}))->setFlags(Qt::ItemIsEnabled);  /*counters packet words adn internal words go forward*/
    createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(address), QString::number(internalTransactionWords++), QString::number(packetWords++)}))->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable); /*as here we fill transaction*/
    //Orders what to do while different transactions
    switch (type) {
        case nonIncrementingWrite:
        case write: {for(quint8 i = 0; i < nWords; ++i)
                       createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(QRandomGenerator::global()->generate()),
                                                                            QString::number(internalTransactionWords++),
                                                                            QString::number(packetWords++)}))->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                    break;}
        case RMWsum: { createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(ANDterm),
                                                                            QString::number(internalTransactionWords++),
                                                                            QString::number(packetWords++)}))->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                    break;}
        case RMWbits:{ createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(ANDterm),
                                                                            QString::number(internalTransactionWords++),
                                                                            QString::number(packetWords++)}))->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                       createNewTreeWidgetItem(headerItem, new QStringList({hexFormatFor(ORterm),
                                                                            QString::number(internalTransactionWords++),
                                                                            QString::number(packetWords++)}))->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                    break;}
        case read:
        case nonIncrementingRead:
        default: break;
    }
    emit wordsAmountChanged();
}


//reinit tree widget - make the numeration correct
void packetViewer::reinit(){
    this->transactions = 0; this->packetWords = 1;
    QList<QTreeWidgetItem*> transactions = this->findItems("transaction", Qt::MatchContains);
    if(transactions.isEmpty()) return;
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
    quint16 internalCounter = 0;
    QString previoustext = headerItem->text(0);
    //Change header item text
    setText(headerItem, new QStringList({previoustext.replace(QRegExp("([0-9]{1,2}|1[0-7][0-9]|18[012])"), QString::asprintf("%u", transactionNo)), "", QString::number(packetWordNo)}));
    TransactionHeader header = static_cast<quint32>(headerItem->child(0)->text(0).remove(0,2).toUInt(nullptr, 16));
    header.TransactionID = transactionNo;
    setText(headerItem->child(0), new QStringList({hexFormatFor(header), QString::number(internalCounter++), QString::number(packetWordNo++)}));
    for(quint16 i = 1; i < headerItem->childCount(); ++i)
        setText(headerItem->child(i),new QStringList({headerItem->child(i)->text(0), QString::number(internalCounter++), QString::number(packetWordNo++)}));

}
