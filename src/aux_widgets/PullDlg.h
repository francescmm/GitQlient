#ifndef PULLDLG_H
#define PULLDLG_H

#include <QDialog>

namespace Ui
{
class PullDlg;
}

class PullDlg : public QDialog
{
   Q_OBJECT

public:
   explicit PullDlg(const QString &text, QWidget *parent = nullptr);
   ~PullDlg();

   void accept() override;

private:
   Ui::PullDlg *ui;
};

#endif // PULLDLG_H
