#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2019  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 3 of the License, or (at your option) any later version.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <QWidget>

#include "domain.h"

class Git;

class PatchViewDomain;
class Git;
class FullDiffWidget;

class DiffWidget : public QWidget
{
   Q_OBJECT

public:
   explicit DiffWidget(QSharedPointer<Git> git, QWidget *parent = nullptr);
   void clear(bool complete = true);

   StateInfo st() const { return mSt; }
   void setStateInfo(const StateInfo &st);

   virtual bool doUpdate(bool force);

private:
   void saveRestoreSizes(bool startup = false);

   QSharedPointer<Git> mGit;
   PatchViewDomain *mDomain = nullptr;
   FullDiffWidget *mTextEditDiff = nullptr;
   QString normalizedSha;
   StateInfo mSt;

   enum ButtonId
   {
      DIFF_TO_PARENT = 0,
      DIFF_TO_HEAD = 1,
      DIFF_TO_SHA = 2
   };
};

class PatchViewDomain : public Domain
{
public:
   PatchViewDomain(QSharedPointer<Git> git);
   void setOwner(DiffWidget *owner);
   bool doUpdate(bool force) override;

   friend class DiffWidget;

private:
   DiffWidget *mOwner = nullptr;
};
