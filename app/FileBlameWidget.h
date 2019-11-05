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
class FileDiffView;
class QVBoxLayout;
class QScrollArea;
class QLabel;

class FileBlameWidget : public QFrame
{
   Q_OBJECT

public:
   explicit FileBlameWidget(QSharedPointer<Git> git, QWidget *parent = nullptr);

   void setup(const QString &fileName);

private:
   QSharedPointer<Git> mGit;
   QFrame *mAnotation = nullptr;
   QScrollArea *mScrollArea = nullptr;
   QFont mInfoFont;
   QFont mCodeFont;

   struct Annotation
   {
      QString shortSha;
      QString author;
      QDateTime dateTime;
      int line;
      QString content;
   };

   QVector<Annotation> processBlame(const QString &blame);
   void formatAnnotatedFile(const QVector<Annotation> &annotations);
   QLabel *createDateLabel(const Annotation &annotation, bool isFirst);
   QLabel *createAuthorLabel(const Annotation &annotation, bool isFirst);
   QLabel *createMessageLabel(const Annotation &annotation, bool isFirst);
   QLabel *createNumLabel(int row);
   QLabel *createCodeLabel(const Annotation &annotation);
};
