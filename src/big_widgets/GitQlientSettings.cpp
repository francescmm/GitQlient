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

void GitQlientSettings::saveRecentProjects(const QString &projectPath)
{
   auto usedProjects = QSettings::value("usedProjects", QStringList()).toStringList();

   if (usedProjects.contains(projectPath))
   {
      const auto index = usedProjects.indexOf(projectPath);
      usedProjects.takeAt(index);
   }

   usedProjects.prepend(projectPath);

   while (!usedProjects.isEmpty() && usedProjects.count() > 5)
      usedProjects.removeLast();

   GitQlientSettings::setValue("usedProjects", usedProjects);
}

void GitQlientSettings::saveMostUsedProjects(const QString &projectPath)
{
   auto projects = QSettings::value("recentProjects", QStringList()).toStringList();
   auto timesUsed = QSettings::value("recentProjectsCount", QList<QVariant>()).toList();
   int count = 1;

   if (projects.contains(projectPath))
   {
      const auto index = projects.indexOf(projectPath);
      timesUsed[index] = QString::number(count + timesUsed[index].toInt());
   }
   else
   {
      projects.append(projectPath);
      timesUsed.append(count);
   }

   GitQlientSettings::setValue("recentProjects", projects);
   GitQlientSettings::setValue("recentProjectsCount", timesUsed);
}

QVector<QString> GitQlientSettings::getRecentProjects() const
{
   const auto projects = QSettings::value("recentProjects", QStringList()).toStringList();
   const auto timesUsed = QSettings::value("recentProjectsCount", QString()).toList();

   QMultiMap<int, QString> projectOrderedByUse;

   const auto projectsCount = projects.count();
   const auto timesCount = timesUsed.count();

   for (auto i = 0; i < projectsCount && i < timesCount; ++i)
      projectOrderedByUse.insert(timesUsed.at(i).toInt(), projects.at(i));

   QVector<QString> recentProjects;
   const auto end = std::min(projectOrderedByUse.count(), 5);
   const auto orderedProjects = projectOrderedByUse.values();

   for (auto i = 0; i < end; ++i)
      recentProjects.append(orderedProjects.at(orderedProjects.count() - 1 - i));

   return recentProjects;
}

QStringList GitQlientSettings::getMostUsedProjects() const
{
   auto projects = QSettings::value("usedProjects", QStringList()).toStringList();

   QStringList recentProjects;
   const auto end = std::min(projects.count(), 5);

   for (auto i = 0; i < end; ++i)
      recentProjects.append(projects.takeFirst());

   return recentProjects;
}
