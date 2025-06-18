#include "cls.h"
#include "ui_cls.h"

cls::cls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::cls)
{
    ui->setupUi(this);
}

cls::~cls()
{
    delete ui;
}
