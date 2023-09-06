#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow){
    ui->setupUi(this);
    setWindowTitle(QCoreApplication::applicationName() + " v" + QCoreApplication::applicationVersion());
    coresspondingTypes["WRITE"]   = write;
    coresspondingTypes["READ"]    = read;
    coresspondingTypes["NIREAD"]  = nonIncrementingRead;
    coresspondingTypes["NIWRITE"] = nonIncrementingWrite;
    coresspondingTypes["RMWSUM"]  = RMWsum;
    coresspondingTypes["RMWBITS"] = RMWbits;
    socket = new QUdpSocket(this);

    writedata* window = new writedata(this);

    //find all radioButtons to connect them with onr slot
    QList<QRadioButton*> radioButtons = ui->centralwidget->findChildren<QRadioButton*>(QRegularExpression("radioButton_*"));
    foreach (QRadioButton* but, radioButtons)
        connect(but, &QRadioButton::clicked, this, [but, this](){selectedTransactionChanged(coresspondingTypes[but->objectName().remove("radioButton_")]);});

    QList<QPushButton *> maskButtons = ui->centralwidget->findChildren<QPushButton*>(QRegularExpression("pushButton_([01][0-9]|20)"));
    foreach(QPushButton* but, maskButtons)
        connect(but, &QPushButton::clicked, this, [but, this](){maskChanged(but);});
    //Adding transaction
    connect(ui->pushButton_ADD, &QPushButton::clicked, ui->treeWidget_REQUEST, [=](){
        if(!ui->checkBox_RANDOMIZE_DATA->isChecked() && (ui->radioButton_WRITE->isChecked() || ui->radioButton_NIWRITE->isChecked())){
            window ->show();
            return;}
        //in case packet was sent recently we need to clear both trees
        if(!ui->treeWidget_REQUEST->topLevelItem(0)){
            ui->pushButton_SEND->setEnabled(true);
            ui->treeWidget_REQUEST->addIPbusPacketHeader();}
        const IPbusWord address = ui->lineEdit_ADDRESS->text().toUInt(nullptr, 16);
        const quint8     nWords = static_cast<quint8>(ui->lineEdit_NWORDS->text().toUInt(nullptr, 10));
        const IPbusWord andTerm = ui->lineEdit_ANDTERM->text().toUInt(nullptr, 16);
        const IPbusWord  orTerm = ui->lineEdit_ORTERM->text().toUInt(nullptr, 16);
        if(multiMode){
            for(size_t i = 0; i < 21; i++)
               if(moduleMask & 1 << i) ui->treeWidget_REQUEST->addIPbusTransaction(currentType, nWords, address + 0x200 * i, nullptr, andTerm, orTerm);
        }else
            ui->treeWidget_REQUEST->addIPbusTransaction(currentType, nWords, address, nullptr, andTerm, orTerm);
        nWordsChanged();});
    //progress bar processing
    connect(ui->treeWidget_REQUEST, &packetViewer::wordsAmountChanged, this, &MainWindow::packetSizeChanged);
    //reaction on nWords changing as packet size is restricted by MTU
    connect(ui->lineEdit_NWORDS, &QLineEdit::textEdited, this, &MainWindow::nWordsChanged);
    //processing of the response from server to see it in tree widget
    connect(socket, &QUdpSocket::readyRead, this, &MainWindow::getResponse);
    //building packet after send button pushed
    connect(ui->pushButton_SEND, &QPushButton::clicked, this, &MainWindow::sendPacket);
    //clear outputs
    connect(ui->pushButton_CLEAR, &QPushButton::clicked, this, &MainWindow::clear);
    //value for read from writeData window
    connect(window, &writedata::valueReady, [this](quint32 value){this->writeData.append(value);});
    //write Data window ok clicked
    connect(window, &writedata::editingFinished, [this](){
        if(!ui->treeWidget_REQUEST->topLevelItem(0)){
            ui->pushButton_SEND->setEnabled(true);
            ui->treeWidget_REQUEST->addIPbusPacketHeader();}
        if(ui->treeWidget_REQUEST->expectedResponseSize() == maxWordsPerPacket) return;
        const quint16 leftSpace = maxWordsPerPacket - ui->treeWidget_REQUEST->packetSize() - 2;
        const quint8 nWords = leftSpace > this->writeData.size() ? this->writeData.size() : leftSpace;
        const IPbusWord address = ui->lineEdit_ADDRESS->text().toUInt(nullptr, 16);
        if(multiMode){
            for(size_t i = 0; i < 21; i++)
               if(moduleMask & 1 << i) ui->treeWidget_REQUEST->addIPbusTransaction(currentType, nWords, address + 0x200 * i, &this->writeData);
        }else
            ui->treeWidget_REQUEST->addIPbusTransaction(currentType, nWords, address, &this->writeData);
        nWordsChanged();
//        ui->treeWidget_PACKET_VIEWER->addIPbusTransaction(currentType, nWords, address, &this->writeData);
//        nWordsChanged();
        this->writeData.clear();});

    ui->lineEdit_NWORDS->setValidator(new QRegExpValidator(QRegExp("[1-9]|[1-9][0-9]|1[0-9]{1,2}|2[0-4][0-9]|25[0-5]")));
    //validator for IP address
    ui->lineEdit_IPADDRESS->setValidator(new QRegExpValidator(IP));
    //installation of eventFilter for delete button
    ui->treeWidget_REQUEST->installEventFilter(this);

    //resize width of coloumns in appropriate form in the beginning of the program
    ui->treeWidget_REQUEST->header()->resizeSection(0, 380);
    ui->treeWidget_RESPONSE->header()->resizeSection(0, 380);
    ui->treeWidget_REQUEST->header()->resizeSection(1, 40);
    ui->treeWidget_RESPONSE->header()->resizeSection(1, 40);
    ui->treeWidget_REQUEST->header()->resizeSection(2, 40);
    ui->treeWidget_RESPONSE->header()->resizeSection(2, 40);


    //setting font for bars -- kostyl
    ui->progressBar_WORDS->setFont(QFont("FranklinGothic", 12));
    ui->progressBar_WORDS_EXPECTED->setFont(QFont("FranklinGothic", 12));

    //hiding in initialisation
    ui->lineEdit_ANDTERM->hide();
    ui->lineEdit_ORTERM->hide();
    ui->label_4->setText("");
    ui->label_ANDTERM->setText("");
    ui->checkBox_RANDOMIZE_DATA->hide();

    getConfiguration();
    //clearSequence(response);
    //clearSequence(request);

    nWordsChanged();

    //if the window is empty -> nothing to send
    ui->pushButton_SEND->setEnabled(ui->treeWidget_REQUEST->topLevelItem(0));
}

bool MainWindow::eventFilter(QObject * obj, QEvent* event){
    if (obj == ui->treeWidget_REQUEST){
        if (event->type() == QEvent::KeyPress){
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Delete){
                if(ui->treeWidget_REQUEST->itemAbove(ui->treeWidget_REQUEST->currentItem()) && !ui->treeWidget_REQUEST->currentItem()->parent()){
                    for(quint16 i = 0; i < ui->treeWidget_REQUEST->currentItem()->childCount(); ++i)
                        delete ui->treeWidget_REQUEST->currentItem()->child(i);
                    if(ui->treeWidget_REQUEST->transactionsAmount() == 1){
                        delete ui->treeWidget_REQUEST->topLevelItem(0);
                        ui->pushButton_SEND->setEnabled(false);
                    }
                    delete ui->treeWidget_REQUEST->currentItem();
                    ui->treeWidget_REQUEST->reinit();
                    nWordsChanged();
                    return true;
                }else{
                    return false;
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::setMultiMode(bool on)
{
    foreach (QPushButton* but, ui->centralwidget->findChildren<QPushButton*>(QRegularExpression("pushButton_([01][0-9]|20)")))
        but->setEnabled(on);
    ui->label_A_modules->setEnabled(on);
    ui->label_C_modules->setEnabled(on);
    ui->label_TCM->setEnabled(on);
    ui->pushButton_ADD->setEnabled(!multiMode || (moduleMask && multiMode));
    ui->lineEdit_ADDRESS->setInputMask(on ? ">HHH" : ">HHHHHHHH");
    ui->lineEdit_ADDRESS->setValidator(on ? new QRegExpValidator(QRegExp("([10]?[0-9A-F]?[0-9A-F]?)"/*"|([0-9A-F][0-9A-F])|([0-9A-F])"*/)) : new QRegExpValidator(nullptr));
    nWordsChanged();
}

void MainWindow::setMask(quint32 mask)
{
    for(size_t i = 0; i < 21; ++i){
        QPushButton* but = ui->centralwidget->findChild<QPushButton*>("pushButton_" + QString::asprintf("%02d", i));
        if(but) but->setChecked(mask & 1 << i);
    }
    nWordsChanged();
}

void MainWindow::makeRequestFromArray(const quint16 requestSize)
{
 ui->treeWidget_REQUEST->addIPbusPacketHeader(request[0]);
 quint16 reqWordsCounter = 1, shift = 0;
 while(reqWordsCounter != requestSize){
     TransactionHeader tr(request[reqWordsCounter]);
     TransactionType type = TransactionType(tr.TypeID);
     QVector<quint32> writeData = {};
     quint32 address = request[reqWordsCounter + 1], ANDTerm = 0, ORTerm = 0;
     switch(type){
        case write:
        case nonIncrementingWrite:{
            for(size_t i = reqWordsCounter + 2; i < reqWordsCounter + 2 + tr.Words; ++i)
                writeData.append(request[i]);
            shift = 2 + tr.Words;
            break;
     }
        case RMWbits:{
            ANDTerm = request[reqWordsCounter + 2];
            ORTerm = request[reqWordsCounter + 3];
            shift = 4;
            break;
     }
        case RMWsum:{
            ANDTerm = request[reqWordsCounter + 2];
            shift = 3;
            break;
     }
        case read:
        case nonIncrementingRead:{
            shift = 2;
            break;
     }
        default: break;
     }
     ui->treeWidget_REQUEST->addIPbusTransaction(type, tr.Words, address, &writeData, ANDTerm, ORTerm);
     writeData.clear();
     reqWordsCounter += shift;
 }

}

quint8 MainWindow::amountOfActiveModules()
{
    quint32 tmp = moduleMask;
    quint8 retValue = 0;
    while(tmp){
        tmp &= (tmp - 1);
        ++retValue;
    }
    return retValue;
}

MainWindow::~MainWindow(){
    saveConfiguration();
    delete ui;
}

void MainWindow::selectedTransactionChanged(const TransactionType type){
    currentType = type;
    ui->checkBox_RANDOMIZE_DATA->setVisible(type == write || type == nonIncrementingWrite);
    ui->lineEdit_ORTERM ->setVisible (type == RMWbits);
    ui->lineEdit_ANDTERM->setVisible (type == RMWbits || type == RMWsum);
    ui->lineEdit_NWORDS ->setDisabled(type == RMWbits || type == RMWsum);
    ui->lineEdit_NWORDS ->setText   ((type == RMWbits || type == RMWsum) ? "1": ui->lineEdit_NWORDS->text());
    ui->label_4         ->setText    (type == RMWbits ? "OR term:" : "");
    ui->label_ANDTERM   ->setText    (type == RMWsum  ? "ADDEND"   : (type == RMWbits ? "AND term:" : ""));
    //checking if packet is possible to send
    nWordsChanged();
}

void MainWindow::packetSizeChanged(){
    const counter newAmount = ui->treeWidget_REQUEST->packetSize(), expectedAmount = ui->treeWidget_REQUEST->expectedResponseSize();
    QProgressBar* request = ui->progressBar_WORDS, *response = ui->progressBar_WORDS_EXPECTED;
    changeProgressBar(request, newAmount);
    changeProgressBar(response, expectedAmount);
}

void MainWindow::changeProgressBar(QProgressBar * const bar, const quint16 value){
    if(value < 111)
        bar->setStyleSheet(" QProgressBar::chunk {background-color: rgb( 67,255, 76);}");
    if(value > 110 && value < 367)
        bar->setStyleSheet(" QProgressBar::chunk {background-color: rgb(247,255,149);}");
    if(value > 366)
        bar->setStyleSheet(" QProgressBar::chunk {background-color: rgb(231, 88,129);}");
    bar->setValue(value);
}

void MainWindow::maskChanged(QPushButton * const but)
{
    quint8 pos = but->objectName().right(2).toInt();
    if(but->isChecked())
        moduleMask |= 1 << pos;
    else
        moduleMask ^= 1 << pos;
    ui->pushButton_ADD->setEnabled(!multiMode || (moduleMask && multiMode));
    nWordsChanged();
}

void MainWindow::nWordsChanged(){
    const counter currentFreeSpaceRequest = maxWordsPerPacket - ui->treeWidget_REQUEST->packetSize(),
                  currentFreeSpaceResponse = maxWordsPerPacket - ui->treeWidget_REQUEST->expectedResponseSize();
    const quint8 currentNWords = static_cast<quint8>(ui->lineEdit_NWORDS->text().toUInt());
    const quint8 transactionsAmount = multiMode ? amountOfActiveModules() : 1;
    bool addButtonEnabled = true;
    switch (currentType) {
    case read:
    case nonIncrementingRead:
        addButtonEnabled = (currentFreeSpaceRequest  >= 2 * transactionsAmount) &&
                           (currentFreeSpaceResponse >= (1 + currentNWords) * transactionsAmount);
        break;
    case write:
    case nonIncrementingWrite:
        addButtonEnabled = (currentFreeSpaceRequest  >= (2 + currentNWords) * transactionsAmount) &&
                           (currentFreeSpaceResponse >= transactionsAmount);
        break;
    case RMWsum:
        addButtonEnabled = (currentFreeSpaceRequest  >= 3 * transactionsAmount) &&
                           (currentFreeSpaceResponse >= 2 * transactionsAmount);
        break;
    case RMWbits:
        addButtonEnabled = (currentFreeSpaceRequest  >= 4 * transactionsAmount) &&
                           (currentFreeSpaceResponse >= 2 * transactionsAmount);
        break;
    default: break;
    }
    ui->pushButton_ADD->setEnabled(addButtonEnabled);
}

void MainWindow::sendPacket(){
    ui->treeWidget_RESPONSE->clear();
    //Declaration of the variables numWord - counter of words in packet, transactionCounter - counter of transactions in the packet
    quint16 numWord = 0, transactionCounter = 0;
    //DInitialisation of requestViewer will let me write less code to apple QTreeWidget functions
    packetViewer* requestViewer = ui->treeWidget_REQUEST;
    //First element in packet is Packet header/ It is getting from the first child of the first item in tree
    request[numWord++] = requestViewer->topLevelItem(transactionCounter++)->text(0).split("x").at(1).left(8).toUInt(nullptr, 16);
    //while words amount in packet less than the building packet size
    while(numWord < requestViewer->packetSize()){
        //we will get top level items from tree, which correspond to every transaction
        QTreeWidgetItem* parentTransaction = requestViewer->topLevelItem(transactionCounter++);
        request[numWord++] = parentTransaction->text(0).split("x").at(1).left(8).toUInt(nullptr, 16);
        //the body (children) of every transaction will be placed in packet
        for(quint16 i = 0; i < parentTransaction->childCount(); ++i)
            request[numWord++] = parentTransaction->child(i)->text(0).toUInt(nullptr, 16);
    }
    //After our packet was filled we send it to server
    if(ui->lineEdit_IPADDRESS->text().contains(IP))
        socket->writeDatagram(Crequest, requestViewer->packetSize() * sizeof (IPbusWord), QHostAddress(ui->lineEdit_IPADDRESS->text()), 50001);
    else ui->statusbar->showMessage("No such address");
    ui->treeWidget_REQUEST->reinit();
    nWordsChanged();
}

void MainWindow::getResponse(){
    qDebug() << response[0];
    //when we have pending datagram
    if(socket->hasPendingDatagrams()){
        //we get response size, to get coressponding amount of bytes from the packet, which we got
        responseSize = static_cast<quint16>(socket->pendingDatagramSize());
        //getting that bytes in char array Cresponse
        socket->readDatagram(Cresponse, responseSize);
        //new packet should be displayed, so it's neccesarry to clear response viewer
        ui->treeWidget_RESPONSE->clear();
        //call function, which will display response to user
        if(responseSize) ui->treeWidget_RESPONSE->showPacket(response, responseSize / sizeof (IPbusWord), expanded, hiddenHeaders);
    }
}

void MainWindow::clear(){
    ui->treeWidget_REQUEST->clear();
    ui->treeWidget_REQUEST->reinit();
    nWordsChanged();
    ui->pushButton_SEND->setEnabled(false);
}

//apply the settings from file
void MainWindow::getConfiguration(){
    QSettings settings(QCoreApplication::applicationName() + ".ini", QSettings::IniFormat);

    settings.beginGroup("Network");
    ui->lineEdit_IPADDRESS->setText(settings.value("TargetIP", "127.0.0.1").toString());
    settings.endGroup();

    settings.beginGroup("IPbus");
    ui->lineEdit_ADDRESS->setText(settings.value("Address", "00000000").toString());
    ui->lineEdit_ANDTERM->setText(settings.value("ANDterm", "FFFFFFFF").toString());
    ui->lineEdit_ORTERM ->setText(settings.value("ORTERM", "00000000").toString());
    ui->lineEdit_NWORDS ->setText(settings.value("nWords", "1").toString());
    ui->checkBox_RANDOMIZE_DATA->setChecked(settings.value("RandomizeData", 0).toBool());
    ui->centralwidget->findChild<QRadioButton *>("radioButton_" + coresspondingTypes.key(static_cast<TransactionType>(settings.value("TransactionType", 0).toInt())))->click();
    settings.endGroup();

    settings.beginGroup("GUI");
    expanded = settings.value("AlwaysExpanded", 0).toBool();    // 1)
    hiddenHeaders = settings.value("HiddenHeaders", 0).toBool();// 2)
    multiMode = settings.value("MultiMode", 0).toBool();        // 3)
    moduleMask = settings.value("Mask", 0).toString().toUInt(nullptr, 16);
    settings.endGroup();


    ui->checkBox_expandAll    ->setChecked(expanded      ); //1)
    ui->checkBox_removeHeaders->setChecked(hiddenHeaders ); //2)
    ui->checkBox_multiMode    ->setChecked(multiMode     ); //3)

    ui->checkBox_expandAll    ->setEnabled(!hiddenHeaders);//2)

    setMultiMode(multiMode); //3)

    setMask(moduleMask);
    ui->pushButton_ADD->setEnabled(!multiMode || (moduleMask && multiMode));

    settings.beginGroup("RESPONSE");
    quint16 tmpSz = settings.value("Length", "0").toInt();
    responseSize = tmpSz * sizeof(IPbusWord);
    if(tmpSz)for(size_t i = 0; i < tmpSz; ++i){
        response[i] = settings.value(QString::asprintf("%03d", i), "0").toString().toInt(nullptr, 16);
    }
    settings.endGroup();

    if(tmpSz)ui->treeWidget_RESPONSE->showPacket(response, tmpSz, expanded, hiddenHeaders);

    settings.beginGroup("REQUEST");
    tmpSz = settings.value("Length", "0").toInt();
    if(tmpSz)for(size_t i = 0; i < tmpSz; ++i){
        request[i] = settings.value(QString::asprintf("%03d", i), "0").toString().toInt(nullptr, 16);
    }
    settings.endGroup();

    makeRequestFromArray(tmpSz);
}

//take the last session values from GUI and store them into the file
void MainWindow::saveConfiguration(){
    QSettings settings(QCoreApplication::applicationName() + ".ini", QSettings::IniFormat);

    settings.beginGroup("Network");
    settings.setValue("TargetIP", ui->lineEdit_IPADDRESS->text());
    settings.endGroup();

    settings.beginGroup("IPbus");
    settings.setValue("TransactionType", currentType                );
    settings.setValue("Address",        ui->lineEdit_ADDRESS->text());
    settings.setValue("ORterm",         ui->lineEdit_ORTERM-> text() );
    settings.setValue("ANDterm",        ui->lineEdit_ANDTERM->text());
    settings.setValue("nWords",         ui->lineEdit_NWORDS-> text() );
    settings.setValue("RandomizeData",  ui->checkBox_RANDOMIZE_DATA->isChecked() ? 1 : 0);
    settings.endGroup();

    settings.beginGroup("GUI");
    settings.setValue("AlwaysExpanded", expanded     );
    settings.setValue("HiddenHeaders" , hiddenHeaders);
    settings.setValue("MultiMode"     , multiMode    );
    settings.setValue("Mask", QString::asprintf("%06X", moduleMask));
    settings.endGroup();

    settings.beginGroup("RESPONSE");
    settings.setValue("Length", ui->treeWidget_RESPONSE->packetSize());
    for(size_t i = 0; i < ui->treeWidget_RESPONSE->packetSize(); ++i){
        settings.setValue(QString::asprintf("%03d", i),  QString::asprintf("%X", response[i]));
    }
    settings.endGroup();

    settings.beginGroup("REQUEST");
    settings.setValue("Length", ui->treeWidget_REQUEST->packetSize());
    for(size_t i = 0; i < ui->treeWidget_REQUEST->packetSize(); ++i){
        settings.setValue(QString::asprintf("%03d", i), QString::asprintf("%X", request[i]));
    }
    settings.endGroup();

}



void MainWindow::on_checkBox_expandAll_clicked()
{
    expanded = ui->checkBox_expandAll->isChecked();
    if(expanded)
        ui->treeWidget_RESPONSE->expandAllTopLevelItems();
    else
        ui->treeWidget_RESPONSE->collapseAllTopLEvelItems();
}


void MainWindow::on_checkBox_removeHeaders_clicked(){

    hiddenHeaders = ui->checkBox_removeHeaders->isChecked();

    ui->checkBox_expandAll->setEnabled(!hiddenHeaders);

    ui->treeWidget_RESPONSE->clear();
    if(responseSize)ui->treeWidget_RESPONSE->showPacket(response, responseSize / sizeof(IPbusWord), expanded, hiddenHeaders);
}


void MainWindow::on_checkBox_multiMode_clicked()
{
    multiMode = ui->checkBox_multiMode->isChecked();
    setMultiMode(multiMode);
}

