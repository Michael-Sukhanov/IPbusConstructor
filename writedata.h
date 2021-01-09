#ifndef WRITEDATA_H
#define WRITEDATA_H

#include <QDialog>
#include <QDebug>

namespace Ui {
class writedata;
}

class writedata : public QDialog
{
    Q_OBJECT

public:
    explicit writedata(QWidget *parent = nullptr);
    ~writedata();

signals:
    void valueReady(quint32 value);
    void editingFinished();


private:
    Ui::writedata *ui;
    void handleTextEdit();
};

#endif // WRITEDATA_H
