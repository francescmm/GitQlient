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

#include <GitCache.h>
#include <GitWip.h>
#include <QLogger.h>

#include <QSharedPointer>

namespace WipHelper
{
inline bool update(const QSharedPointer<GitBase> &git, const QSharedPointer<GitCache> cache)
{
   GitWip wip(git);

   const auto files = wip.getUntrackedFiles();
   cache->setUntrackedFilesList(std::move(files));

   if (const auto info = wip.getWipInfo(); info->second.isValid())
      return cache->updateWipCommit(info->first, info->second);

   return false;
}
}
