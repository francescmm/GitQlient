#include "ConfigWidget.h"

#include <QPushButton>
#include <QGridLayout>

ConfigWidget::ConfigWidget(QWidget *parent)
   : QWidget(parent)
   , mOpenRepo(new QPushButton(tr("Open existing repo")))
   , mCloneRepo(new QPushButton(tr("Clone new repo")))
   , mInitRepo(new QPushButton(tr("Init new repo")))
{
   const auto layout = new QGridLayout(this);

   layout->addWidget(mOpenRepo, 0, 0);
   layout->addWidget(mCloneRepo, 1, 0);
   layout->addWidget(mInitRepo, 2, 0);
}
