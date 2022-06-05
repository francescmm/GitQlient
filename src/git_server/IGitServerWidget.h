#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2021  Francesc Martinez
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
#include <QtPlugin>

#include <gitserverplugin_global.h>

#include <ConfigData.h>

class GitBase;
class IGitServerCache;

#define IGitServerWidget_iid "francescmm.GitServerPlugin/0.1.0"

class IGitServerWidget : public QFrame
{
public:
   explicit IGitServerWidget(QWidget *parent = nullptr)
      : QFrame(parent)
   {
   }

   virtual ~IGitServerWidget() = default;

   /**
    * @brief configure Configures the widget by showing the config dialog or the full content if it was already
    * configured.
    * @return Returns true if configured, otherwise false.
    */
   virtual bool configure(const GitServerPlugin::ConfigData &config,
                          const QVector<QPair<QString, QStringList>> &remoteBranches, const QString &styles)
       = 0;

   /**
    * @brief isConfigured Returns the current state of the widget
    * @return True if configured, otherwise false.
    */
   virtual bool isConfigured() const = 0;

   /**
    * @brief openPullRequest The method opens the PR view directly.
    * @param prNumber The PR number.
    */
   virtual void openPullRequest(int prNumber) = 0;

   /**
    * @brief start Creates all the contents of the GitServerWidget.
    * @param The remote branches of the current repo.
    */
   virtual void start(const QVector<QPair<QString, QStringList>> &remoteBranches) = 0;

   /**
    * @brief getCache Retrieves the internal cache to be read by any other application.
    * @return A shared pointer to the internal cache.
    */
   virtual QSharedPointer<IGitServerCache> getCache() = 0;

   /**
    * @brief createWidget Creates a IGitServerWidget.
    * @param git The configured GitBase object pointing to the repository directory.
    * @return Return a newly created widget.
    */
   virtual IGitServerWidget *createWidget(const QSharedPointer<GitBase> &git) = 0;

protected:
   bool mConfigured = false;
};

Q_DECLARE_INTERFACE(IGitServerWidget, IGitServerWidget_iid)

GITSERVERPLUGIN_EXPORT IGitServerWidget *createWidget(const QSharedPointer<GitBase> &git);
