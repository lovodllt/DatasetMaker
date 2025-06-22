#ifndef CLS_H
#define CLS_H

#include <QWidget>
#include <QAbstractButton>
#include <QDebug>

namespace Ui { class cls; }

class cls : public QWidget
{
    Q_OBJECT

public:
    explicit cls(QWidget *parent = nullptr);
    ~cls();

private slots:
    void onClassSelected(QAbstractButton *button);
    void on_sure_clicked();

private:
    Ui::cls *ui;
    //QButtonGroup *classButtonGroup;
};

#endif // CLS_H
