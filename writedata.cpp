#include "writedata.h"
#include "ui_writedata.h"
#include <QPlainTextEdit>

writedata::writedata(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::writedata)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, [this](){ui->plainTextEdit->clear(); this->close();});
    connect(ui->buttonBox, &QDialogButtonBox::accepted, [this](){
        handleTextEdit();
        ui->plainTextEdit->clear();
        emit editingFinished();});
    setWindowTitle("Enter your data");
}

writedata::~writedata()
{
    delete ui;
}

void writedata::handleTextEdit(){
    QStringList listOfValues = ui->plainTextEdit->toPlainText().split('\n', QString::SkipEmptyParts);
    QString stringVal;
    quint32 value;
    quint8 counter = 0;
    bool ok;
    foreach(stringVal, listOfValues){
        counter++;
        value = stringVal.toUInt(&ok, 16);
        if(ok && counter != 0xFF) emit valueReady(value);
    }
}
