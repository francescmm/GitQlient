#include "RepositoryViewDelegate.h"

#include <git.h>
#include <lanes.h>
#include <RevisionsCache.h>
#include <Revision.h>
#include <RepositoryModelColumns.h>

#include <QPainter>

static const int COLORS_NUM = 8;
static const int MIN_VIEW_WIDTH_PX = 480;

namespace
{
/** Iterator over all refnames of a given sha.
 *  References are traversed in following order:
 *  detached (name empty), local branches, remote branches, tags, other refs
 */
class RefNameIterator
{
   const QString sha;
   QSharedPointer<Git> mGit;
   uint ref_types; // all reference types associated with sha
   int cur_state; // state indicating the currently processed ref type
   QStringList ref_names; // ref_names of current type
   QStringList::const_iterator cur_name;
   QString cur_branch;

public:
   RefNameIterator(QSharedPointer<Git> git, const QString &sha);
   bool valid() const { return cur_state != -1; }
   QString name() const { return *cur_name; }
   int type() { return cur_state; }
   bool isCurrentBranch() { return *cur_name == cur_branch; }

   void next();
};

RefNameIterator::RefNameIterator(QSharedPointer<Git> git, const QString &sha)
   : sha(sha)
   , mGit(git)
   , cur_state(0)
   , cur_branch(mGit->getCurrentBranchName())
{
   ref_types = mGit->checkRef(sha);
   if (ref_types == 0)
   {
      cur_state = -1; // indicates end
      return;
   }

   // initialize dummy string list
   ref_names << "";
   cur_name = ref_names.begin();

   // detached ?
   if ((ref_types & Git::CUR_BRANCH) && cur_branch.isEmpty())
   {
      // indicate detached state with type() == 0 and empty ref name
      cur_branch = *cur_name;
   }
   else
   { // advance to first real ref name
      next();
   }
}

void RefNameIterator::next()
{
   ++cur_name;

   // switch to next ref type if required
   while (valid() && cur_name == ref_names.end())
   {
      switch (cur_state)
      {
         case 0:
            cur_state = Git::BRANCH;
            break;
         case Git::BRANCH:
            cur_state = Git::RMT_BRANCH;
            break;
         case Git::RMT_BRANCH:
            cur_state = Git::TAG;
            break;
         case Git::TAG:
            cur_state = Git::REF;
            break;
         default:
            cur_state = -1; // indicate end
      }
      ref_names = mGit->getRefNames(sha, (Git::RefType)cur_state);
      cur_name = ref_names.begin();
   }
}

}

RepositoryViewDelegate::RepositoryViewDelegate(QSharedPointer<Git> git, QSharedPointer<RevisionsCache> revCache)
   : QStyledItemDelegate()
   , mGit(git)
   , mRevCache(revCache)
{
}

void RepositoryViewDelegate::diffTargetChanged(int row)
{
   if (diffTargetRow != row)
   {
      diffTargetRow = row;
      emit updateView();
   }
}

static QColor blend(const QColor &col1, const QColor &col2, int amount = 128)
{
   // Returns ((256 - amount)*col1 + amount*col2) / 256;
   return QColor(((256 - amount) * col1.red() + amount * col2.red()) / 256,
                 ((256 - amount) * col1.green() + amount * col2.green()) / 256,
                 ((256 - amount) * col1.blue() + amount * col2.blue()) / 256);
}

void RepositoryViewDelegate::paintGraphLane(QPainter *p, LaneType type, int x1, int x2, const QColor &col,
                                            const QColor &activeCol, const QBrush &back) const
{
   const int padding = 2;
   x1 += padding;
   x2 += padding;

   int h = ROW_HEIGHT / 2;
   int m = (x1 + x2) / 2;
   int r = (x2 - x1) * 1 / 3;
   int d = 2 * r;

#define P_CENTER m, h
#define P_0 x2, h // >
#define P_90 m, 0 // ^
#define P_180 x1, h // <
#define P_270 m, 2 * h // v
#define DELTA_UR 2 * (x1 - m), 2 * h, 0 * 16, 90 * 16 // -,
#define DELTA_DR 2 * (x1 - m), 2 * -h, 270 * 16, 90 * 16 // -'
#define DELTA_UL 2 * (x2 - m), 2 * h, 90 * 16, 90 * 16 //  ,-
#define DELTA_DL 2 * (x2 - m), 2 * -h, 180 * 16, 90 * 16 //  '-
#define CENTER_UR x1, 2 * h, 225
#define CENTER_DR x1, 0, 135
#define CENTER_UL x2, 2 * h, 315
#define CENTER_DL x2, 0, 45
#define R_CENTER m - r, h - r, d, d

   static QColor const &lanePenColor = QPalette().color(QPalette::WindowText);
   static QPen lanePen(lanePenColor, 2); // fast path here

   // arc
   switch (type)
   {
      case LaneType::JOIN:
      case LaneType::JOIN_R:
      case LaneType::HEAD:
      case LaneType::HEAD_R: {
         QConicalGradient gradient(CENTER_UR);
         gradient.setColorAt(0.375, col);
         gradient.setColorAt(0.625, activeCol);
         lanePen.setBrush(gradient);
         p->setPen(lanePen);
         p->drawArc(P_CENTER, DELTA_UR);
         break;
      }
      case LaneType::JOIN_L: {
         QConicalGradient gradient(CENTER_UL);
         gradient.setColorAt(0.375, activeCol);
         gradient.setColorAt(0.625, col);
         lanePen.setBrush(gradient);
         p->setPen(lanePen);
         p->drawArc(P_CENTER, DELTA_UL);
         break;
      }
      case LaneType::TAIL:
      case LaneType::TAIL_R: {
         QConicalGradient gradient(CENTER_DR);
         gradient.setColorAt(0.375, activeCol);
         gradient.setColorAt(0.625, col);
         lanePen.setBrush(gradient);
         p->setPen(lanePen);
         p->drawArc(P_CENTER, DELTA_DR);
         break;
      }
      default:
         break;
   }

   lanePen.setColor(col);
   p->setPen(lanePen);

   // vertical line
   switch (type)
   {
      case LaneType::ACTIVE:
      case LaneType::NOT_ACTIVE:
      case LaneType::MERGE_FORK:
      case LaneType::MERGE_FORK_R:
      case LaneType::MERGE_FORK_L:
      case LaneType::JOIN:
      case LaneType::JOIN_R:
      case LaneType::JOIN_L:
      case LaneType::CROSS:
         p->drawLine(P_90, P_270);
         break;
      case LaneType::HEAD_L:
      case LaneType::BRANCH:
         p->drawLine(P_CENTER, P_270);
         break;
      case LaneType::TAIL_L:
      case LaneType::INITIAL:
      case LaneType::BOUNDARY:
      case LaneType::BOUNDARY_C:
      case LaneType::BOUNDARY_R:
      case LaneType::BOUNDARY_L:
         p->drawLine(P_90, P_CENTER);
         break;
      default:
         break;
   }

   lanePen.setColor(activeCol);
   p->setPen(lanePen);

   // horizontal line
   switch (type)
   {
      case LaneType::MERGE_FORK:
      case LaneType::JOIN:
      case LaneType::HEAD:
      case LaneType::TAIL:
      case LaneType::CROSS:
      case LaneType::CROSS_EMPTY:
      case LaneType::BOUNDARY_C:
         p->drawLine(P_180, P_0);
         break;
      case LaneType::MERGE_FORK_R:
      case LaneType::BOUNDARY_R:
         p->drawLine(P_180, P_CENTER);
         break;
      case LaneType::MERGE_FORK_L:
      case LaneType::HEAD_L:
      case LaneType::TAIL_L:
      case LaneType::BOUNDARY_L:
         p->drawLine(P_CENTER, P_0);
         break;
      default:
         break;
   }

   // center symbol, e.g. rect or ellipse
   switch (type)
   {
      case LaneType::ACTIVE:
      case LaneType::INITIAL:
      case LaneType::BRANCH:
         p->setPen(Qt::black);
         p->setBrush(col);
         p->drawEllipse(R_CENTER);
         break;
      case LaneType::MERGE_FORK:
      case LaneType::MERGE_FORK_R:
      case LaneType::MERGE_FORK_L:
         p->setPen(Qt::black);
         p->setBrush(col);
         p->drawRect(R_CENTER);
         break;
      case LaneType::UNAPPLIED:
         // Red minus sign
         p->setPen(Qt::NoPen);
         p->setBrush(Qt::red);
         p->drawRect(m - r, h - 1, d, 2);
         break;
      case LaneType::APPLIED:
         // Green plus sign
         p->setPen(Qt::NoPen);
         p->setBrush(Qt::darkGreen);
         p->drawRect(m - r, h - 1, d, 2);
         p->drawRect(m - 1, h - r, 2, d);
         break;
      case LaneType::BOUNDARY:
         p->setPen(Qt::black);
         p->setBrush(back);
         p->drawEllipse(R_CENTER);
         break;
      case LaneType::BOUNDARY_C:
      case LaneType::BOUNDARY_R:
      case LaneType::BOUNDARY_L:
         p->setPen(Qt::black);
         p->setBrush(back);
         p->drawRect(R_CENTER);
         break;
      default:
         break;
   }
#undef P_CENTER
#undef P_0
#undef P_90
#undef P_180
#undef P_270
#undef DELTA_UR
#undef DELTA_DR
#undef DELTA_UL
#undef DELTA_DL
#undef CENTER_UR
#undef CENTER_DR
#undef CENTER_UL
#undef CENTER_DL
#undef R_CENTER
}

void RepositoryViewDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
   p->setRenderHints(QPainter::Antialiasing);

   QStyleOptionViewItem newOpt(opt);

   if (newOpt.state & QStyle::State_Selected)
   {
      QColor c("#404142");
      c.setAlphaF(0.75);
      p->fillRect(newOpt.rect, c);
   }
   else if (newOpt.state & QStyle::State_MouseOver)
   {
      QColor c("#404142");
      c.setAlphaF(0.4);
      p->fillRect(newOpt.rect, c);
   }

   if (index.column() == static_cast<int>(RepositoryModelColumns::GRAPH))
      return paintGraph(p, newOpt, index);

   if (index.column() == static_cast<int>(RepositoryModelColumns::LOG))
      return paintLog(p, newOpt, index);
}

void RepositoryViewDelegate::paintGraph(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &i) const
{
   static const QColor &baseColor = QPalette().color(QPalette::WindowText);
   static const QColor colors[COLORS_NUM] = { baseColor,
                                              QColor("#FF5555") /* red */,
                                              QColor("#579BD5") /* blue */,
                                              QColor("#8dc944") /* green */,
                                              QColor("#FFB86C") /* orange */,
                                              QColor("#848484") /* grey */,
                                              QColor("#FF79C6") /* pink */,
                                              QColor("#CD9077") /* pastel */ };
   const auto r = mRevCache->revLookup(i.row());

   if (!r)
      return;

   if (r->isDiffCache && !mGit->isNothingToCommit())
   {
      paintWip(p, opt);
      return;
   }

   p->save();
   p->setClipRect(opt.rect, Qt::IntersectClip);
   p->translate(opt.rect.topLeft());

   // calculate lanes
   if (r->lanes.count() == 0)
      mGit->setLane(r->sha());

   QBrush back = opt.palette.base();
   const QVector<LaneType> &lanes(r->lanes);
   auto laneNum = lanes.count();
   auto activeLane = 0;
   for (int i = 0; i < laneNum; i++)
      if (isActive(lanes[i]))
      {
         activeLane = i;
         break;
      }

   int x1 = 0, x2 = 0;
   int maxWidth = opt.rect.width();
   QColor activeColor = colors[activeLane % COLORS_NUM];
   if (opt.state & QStyle::State_Selected)
      activeColor = blend(activeColor, opt.palette.highlightedText().color(), 208);
   for (auto i = 0; i < laneNum && x2 < maxWidth; i++)
   {

      x1 = x2;
      x2 += LANE_WIDTH;

      auto ln = lanes[i];
      if (ln != LaneType::EMPTY)
      {
         QColor color = i == activeLane ? activeColor : colors[i % COLORS_NUM];
         paintGraphLane(p, ln, x1, x2, color, activeColor, back);
      }
   }
   p->restore();
}

void RepositoryViewDelegate::paintWip(QPainter *painter, QStyleOptionViewItem opt) const
{
   opt.font.setBold(true);

   const auto name = QString("WIP");
   QFontMetrics fm(opt.font);
   const auto textBoundingRect = fm.boundingRect(name);
   const auto fontRect = textBoundingRect.height();

   painter->save();
   painter->fillRect(opt.rect.x(), opt.rect.y(), opt.rect.width(), ROW_HEIGHT, QColor("#d89000"));
   painter->setPen(QColor("#FFFFFF"));

   const auto x = opt.rect.x() + (opt.rect.width() - textBoundingRect.width()) / 2;
   const auto y = opt.rect.y() + ROW_HEIGHT - (ROW_HEIGHT - fontRect) + 2;

   painter->setFont(opt.font);
   painter->drawText(x, y, name);
   painter->restore();
}

void RepositoryViewDelegate::paintLog(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
   int row = index.row();
   const auto r = mRevCache->revLookup(row);

   if (!r)
      return;

   auto offset = 0;

   if (mGit->checkRef(r->sha()) > 0)
      paintTagBranch(p, opt, offset, r->sha());

   auto newOpt = opt;
   newOpt.rect.setX(opt.rect.x() + offset + 5);

   QFontMetrics fm(newOpt.font);
   fm.height();

   p->setPen(QColor("white"));
   p->drawText(newOpt.rect, fm.elidedText(index.data().toString(), Qt::ElideRight, newOpt.rect.width()),
               QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

void RepositoryViewDelegate::paintTagBranch(QPainter *painter, QStyleOptionViewItem o, int &startPoint,
                                            const QString &sha) const
{
   const auto showMinimal = o.rect.width() <= MIN_VIEW_WIDTH_PX;
   const int mark_spacing = 5; // Space between markers in pixels

   for (RefNameIterator it(mGit, sha); it.valid(); it.next())
   {
      auto name = it.name();

      QColor clr;

      switch (it.type())
      {
         case 0:
            name = "detached";
            clr = QColor("#851e3e");
            break;
         case Git::BRANCH:
            clr = it.isCurrentBranch() ? QColor("#005b96") : QColor("#6497b1");
            break;
         case Git::RMT_BRANCH:
            clr = QColor("#011f4b");
            break;
         case Git::TAG:
            clr = QColor("#dec3c3");
            break;
         case Git::REF:
            clr = QColor("#FF5555");
            break;
      }

      o.font.setBold(it.isCurrentBranch());

      const auto nameToDisplay = showMinimal ? QString(". . .") : name;
      QFontMetrics fm(o.font);
      const auto textBoundingRect = fm.boundingRect(nameToDisplay);
      const int textPadding = 10;
      const auto rectWidth = textBoundingRect.width() + 2 * textPadding;

      painter->save();
      painter->fillRect(o.rect.x() + startPoint, o.rect.y(), rectWidth, ROW_HEIGHT, clr);
      painter->setPen(QColor(it.type() == Git::TAG ? QString("#000000") : QString("#FFFFFF")));

      const auto fontRect = textBoundingRect.height();
      const auto y = o.rect.y() + ROW_HEIGHT - (ROW_HEIGHT - fontRect) + 2;
      painter->setFont(o.font);
      painter->drawText(o.rect.x() + startPoint + textPadding, y, nameToDisplay);
      painter->restore();

      startPoint += rectWidth + mark_spacing;
   }
}
