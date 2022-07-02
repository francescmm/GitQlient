#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2022  Francesc Martinez
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

#include <gitserverplugin_global.h>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVector>

namespace GitServerPlugin
{
enum class Platform
{
   GitLab,
   GitHub
};

struct User
{
   int id;
   QString name;
   QString avatar;
   QString url;
   QString type;
};

struct Milestone
{
   int id = -1;
   int number = -1;
   QString nodeId;
   QString title;
   QString description;
   bool isOpen = false;
};

struct Label
{
   int id;
   QString nodeId;
   QString url;
   QString name;
   QString description;
   QString colorHex;
   bool isDefault;
};

struct Diff
{
   QString diff;
   QString file;
   int line;
   int originalLine;
};

struct Comment
{
   Comment() = default;
   Comment(int _id, const QString _body, const User &_user, const QDateTime dt, const QString relation)
      : id(_id)
      , body(_body)
      , creator(_user)
      , creation(dt)
      , association(relation)
   {
   }

   virtual ~Comment() = default;

   int id;
   QString body;
   User creator;
   QDateTime creation;
   QString association;
};

struct Review : public Comment
{
   Review() = default;

   QString state;
};

struct CodeReview : public Comment
{
   CodeReview() = default;
   bool operator==(const CodeReview &c) const { return c.reviewId == reviewId; }

   Diff diff;
   int replyToId;
   int reviewId;
   bool outdated;
};

struct Commit
{
   bool operator==(const Commit &c) const { return sha == c.sha; }
   QString sha;
   QString url;
   QString message;
   User commiter;
   User author;
   QDateTime authorCommittedTimestamp;
};

struct Issue
{
   Issue() = default;
   Issue(const QString &_title, const QByteArray &_body, const Milestone &goal, const QVector<Label> &_labels,
         const QVector<GitServerPlugin::User> &_assignees)
      : title(_title)
      , body(_body)
      , milestone(goal)
      , labels(_labels)
      , assignees(_assignees)
   {
   }
   virtual ~Issue() = default;

   int number {};
   QString title {};
   QByteArray body {};
   Milestone milestone {};
   QVector<Label> labels {};
   User creator {};
   QVector<User> assignees {};
   QString url {};
   QDateTime creation {};
   int commentsCount = 0;
   QVector<Comment> comments;
   bool isOpen = true;

   virtual QJsonObject toJson() const
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

      for (const auto &assignee : assignees)
         array.insert(count++, assignee.name);
      object.insert("assignees", array);

      object.insert("state", isOpen ? "open" : "closed");

      QJsonArray labelsArray;
      count = 0;

      for (const auto &label : labels)
         labelsArray.insert(count++, label.name);
      object.insert("labels", labelsArray);

      return object;
   }
};

struct PullRequest : public Issue
{
   PullRequest() = default;

   ~PullRequest() override = default;

   struct HeadState
   {
      enum class State
      {
         Failure,
         Success,
         Pending
      };

      struct Check
      {
         QString description;
         QString state;
         QString url;
         QString name;
      };

      QString sha {};
      QString state {};
      State eState {};
      QVector<Check> checks {};
   };

   QString head {};
   QString headRepo {};
   QString headUrl {};
   QString base {};
   QString baseRepo {};
   bool maintainerCanModify = true;
   bool draft = false;
   int id = 0;
   QString url {};
   HeadState state {};
   QMap<int, Review> reviews {};
   QVector<CodeReview> reviewComment {};
   int reviewCommentsCount = 0;
   int commitCount = 0;
   int additions = 0;
   int deletions = 0;
   int changedFiles = 0;
   bool merged = false;
   bool mergeable = false;
   bool rebaseable = false;
   QString mergeableState {};
   QVector<Commit> commits {};

   QJsonObject toJson() const override
   {
      QJsonObject object;

      object.insert("title", title);
      object.insert("head", head);
      object.insert("base", base);
      object.insert("body", body.toStdString().c_str());
      object.insert("maintainer_can_modify", maintainerCanModify);
      object.insert("draft", draft);

      return object;
   }

   bool isValid() const { return !title.isEmpty(); }
};

}
