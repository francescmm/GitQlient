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

#include <ServerMilestone.h>
#include <ServerLabel.h>
#include <Assignee.h>

#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QDateTime>

struct ServerIssue
{
   ServerIssue() = default;
   ServerIssue(const QString &_title, const QByteArray &_body, const ServerMilestone &goal,
               const QVector<ServerLabel> &_labels, const QVector<GitServer::User> &_assignees)
      : title(_title)
      , body(_body)
      , milestone(goal)
      , labels(_labels)
      , assignees(_assignees)
   {
   }

   QString title;
   QByteArray body;
   ServerMilestone milestone;
   QVector<ServerLabel> labels;
   GitServer::User creator;
   QVector<GitServer::User> assignees;
   QString url;
   QDateTime creation;

   QJsonObject toJson() const
   {
      QJsonObject object;

      if (!title.isEmpty())
         object.insert("title", title);

      if (!body.isEmpty())
         object.insert("body", body.toStdString().c_str());

      if (milestone.id != -1)
         object.insert("milestone", milestone.id);

      QJsonArray array;
      auto count = 0;
      for (auto assignee : assignees)
         array.insert(count++, assignee.name);
      object.insert("assignees", array);

      QJsonArray labelsArray;
      count = 0;
      for (auto label : labels)
         labelsArray.insert(count++, label.name);
      object.insert("labels", labelsArray);

      return object;
   }
};
