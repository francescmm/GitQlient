/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#ifndef COMMON_H
#define COMMON_H

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QHash>
#include <QLatin1String>
#include <QSet>
#include <QVariant>
#include <QVector>
#include <QTextBrowser>

class QDataStream;
class QProcess;
class QSplitter;
class QWidget;
class QString;

uint qHash(const QString &); // optimized custom hash for sha strings

namespace QGit
{

// tab pages
enum TabType
{
   TAB_REV
};

// graph elements
enum LaneType
{
   EMPTY,
   ACTIVE,
   NOT_ACTIVE,
   MERGE_FORK,
   MERGE_FORK_R,
   MERGE_FORK_L,
   JOIN,
   JOIN_R,
   JOIN_L,
   HEAD,
   HEAD_R,
   HEAD_L,
   TAIL,
   TAIL_R,
   TAIL_L,
   CROSS,
   CROSS_EMPTY,
   INITIAL,
   BRANCH,
   UNAPPLIED,
   APPLIED,
   BOUNDARY,
   BOUNDARY_C, // corresponds to MERGE_FORK
   BOUNDARY_R, // corresponds to MERGE_FORK_R
   BOUNDARY_L, // corresponds to MERGE_FORK_L

   LANE_TYPES_NUM
};
const int COLORS_NUM = 8;

// graph helpers
inline bool isHead(int x)
{
   return (x == HEAD || x == HEAD_R || x == HEAD_L);
}
inline bool isTail(int x)
{
   return (x == TAIL || x == TAIL_R || x == TAIL_L);
}
inline bool isJoin(int x)
{
   return (x == JOIN || x == JOIN_R || x == JOIN_L);
}
inline bool isFreeLane(int x)
{
   return (x == NOT_ACTIVE || x == CROSS || isJoin(x));
}
inline bool isBoundary(int x)
{
   return (x == BOUNDARY || x == BOUNDARY_C || x == BOUNDARY_R || x == BOUNDARY_L);
}
inline bool isMerge(int x)
{
   return (x == MERGE_FORK || x == MERGE_FORK_R || x == MERGE_FORK_L || isBoundary(x));
}
inline bool isActive(int x)
{
   return (x == ACTIVE || x == INITIAL || x == BRANCH || isMerge(x));
}

// custom events
enum EventType
{
   ERROR_EV = 65432,
   ANN_PRG_EV = 65437,
   UPD_DM_EV = 65438,
   UPD_DM_MST_EV = 65439
};

// The big shit
extern QTextBrowser *kErrorLogBrowser;

// initialized at startup according to system wide settings
extern QString GIT_DIR;

// git index parameters
extern const QByteArray ZERO_SHA_BA;
extern const QString ZERO_SHA_RAW;

extern const QString ZERO_SHA;
extern const QString CUSTOM_SHA;
extern const QString ALL_MERGE_FILES;

// QString helpers
const QString toPersistentSha(const QString &, QVector<QByteArray> &);

// misc helpers
bool stripPartialParaghraps(const QByteArray &src, QString *dst, QString *prev);
bool writeToFile(const QString &fileName, const QString &data, bool setExecutable = false);
bool writeToFile(const QString &fileName, const QByteArray &data, bool setExecutable = false);
bool readFromFile(const QString &fileName, QString &data);

// cache file
const uint C_MAGIC = 0xA0B0C0D0;
const int C_VERSION = 15;

extern const QString BAK_EXT;
extern const QString C_DAT_FILE;

// misc
const int MAX_DICT_SIZE = 100003; // must be a prime number see QDict docs
const int MAX_MENU_ENTRIES = 20;
const int MAX_RECENT_REPOS = 7;
extern const QString SCRIPT_EXT;
}

class RevFile
{

   friend class Cache; // to directly load status
   friend class Git;

   // Status information is splitted in a flags vector and in a string
   // vector in 'status' are stored flags according to the info returned
   // by 'git diff-tree' without -C option.
   // In case of a working directory file an IN_INDEX flag is or-ed togheter in
   // case file is present in git index.
   // If file is renamed or copied an entry in 'extStatus' stores the
   // value returned by 'git diff-tree -C' plus source and destination
   // files info.
   // When status of all the files is 'modified' then onlyModified is
   // set, this let us to do some optimization in this common case
   bool onlyModified;
   QVector<int> status;
   QVector<QString> extStatus;

   // prevent implicit C++ compiler defaults
   RevFile(const RevFile &);
   RevFile &operator=(const RevFile &);

public:
   enum StatusFlag
   {
      MODIFIED = 1,
      DELETED = 2,
      NEW = 4,
      RENAMED = 8,
      COPIED = 16,
      UNKNOWN = 32,
      IN_INDEX = 64,
      ANY = 127
   };

   RevFile()
      : onlyModified(true)
   {
   }

   /* This QByteArray keeps indices in some dir and names vectors,
    * defined outside RevFile. Paths are splitted in dir and file
    * name, first all the dirs are listed then the file names to
    * achieve a better compression when saved to disk.
    * A single QByteArray is used instead of two vectors because it's
    * much faster to load from disk when using a QDataStream
    */
   QByteArray pathsIdx;

   int dirAt(int idx) const { return ((const int *)pathsIdx.constData())[idx]; }
   int nameAt(int idx) const { return ((const int *)pathsIdx.constData())[count() + idx]; }

   QVector<int> mergeParent;

   // helper functions
   int count() const { return pathsIdx.size() / (sizeof(int) * 2); }
   bool statusCmp(int idx, StatusFlag sf) const
   {

      return ((onlyModified ? MODIFIED : status.at(static_cast<int>(idx))) & sf);
   }
   const QString extendedStatus(int idx) const
   {
      /*
         rf.extStatus has size equal to position of latest copied/renamed file,
         that could be lower then count(), so we have to explicitly check for
         an out of bound condition.
      */
      return (!extStatus.isEmpty() && idx < extStatus.count() ? extStatus.at(idx) : "");
   }
   const RevFile &operator>>(QDataStream &) const;
   RevFile &operator<<(QDataStream &);
};

#endif
