#include "detection.h"
#include "ui_detection.h"

detection::detection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::detection)
{
    ui->setupUi(this);
}

detection::~detection()
{
    delete ui;
}
