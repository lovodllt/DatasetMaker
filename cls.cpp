#include "cls.h"
#include "ui_cls.h"

cls::cls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::cls)
{
    ui->setupUi(this);

    QButtonGroup *classButtonGroup = ui->classButtonGroup;

    connect(classButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &cls::onClassSelected);
}

cls::~cls()
{
    delete ui;
}

void cls::onClassSelected(QAbstractButton *button)
{
    QString classId = button->text();
    qDebug() << "Class Selected: " << classId;
}

void cls::on_sure_clicked()
{
    QString width = ui->widthEdit->text();
    QString height = ui->heightEdit->text();

    if (width != "" && height != "")
    {
        qDebug() << "Width: " << width << "  Height: " << height;
    }
}
