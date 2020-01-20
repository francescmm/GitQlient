#include "GitQlientSettings.h"

#include <QVector>

void GitQlientSettings::setValue(const QString &key, const QVariant &value)
{
   QSettings::setValue(key, value);

   emit valueChanged(key, value);
}

void GitQlientSettings::setProjectOpened(const QString &projectPath)
{
   auto projects = QSettings::value("recentProjects", QStringList()).toStringList();
   auto timesUsed = QSettings::value("recentProjectsCount", QString()).toList();
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

   QMap<int, QString> projectOrderedByUse;

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
