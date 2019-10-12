/*
        Description: patch viewer window

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include "DiffWidget.h"
#include "common.h"
#include "git.h"
#include "FullDiffWidget.h"

#include <QVBoxLayout>

DiffWidget::DiffWidget(QSharedPointer<Git> git, QWidget *parent)
   : QWidget(parent)
   , mGit(git)
   , mDomain(new PatchViewDomain())
{
   QFont font;
   font.setFamily(QString::fromUtf8("Ubuntu Mono"));

   mTextEditDiff = new FullDiffWidget();
   mTextEditDiff->setObjectName("textEditDiff");
   mTextEditDiff->setFont(font);
   mTextEditDiff->setUndoRedoEnabled(false);
   mTextEditDiff->setLineWrapMode(QTextEdit::NoWrap);
   mTextEditDiff->setReadOnly(true);
   mTextEditDiff->setTextInteractionFlags(Qt::TextSelectableByMouse);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->addWidget(mTextEditDiff);

   mDomain->setOwner(this);
}

void DiffWidget::clear(bool complete)
{
   if (complete)
      mSt.clear();

   mTextEditDiff->clear();
}

void DiffWidget::setStateInfo(const StateInfo &st)
{
   mSt = st;
   mDomain->update(true, false);
}

bool DiffWidget::doUpdate(bool force)
{
   if ((mSt.isChanged(StateInfo::SHA) || force) && !mDomain->isLinked())
   {
      QString caption(mGit->getShortLog(mSt.sha()));

      if (caption.length() > 30)
         caption = caption.left(30 - 3).trimmed().append("...");
   }

   if (mSt.isChanged(StateInfo::ANY & ~StateInfo::FILE_NAME) || force)
   {
      mTextEditDiff->clear();

      if (normalizedSha != mSt.diffToSha() && !normalizedSha.isEmpty())
         normalizedSha = "";

      mTextEditDiff->update(mSt); // non blocking
   }

   if (mSt.isChanged() || force)
      mTextEditDiff->centerOnFileHeader(mSt);

   return true;
}

PatchViewDomain::PatchViewDomain()
   : Domain(false)
{
}

void PatchViewDomain::setOwner(DiffWidget *owner)
{
   mOwner = owner;
}

bool PatchViewDomain::doUpdate(bool force)
{
   return mOwner->doUpdate(force);
}
