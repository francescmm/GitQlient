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
#include <QDateTime>

class Git;
class QScrollArea;
class ClickableFrame;
class QLabel;

class FileBlameWidget : public QFrame
{
   Q_OBJECT

signals:
   void signalCommitSelected(const QString &sha);

public:
   explicit FileBlameWidget(const QSharedPointer<Git> &git, QWidget *parent = nullptr);

   void setup(const QString &fileName, const QString &currentSha, const QString &previousSha);
   void reload(const QString &currentSha, const QString &previousSha);
   QString getCurrentSha() const;
   QString getCurrentFile() const { return mCurrentFile; }

private:
   QSharedPointer<Git> mGit;
   QFrame *mAnotation = nullptr;
   QLabel *mCurrentSha = nullptr;
   QLabel *mPreviousSha = nullptr;
   QScrollArea *mScrollArea = nullptr;
   QFont mInfoFont;
   QFont mCodeFont;
   QString mCurrentFile;

   struct Annotation
   {
      QString sha;
      QString author;
      QDateTime dateTime;
      int line = 0;
      QString content;
   };

   QVector<Annotation> processBlame(const QString &blame);
   void formatAnnotatedFile(const QVector<Annotation> &annotations);
   QLabel *createDateLabel(const Annotation &annotation, bool isFirst);
   QLabel *createAuthorLabel(const QString &author, bool isFirst);
   ClickableFrame *createMessageLabel(const QString &sha, bool isFirst);
   QLabel *createNumLabel(const Annotation &annotation, int row);
   QLabel *createCodeLabel(const QString &content);
};
