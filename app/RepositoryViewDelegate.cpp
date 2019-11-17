#include "RepositoryViewDelegate.h"

#include <git.h>
#include <lanes.h>
#include <CommitInfo.h>
#include <CommitHistoryColumns.h>
#include <CommitHistoryView.h>
#include <CommitHistoryModel.h>

#include <QSortFilterProxyModel>
#include <QPainter>

static const int COLORS_NUM = 8;
static const int MIN_VIEW_WIDTH_PX = 480;

RepositoryViewDelegate::RepositoryViewDelegate(QSharedPointer<Git> git, CommitHistoryView *view)
   : QStyledItemDelegate()
   , mGit(git)
   , mView(view)
{
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

   static QColor const &lanePenColor = QPalette().color(QPalette::WindowText);
   static QPen lanePen(lanePenColor, 2); // fast path here

   // arc
   switch (type)
   {
      case LaneType::JOIN:
      case LaneType::JOIN_R:
      case LaneType::HEAD:
      case LaneType::HEAD_R: {
         QConicalGradient gradient(x1, 2 * h, 225);
         gradient.setColorAt(0.375, col);
         gradient.setColorAt(0.625, activeCol);
         lanePen.setBrush(col);
         p->setPen(lanePen);
         p->drawArc(m, h, angleWidthRight, angleHeightUp, 0 * 16, spanAngle);
         break;
      }
      case LaneType::JOIN_L: {
         QConicalGradient gradient(2, 2 * h, 315);
         gradient.setColorAt(0.375, activeCol);
         gradient.setColorAt(0.625, col);
         lanePen.setBrush(col);
         p->setPen(lanePen);
         p->drawArc(m, h, angleWidthLeft, angleHeightUp, 90 * 16, spanAngle);
         break;
      }
      case LaneType::TAIL:
      case LaneType::TAIL_R: {
         QConicalGradient gradient(x1, 0, 135);
         gradient.setColorAt(0.375, activeCol);
         gradient.setColorAt(0.625, col);
         lanePen.setBrush(col);
         p->setPen(lanePen);
         p->drawArc(m, h, angleWidthRight, angleHeightDown, 270 * 16, spanAngle);
         break;
      }
      default:
         break;
   }

   lanePen.setColor(isWip ? activeCol : col);
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
      case LaneType::MERGE_FORK_L: {
         isCommit = true;
         QPen pen(mergeColor, 2);
         p->setPen(col);
         p->setBrush(col);
         p->drawEllipse(m - r + 2, h - r + 2, 8, 8);
         if (laneHeadPresent)
         {
            pen = QPen(mergeColor, 1.5);
            p->setPen(pen);
            p->drawArc(m - r + 4, h - r + 2, 8, 8, 16 * 270, 16 * 180);
         }
      }
      break;
      case LaneType::ACTIVE: {
         isCommit = true;
         QPen pen(col, 2);
         p->setPen(pen);
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
         p->drawLine(x1 + (isCommit ? 7 : 0), h, m, h);
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

      p->setPen(QColor("white"));
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

void RepositoryViewDelegate::paintGraph(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
   static const QColor colors[COLORS_NUM] = { QPalette().color(QPalette::WindowText), QColor("#FF5555") /* red */,
                                              QColor("#579BD5") /* blue */,           QColor("#8dc944") /* green */,
                                              QColor("#FFB86C") /* orange */,         QColor("#848484") /* grey */,
                                              QColor("#FF79C6") /* pink */,           QColor("#CD9077") /* pastel */ };

   const auto row = mView->hasActiveFiler()
       ? dynamic_cast<QSortFilterProxyModel *>(mView->model())->mapToSource(index).row()
       : index.row();

   const auto r = mGit->getCommitInfoByRow(row);

   if (r.sha().isEmpty())
      return;

   p->save();
   p->setClipRect(opt.rect, Qt::IntersectClip);
   p->translate(opt.rect.topLeft());

   const QVector<LaneType> &lanes(r.lanes);
   auto laneNum = lanes.count();
   auto activeLane = 0;

   for (int i = 0; i < laneNum && !mView->hasActiveFiler(); i++)
   {
      if (isActive(lanes[i]))
      {
         activeLane = i;
         break;
      }
   }

   auto x1 = 0;
   // const auto maxWidth = opt.rect.width();
   const auto activeColor = colors[activeLane % COLORS_NUM];
   auto back = colors[(laneNum - 1) % COLORS_NUM];
   auto isSet = false;
   auto laneHeadPresent = false;

   for (auto i = laneNum - 1, x2 = LANE_WIDTH * laneNum; i >= 0; --i, x2 -= LANE_WIDTH)
   {
      if (!laneHeadPresent)
      {
         laneHeadPresent = i < laneNum - 1
             && (lanes[i + 1] == LaneType::HEAD || lanes[i + 1] == LaneType::HEAD_L
                 || lanes[i + 1] == LaneType::HEAD_R);
      }

      x1 = x2 - LANE_WIDTH;

      auto ln = mView->hasActiveFiler() ? LaneType::ACTIVE : lanes[i];

      if (ln != LaneType::EMPTY)
      {
         QColor color;
         if (i == activeLane)
         {
            if (r.sha() == ZERO_SHA && !mGit->isNothingToCommit())
               color = QColor("#D89000");
            else
               color = activeColor;
         }
         else
            color = colors[i % COLORS_NUM];

         switch (ln)
         {
            case LaneType::HEAD_L:
            case LaneType::HEAD_R:
            case LaneType::TAIL_L:
            case LaneType::TAIL_R:
               if (!isSet)
               {
                  isSet = true;
                  back = color;
               }
               break;
            default:
               break;
         }
         paintGraphLane(p, ln, laneHeadPresent, x1, x2, color, activeColor, back, r.sha() == ZERO_SHA);

         if (mView->hasActiveFiler())
            break;
      }
   }
   p->restore();
}

void RepositoryViewDelegate::paintWip(QPainter *painter, QStyleOptionViewItem opt) const
{
   opt.font.setBold(true);

   const auto name = QString("WIP");
   const auto textBoundingRect = QFontMetrics(opt.font).boundingRect(name);
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
   const auto sha = mGit->getCommitInfoByRow(index.row()).sha();

   if (sha.isEmpty())
      return;

   auto offset = 0;

   if (mGit->checkRef(sha) > 0 && !mView->hasActiveFiler())
      paintTagBranch(p, opt, offset, sha);

   auto newOpt = opt;
   newOpt.rect.setX(opt.rect.x() + offset + 5);

   QFontMetrics fm(newOpt.font);

   p->setFont(newOpt.font);
   p->setPen(QColor("white"));
   p->drawText(newOpt.rect, fm.elidedText(index.data().toString(), Qt::ElideRight, newOpt.rect.width()),
               QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

void RepositoryViewDelegate::paintTagBranch(QPainter *painter, QStyleOptionViewItem o, int &startPoint,
                                            const QString &sha) const
{
   QMap<QString, QColor> markValues;
   auto ref_types = mGit->checkRef(sha);
   const auto currentBranch = mGit->getCurrentBranchName();

   if (ref_types != 0)
   {
      if (ref_types & Git::CUR_BRANCH && currentBranch.isEmpty())
         markValues.insert("detached", QColor("#851e3e"));

      const auto localBranches = mGit->getRefNames(sha, Git::BRANCH);
      for (auto branch : localBranches)
         markValues.insert(branch, branch == currentBranch ? QColor("#005b96") : QColor("#6497b1"));

      const auto remoteBranches = mGit->getRefNames(sha, Git::RMT_BRANCH);
      for (auto branch : remoteBranches)
         markValues.insert(branch, QColor("#011f4b"));

      const auto tags = mGit->getRefNames(sha, Git::TAG);
      for (auto tag : tags)
         markValues.insert(tag, QColor("#dec3c3"));

      const auto refs = mGit->getRefNames(sha, Git::REF);
      for (auto ref : refs)
         markValues.insert(ref, QColor("#FF5555"));
   }

   const auto showMinimal = o.rect.width() <= MIN_VIEW_WIDTH_PX;
   const int mark_spacing = 5; // Space between markers in pixels
   const auto mapEnd = markValues.constEnd();

   for (auto mapIt = markValues.constBegin(); mapIt != mapEnd; ++mapIt)
   {
      const auto isCurrentSpot = (mapIt.key() == "detached" && currentBranch.isEmpty()) || mapIt.key() == currentBranch;
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
