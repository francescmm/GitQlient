#include "GitQlientSettings.h"

#include <QVector>

void GitQlientSettings::setValue(const QString &key, const QVariant &value)
{
   QSettings::setValue(key, value);

   emit valueChanged(key, value);
}

void GitQlientSettings::setProjectOpened(const QString &projectPath)
{
   saveMostUsedProjects(projectPath);

   saveRecentProjects(projectPath);
}

QStringList GitQlientSettings::getRecentProjects() const
{
   auto projects = QSettings::value("Config/RecentProjects", QStringList()).toStringList();

   QStringList recentProjects;
   const auto end = std::min(projects.count(), 5);

   for (auto i = 0; i < end; ++i)
      recentProjects.append(projects.takeFirst());

   return recentProjects;
}

void GitQlientSettings::saveRecentProjects(const QString &projectPath)
{
   auto usedProjects = QSettings::value("Config/RecentProjects", QStringList()).toStringList();

   if (usedProjects.contains(projectPath))
   {
      const auto index = usedProjects.indexOf(projectPath);
      usedProjects.takeAt(index);
   }

   usedProjects.prepend(projectPath);

   while (!usedProjects.isEmpty() && usedProjects.count() > 5)
      usedProjects.removeLast();

   GitQlientSettings::setValue("Config/RecentProjects", usedProjects);
}

void GitQlientSettings::clearRecentProjects()
{
   remove("Config/RecentProjects");
}

void GitQlientSettings::saveMostUsedProjects(const QString &projectPath)
{
   auto projects = QSettings::value("Config/UsedProjects", QStringList()).toStringList();
   auto timesUsed = QSettings::value("Config/UsedProjectsCount", QList<QVariant>()).toList();

   if (projects.contains(projectPath))
   {
      const auto index = projects.indexOf(projectPath);
      timesUsed[index] = QString::number(timesUsed[index].toInt() + 1);
   }
   else
   {
      projects.append(projectPath);
      timesUsed.append(1);
   }

   GitQlientSettings::setValue("Config/UsedProjects", projects);
   GitQlientSettings::setValue("Config/UsedProjectsCount", timesUsed);
}

void GitQlientSettings::clearMostUsedProjects()
{
   remove("Config/UsedProjects");
   remove("Config/UsedProjectsCount");
}

QStringList GitQlientSettings::getMostUsedProjects() const
{
   const auto projects = QSettings::value("Config/UsedProjects", QStringList()).toStringList();
   const auto timesUsed = QSettings::value("Config/UsedProjectsCount", QString()).toList();

   QMultiMap<int, QString> projectOrderedByUse;

   const auto projectsCount = projects.count();
   const auto timesCount = timesUsed.count();

   for (auto i = 0; i < projectsCount && i < timesCount; ++i)
      projectOrderedByUse.insert(timesUsed.at(i).toInt(), projects.at(i));

   QStringList recentProjects;
   const auto end = std::min(projectOrderedByUse.count(), 5);
   const auto orderedProjects = projectOrderedByUse.values();

   for (auto i = 0; i < end; ++i)
      recentProjects.append(orderedProjects.at(orderedProjects.count() - 1 - i));

   return recentProjects;
}
