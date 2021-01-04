#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow){
    ui->setupUi(this);
    CoresspondingTypes["WRITE"]   = write;
    CoresspondingTypes["READ"]    = read;
    CoresspondingTypes["NIREAD"]  = nonIncrementingRead;
    CoresspondingTypes["NIWRITE"] = nonIncrementingWrite;
    CoresspondingTypes["RMWSUM"]  = RMWsum;
    CoresspondingTypes["RMWBITS"] = RMWbits;
    //find all radioButtons to connect them with onr slot
    QList<QRadioButton*> radioButtons = ui->centralwidget->findChildren<QRadioButton*>(QRegularExpression("radioButton_*"));
    QRadioButton* but;
    foreach (but, radioButtons)
        connect(but, &QRadioButton::clicked, this, [but, this](){selectedTransactionChanged(CoresspondingTypes[but->objectName().remove("radioButton_")]);});
    connect(ui->pushButton_ADD, &QPushButton::clicked, ui->treeWidget_PACKET_WIEVER, [=](){
        if(!ui->treeWidget_PACKET_WIEVER->topLevelItem(0)) ui->treeWidget_PACKET_WIEVER->addIPbusPacketHeader();
        const IPbusWord address = ui->lineEdit_ADDRESS->text().toUInt(nullptr, 16);
        const quint8     nWords = static_cast<quint8>(ui->lineEdit_NWORDS->text().toUInt(nullptr, 10));
        const IPbusWord andTerm = ui->lineEdit_ANDTERM->text().toUInt(nullptr, 16);
        const IPbusWord  orTerm = ui->lineEdit_ORTERM->text().toUInt(nullptr, 16);
        ui->treeWidget_PACKET_WIEVER->addIPbusTransaction(currentType, nWords, address, andTerm, orTerm);
        nWordsChanged();});
    //progress bar processing
    connect(ui->treeWidget_PACKET_WIEVER, &packetViewer::wordsAmountChanged, this, &MainWindow::packetSizeChanged);
    connect(ui->lineEdit_NWORDS, &QLineEdit::textEdited, this, &MainWindow::nWordsChanged);
    ui->lineEdit_NWORDS->setValidator(new QRegExpValidator(QRegExp("[1-9]|[1-9][0-9]|1[0-9]{1,2}|2[0-4][0-9]|25[0-5]")));
    //installation of eventFilter for delete button
    ui->treeWidget_PACKET_WIEVER->installEventFilter(this);
    
}

bool MainWindow::eventFilter(QObject * obj, QEvent* event){
    if (obj == ui->treeWidget_PACKET_WIEVER){
        if (event->type() == QEvent::KeyPress){
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Delete){
                if(ui->treeWidget_PACKET_WIEVER->itemAbove(ui->treeWidget_PACKET_WIEVER->currentItem()) && !ui->treeWidget_PACKET_WIEVER->currentItem()->parent()){
                    for(quint16 i = 0; i < ui->treeWidget_PACKET_WIEVER->currentItem()->childCount(); ++i)
                        delete ui->treeWidget_PACKET_WIEVER->currentItem()->child(i);
                    delete ui->treeWidget_PACKET_WIEVER->currentItem();
                    ui->treeWidget_PACKET_WIEVER->reinit();
                    return true;
                }else{
                    return false;
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::selectedTransactionChanged(const TransactionType type){
    currentType = type;
    switch (type) {
    case write:
    case read:
    case nonIncrementingRead:
    case nonIncrementingWrite:{
        ui->lineEdit_ORTERM->setEnabled(false);
        ui->lineEdit_ANDTERM->setEnabled(false);
        ui->lineEdit_NWORDS->setEnabled(true);
        break;
    }
    case RMWsum:{
        ui->lineEdit_ANDTERM->setEnabled(true);
        ui->lineEdit_ORTERM->setEnabled(false);
        ui->lineEdit_NWORDS->setText("1");
        ui->lineEdit_NWORDS->setEnabled(false);
        ui->label_ANDTERM->setText("ADDEND");
        break;
    }
    case RMWbits:{
        ui->lineEdit_ANDTERM->setEnabled(true);
        ui->lineEdit_ORTERM->setEnabled(true);
        ui->lineEdit_NWORDS->setText("1");
        ui->lineEdit_NWORDS->setEnabled(false);
        ui->label_ANDTERM->setText("AND term");
        break;
    }
    default:
        break;
    }
    //checking if packet is possible to send
    nWordsChanged();
}

void MainWindow::packetSizeChanged(){
    const counter newAmount = ui->treeWidget_PACKET_WIEVER->size();
    QProgressBar* request = ui->progressBar_WORDS;
    if(newAmount < 111)
        request->setStyleSheet(" QProgressBar::chunk {background-color: rgb( 67,255, 76);}");
    if(newAmount > 110 && newAmount < 367)
        request->setStyleSheet(" QProgressBar::chunk {background-color: rgb(247,255,149);}");
    if(newAmount > 366)
        request->setStyleSheet(" QProgressBar::chunk {background-color: rgb(231, 88,129);}");
    request->setValue(newAmount);

}

void MainWindow::nWordsChanged(){
    const counter currentFreeSpaceRequest = maxWordsPerPacket - ui->treeWidget_PACKET_WIEVER->size();
    const quint8 currentNWords = static_cast<quint8>(ui->lineEdit_NWORDS->text().toUInt());
    bool addButtonEnabled = true;
    switch (currentType) {
    case read:
    case nonIncrementingRead:
        addButtonEnabled = currentFreeSpaceRequest >= 2;
        break;
    case write:
    case nonIncrementingWrite:
        addButtonEnabled = currentFreeSpaceRequest >= 2 + currentNWords;
        break;
    case RMWsum:
        addButtonEnabled = currentFreeSpaceRequest >= 3;
        break;
    case RMWbits:
        addButtonEnabled = currentFreeSpaceRequest >= 4;
        break;
    default: break;
    }
    ui->pushButton_ADD->setEnabled(addButtonEnabled);
}

void MainWindow::SendPacket(){

}

