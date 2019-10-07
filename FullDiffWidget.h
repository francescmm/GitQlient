#pragma once

/****************************************************************************************
 ** Heimdall is an application to manage and operate one or several Git repositories. With
 ** Heimdal you will be able to add commits, branches and manage all the options Git provides.
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

#include <QPointer>
#include <QSyntaxHighlighter>
#include <QTextEdit>

class Domain;
class Git;
class MyProcess;
class StateInfo;

class DiffHighlighter : public QSyntaxHighlighter
{
public:
   DiffHighlighter(QTextEdit *p)
      : QSyntaxHighlighter(p)
   {
   }
   void setCombinedLength(uint c) { cl = c; }
   virtual void highlightBlock(const QString &text);

private:
   uint cl = 0;
};

class FullDiffWidget : public QTextEdit
{
   Q_OBJECT
public:
   FullDiffWidget(QWidget *parent);
   void clear();
   void centerOnFileHeader(StateInfo &st);
   void refresh();
   void update(StateInfo &st);

   enum PatchFilter
   {
      VIEW_ALL,
      VIEW_ADDED,
      VIEW_REMOVED
   };
   PatchFilter curFilter = VIEW_ALL;
   PatchFilter prevFilter = VIEW_ALL;

public slots:
   void typeWriterFontChanged();
   void procReadyRead(const QByteArray &data);
   void procFinished();

private:
   friend class DiffHighlighter;

   void scrollCursorToTop();
   void scrollLineToTop(int lineNum);
   int positionToLineNum(int pos);
   int topToLineNum();
   void saveRestoreSizes(bool startup = false);
   bool getMatch(int para, int *indexFrom, int *indexTo);
   void centerMatch(int id = 0);
   bool centerTarget(const QString &target);
   void processData(const QByteArray &data, int *prevLineNum = nullptr);

   DiffHighlighter *diffHighlighter = nullptr;
   QPointer<MyProcess> proc;
   bool diffLoaded = false;
   bool seekTarget = false;
   QByteArray patchRowData;
   QString halfLine;
   QString target;

   struct MatchSelection
   {
      int paraFrom;
      int indexFrom;
      int paraTo;
      int indexTo;
   };
   typedef QVector<MatchSelection> Matches;
   Matches matches;
};
