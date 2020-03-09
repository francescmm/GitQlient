#include "RepositoryViewDelegate.h"

#include <GitQlientStyles.h>
#include <lanes.h>
#include <CommitInfo.h>
#include <CommitHistoryColumns.h>
#include <CommitHistoryView.h>
#include <CommitHistoryModel.h>
#include <RevisionsCache.h>
#include <GitBase.h>

#include <QSortFilterProxyModel>
#include <QPainter>

static const int MIN_VIEW_WIDTH_PX = 480;

RepositoryViewDelegate::RepositoryViewDelegate(const QSharedPointer<RevisionsCache> &cache,
                                               const QSharedPointer<GitBase> &git, CommitHistoryView *view)
   : mCache(cache)
   , mGit(git)
   , mView(view)
{
}

void RepositoryViewDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
   p->setRenderHints(QPainter::Antialiasing);

   QStyleOptionViewItem newOpt(opt);
   newOpt.font.setPointSize(9);

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

   if (index.column() == static_cast<int>(CommitHistoryColumns::GRAPH))
      paintGraph(p, newOpt, index);
   else if (index.column() == static_cast<int>(CommitHistoryColumns::LOG))
      paintLog(p, newOpt, index);
   else
   {
      p->setPen(GitQlientStyles::getTextColor());
      newOpt.rect.setX(newOpt.rect.x() + 10);

      auto text = index.data().toString();

      if (index.column() == static_cast<int>(CommitHistoryColumns::SHA))
      {
         newOpt.font.setPointSize(10);
         newOpt.font.setFamily("Ubuntu Mono");
         text = text.left(8);
      }

      QFontMetrics fm(newOpt.font);
      p->setFont(newOpt.font);
      p->drawText(newOpt.rect, fm.elidedText(text, Qt::ElideRight, newOpt.rect.width()),
                  QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
   }
}

QSize RepositoryViewDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
   return QSize(LANE_WIDTH, ROW_HEIGHT);
}

void RepositoryViewDelegate::paintGraphLane(QPainter *p, LaneType type, bool laneHeadPresent, int x1, int x2,
                                            const QColor &col, const QColor &activeCol, const QColor &mergeColor,
                                            bool isWip) const
{
   const auto padding = 2;
   x1 += padding;
   x2 += padding;

   const auto h = ROW_HEIGHT / 2;
   const auto m = (x1 + x2) / 2;
   const auto r = (x2 - x1) * 1 / 3;
   const auto spanAngle = 90 * 16;
   const auto angleWidthRight = 2 * (x1 - m);
   const auto angleWidthLeft = 2 * (x2 - m);
   const auto angleHeightUp = 2 * h;
   const auto angleHeightDown = 2 * -h;

   static QPen lanePen(GitQlientStyles::getTextColor(), 2); // fast path here

   // arc
   lanePen.setBrush(col);
   p->setPen(lanePen);

   switch (type)
   {
      case LaneType::JOIN:
      case LaneType::JOIN_R:
      case LaneType::HEAD:
      case LaneType::HEAD_R: {
         p->drawArc(m, h, angleWidthRight, angleHeightUp, 0 * 16, spanAngle);
         break;
      }
      case LaneType::JOIN_L: {
         p->drawArc(m, h, angleWidthLeft, angleHeightUp, 90 * 16, spanAngle);
         break;
      }
      case LaneType::TAIL:
      case LaneType::TAIL_R: {
         p->drawArc(m, h, angleWidthRight, angleHeightDown, 270 * 16, spanAngle);
         break;
      }
      default:
         break;
   }

   if (isWip)
   {
      lanePen.setColor(activeCol);
      p->setPen(lanePen);
   }

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
         p->drawLine(m, 0, m, 2 * h);
         break;
      case LaneType::HEAD_L:
      case LaneType::BRANCH:
         p->drawLine(m, h, m, 2 * h);
         break;
      case LaneType::TAIL_L:
      case LaneType::INITIAL:
         p->drawLine(m, 0, m, h);
         break;
      default:
         break;
   }

   // center symbol, e.g. rect or ellipse
   auto isCommit = false;
   switch (type)
   {
      case LaneType::HEAD:
      case LaneType::INITIAL:
      case LaneType::BRANCH:
      case LaneType::MERGE_FORK:
      case LaneType::MERGE_FORK_R:
         isCommit = true;
         p->setPen(QPen(mergeColor, 2));
         p->setBrush(col);
         p->drawEllipse(m - r + 2, h - r + 2, 8, 8);
         break;
      case LaneType::MERGE_FORK_L:
         isCommit = true;
         p->setPen(QPen(laneHeadPresent ? mergeColor : col, 2));
         p->setBrush(col);
         p->drawEllipse(m - r + 2, h - r + 2, 8, 8);
         break;
      case LaneType::ACTIVE: {
         isCommit = true;
         p->setPen(QPen(col, 2));
         p->setBrush(QColor(isWip ? col : "#2E2F30"));
         p->drawEllipse(m - r + 2, h - r + 2, 8, 8);
      }
      break;
      default:
         break;
   }

   lanePen.setColor(mergeColor);
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
         p->drawLine(x1 + (isCommit ? 10 : 0), h, x2, h);
         break;
      case LaneType::MERGE_FORK_R:
      case LaneType::BOUNDARY_R:
         p->drawLine(x1 + (isCommit ? 0 : 10), h, m - (isCommit ? 6 : 0), h);
         break;
      case LaneType::MERGE_FORK_L:
      case LaneType::HEAD_L:
      case LaneType::TAIL_L:
      case LaneType::BOUNDARY_L:
         p->drawLine(m + (isCommit ? 6 : 0), h, x2, h);
         break;
      default:
         break;
   }
}

void RepositoryViewDelegate::paintGraph(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
   const auto row = mView->hasActiveFilter()
       ? dynamic_cast<QSortFilterProxyModel *>(mView->model())->mapToSource(index).row()
       : index.row();

   const auto r = mCache->getCommitInfoByRow(row);

   if (r.sha().isEmpty())
      return;

   p->save();
   p->setClipRect(opt.rect, Qt::IntersectClip);
   p->translate(opt.rect.topLeft());

   const QVector<LaneType> &lanes(r.lanes);
   auto laneNum = lanes.count();
   auto activeLane = 0;

   for (int i = 0; i < laneNum && !mView->hasActiveFilter(); i++)
   {
      if (isActive(lanes[i]))
      {
         activeLane = i;
         break;
      }
   }

   auto x1 = 0;
   // const auto maxWidth = opt.rect.width();
   const auto activeColor = GitQlientStyles::getBranchColorAt(activeLane % GitQlientStyles::getTotalBranchColors());
   auto back = GitQlientStyles::getBranchColorAt((laneNum - 1) % GitQlientStyles::getTotalBranchColors());
   auto isSet = false;
   auto laneHeadPresent = false;

   for (auto i = laneNum - 1, x2 = LANE_WIDTH * laneNum; i >= 0; --i, x2 -= LANE_WIDTH)
   {
      if (!laneHeadPresent)
      {
         laneHeadPresent = i < laneNum - 1
             && (lanes[i + 1] == LaneType::HEAD || lanes[i + 1] == LaneType::HEAD_L || lanes[i + 1] == LaneType::HEAD_R
                 || lanes[i + 1] == LaneType::JOIN_L || lanes[i + 1] == LaneType::JOIN_R);
      }

      x1 = x2 - LANE_WIDTH;

      auto ln = mView->hasActiveFilter() ? LaneType::ACTIVE : lanes[i];

      if (ln != LaneType::EMPTY)
      {
         QColor color;
         if (i == activeLane)
         {
            if (r.sha() == CommitInfo::ZERO_SHA && !mCache->pendingLocalChanges())
               color = QColor("#D89000");
            else
               color = activeColor;
         }
         else
            color = GitQlientStyles::getBranchColorAt(i % GitQlientStyles::getTotalBranchColors());

         switch (ln)
         {
            case LaneType::HEAD_L:
            case LaneType::HEAD_R:
            case LaneType::TAIL_L:
            case LaneType::TAIL_R:
            case LaneType::MERGE_FORK_L:
            case LaneType::JOIN_R:
               if (!isSet)
               {
                  isSet = true;
                  back = color;
               }
               break;
            case LaneType::MERGE_FORK_R:
            case LaneType::JOIN_L:
               if (!isSet)
               {
                  for (auto laneCount = 0; laneCount < i; ++laneCount)
                  {
                     if (lanes[laneCount] == LaneType::JOIN_L)
                     {
                        back = GitQlientStyles::getBranchColorAt(laneCount % GitQlientStyles::getTotalBranchColors());
                        isSet = true;
                        break;
                     }
                  }
               }
               break;
            default:
               break;
         }
         paintGraphLane(p, ln, laneHeadPresent, x1, x2, color, activeColor, back, r.sha() == CommitInfo::ZERO_SHA);

         if (mView->hasActiveFilter())
            break;
      }
   }
   p->restore();
}

void RepositoryViewDelegate::paintLog(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
   const auto sha = mCache->getCommitInfoByRow(index.row()).sha();

   if (sha.isEmpty())
      return;

   auto offset = 0;

   if (mCache->checkRef(sha) > 0 && !mView->hasActiveFilter())
   {
      offset = 5;
      paintTagBranch(p, opt, offset, sha);
   }

   auto newOpt = opt;
   newOpt.rect.setX(opt.rect.x() + offset + 5);

   QFontMetrics fm(newOpt.font);

   p->setFont(newOpt.font);
   p->setPen(GitQlientStyles::getTextColor());
   p->drawText(newOpt.rect, fm.elidedText(index.data().toString(), Qt::ElideRight, newOpt.rect.width()),
               QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

void RepositoryViewDelegate::paintTagBranch(QPainter *painter, QStyleOptionViewItem o, int &startPoint,
                                            const QString &sha) const
{
   QMap<QString, QColor> markValues;
   auto ref_types = mCache->checkRef(sha);
   const auto currentBranch = mGit->getCurrentBranch();

   if (ref_types != 0)
   {
      if (ref_types & CUR_BRANCH && (currentBranch.isEmpty() || currentBranch == "HEAD"))
         markValues.insert("detached", GitQlientStyles::getDetachedColor());

      const auto localBranches = mCache->getRefNames(sha, BRANCH);
      for (const auto &branch : localBranches)
         markValues.insert(branch,
                           branch == currentBranch ? GitQlientStyles::getCurrentBranchColor()
                                                   : GitQlientStyles::getLocalBranchColor());

      const auto remoteBranches = mCache->getRefNames(sha, RMT_BRANCH);
      for (const auto &branch : remoteBranches)
         markValues.insert(branch, QColor("#011f4b"));

      const auto tags = mCache->getRefNames(sha, TAG);
      for (const auto &tag : tags)
         markValues.insert(tag, GitQlientStyles::getTagColor());

      const auto refs = mCache->getRefNames(sha, REF);
      for (const auto &ref : refs)
         markValues.insert(ref, GitQlientStyles::getRefsColor());
   }

   const auto showMinimal = o.rect.width() <= MIN_VIEW_WIDTH_PX;
   const int mark_spacing = 5; // Space between markers in pixels
   const auto mapEnd = markValues.constEnd();

   for (auto mapIt = markValues.constBegin(); mapIt != mapEnd; ++mapIt)
   {
      const auto isCurrentSpot = mapIt.key() == "detached" || mapIt.key() == currentBranch;
      o.font.setBold(isCurrentSpot);

      const auto nameToDisplay = showMinimal ? QString(". . .") : mapIt.key();
      QFontMetrics fm(o.font);
      const auto textBoundingRect = fm.boundingRect(nameToDisplay);
      const int textPadding = 3;
      const auto rectWidth = textBoundingRect.width() + 2 * textPadding;

      painter->save();
      painter->setRenderHint(QPainter::Antialiasing);
      painter->setPen(QPen(mapIt.value(), 2));
      QPainterPath path;
      path.addRoundedRect(QRectF(o.rect.x() + startPoint, o.rect.y() + 4, rectWidth, ROW_HEIGHT - 8), 1, 1);
      painter->fillPath(path, mapIt.value());
      painter->drawPath(path);

      // TODO: Fix this with a nicer way
      painter->setPen(QColor(mapIt.value() == QColor("#dec3c3") ? QString("#000000") : QString("#FFFFFF")));

      const auto fontRect = textBoundingRect.height();
      const auto y = o.rect.y() + ROW_HEIGHT - (ROW_HEIGHT - fontRect) + 2;
      painter->setFont(o.font);
      painter->drawText(o.rect.x() + startPoint + textPadding, y, nameToDisplay);
      painter->restore();

      startPoint += rectWidth + mark_spacing;
   }
}
