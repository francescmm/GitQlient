#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2019  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This program is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <QFrame>

class QPushButton;
class QButtonGroup;
class Git;
class ProgressDlg;
class GitQlientSettings;
class QVBoxLayout;

class ConfigWidget : public QFrame
{
   Q_OBJECT

signals:
   void signalOpenRepo(const QString &repoPath);

public:
   explicit ConfigWidget(QWidget *parent = nullptr);
   ~ConfigWidget() override;

   void updateRecentProjectsList();

private:
   QSharedPointer<Git> mGit;
   QPushButton *mOpenRepo = nullptr;
   QPushButton *mCloneRepo = nullptr;
   QPushButton *mInitRepo = nullptr;
   QButtonGroup *mBtnGroup = nullptr;
   ProgressDlg *mProgressDlg = nullptr;
   QString mPathToOpen;
   GitQlientSettings *mSettings = nullptr;
   QVBoxLayout *mRecentProjectsLayout = nullptr;
   QWidget *mInnerWidget = nullptr;

   void openRepo();
   void cloneRepo();
   void initRepo();
   QWidget *createConfigWidget();
   QWidget *createRecentProjectsPage();
   void updateProgressDialog(const QString &stepDescription, int value);
};
