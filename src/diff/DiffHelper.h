#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2020  Francesc Martinez
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

#include <DiffInfo.h>

#include <QStringList>
#include <QPair>

namespace DiffHelper
{

struct DiffChange
{
   QString newFileName;
   int newFileStartLine;
   QString oldFileName;
   int oldFileStartLine;
   QString header;
   QString content;
   QPair<QStringList, QVector<DiffInfo::ChunkInfo>> oldData;
   QPair<QStringList, QVector<DiffInfo::ChunkInfo>> newData;
};

inline void extractLinesFromHeader(QString header, int &startOldFile, int &startNewFile)
{
   header = header.split(" @@ ").first();
   header.remove("@");
   header = header.trimmed();
   auto modifications = header.split(" ");
   startOldFile = std::abs(modifications.first().split(",").first().toInt());
   startNewFile = std::abs(modifications.last().split(",").first().toInt());
}

inline QVector<DiffChange> splitDiff(const QString &diff)
{
   QVector<DiffHelper::DiffChange> changes;
   const auto chunks = diff.split("diff --git", Qt::SkipEmptyParts);

   for (const auto &chunk : chunks)
   {
      auto lines = chunk.split("\n");
      DiffHelper::DiffChange change;

      auto filesStr = lines.takeFirst();
      auto files = filesStr.trimmed().split(" ");
      change.newFileName = files.first().remove("a/");
      change.oldFileName = files.last().remove("b/");

      auto isA = lines.constFirst().startsWith("copy ") || lines.constFirst().startsWith("index ")
          || lines.constFirst().startsWith("new ");
      auto isB = lines.constFirst().startsWith("old ") || lines.constFirst().startsWith("rename ")
          || lines.constFirst().startsWith("similarity ");
      auto isC = lines.constFirst().startsWith("+++ ") || lines.constFirst().startsWith("--- ");

      while (isA || isB || isC)
      {
         lines.takeFirst();

         isA = lines.constFirst().startsWith("copy ") || lines.constFirst().startsWith("index ")
             || lines.constFirst().startsWith("new ");
         isB = lines.constFirst().startsWith("old ") || lines.constFirst().startsWith("rename ")
             || lines.constFirst().startsWith("similarity ");
         isC = lines.constFirst().startsWith("+++ ") || lines.constFirst().startsWith("--- ");
      }

      for (auto &line : lines)
      {
         if (line.startsWith("@@"))
         {
            if (!change.content.isEmpty())
            {
               changes.append(change);
               change.content.clear();
            }

            change.header = line;
            extractLinesFromHeader(change.header, change.oldFileStartLine, change.newFileStartLine);
         }
         else
            change.content.append(line + "\n");
      }

      changes.append(change);
   }
   return changes;
}

inline QVector<DiffInfo::ChunkInfo> processDiff(const QString &text, bool fileVsFile,
                                                QPair<QStringList, QVector<DiffInfo::ChunkInfo>> &newFileData,
                                                QPair<QStringList, QVector<DiffInfo::ChunkInfo>> &oldFileData)
{
   DiffInfo diff;
   // int newFileVariation = 0;
   int oldFileRow = 1;
   int newFileRow = 1;
   QVector<DiffInfo::ChunkInfo> chunks;

   const auto lines = text.split("\n");
   for (auto line : lines)
   {
      if (fileVsFile)
      {
         if (line.startsWith('-'))
         {
            line.remove(0, 1);

            if (diff.oldFile.startLine == -1)
               diff.oldFile.startLine = oldFileRow;

            oldFileData.first.append(line);

            ++oldFileRow;
         }
         else if (line.startsWith('+'))
         {
            line.remove(0, 1);

            if (diff.newFile.startLine == -1)
            {
               diff.newFile.startLine = newFileRow;
               diff.newFile.addition = true;
            }

            newFileData.first.append(line);

            ++newFileRow;
         }
         else
         {
            line.remove(0, 1);

            if (diff.oldFile.startLine != -1)
               diff.oldFile.endLine = oldFileRow - 1;

            if (diff.newFile.startLine != -1)
               diff.newFile.endLine = newFileRow - 1;

            if (diff.isValid())
            {
               if (diff.newFile.isValid())
               {
                  newFileData.second.append(diff.newFile);
                  chunks.append(diff.newFile);
               }

               if (diff.oldFile.isValid())
               {
                  oldFileData.second.append(diff.oldFile);
                  chunks.append(diff.oldFile);
               }
            }

            oldFileData.first.append(line);
            newFileData.first.append(line);

            diff = DiffInfo();

            ++oldFileRow;
            ++newFileRow;
         }
      }
      else
      {
         oldFileData.first.append(line);
         newFileData.first.append(line);
      }
   }

   return chunks;
}
}
