#include "RepositoryViewDelegate.h"

#include <Colors.h>
#include <CommitHistoryColumns.h>
#include <CommitHistoryModel.h>
#include <CommitHistoryView.h>
#include <Commit.h>
#include <GitBase.h>
#include <GitCache.h>
#include <GitLocal.h>
#include <GitQlientSettings.h>
#include <GitQlientStyles.h>
#include <State.h>
#include <StateType.h>

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QEvent>
#include <QFontDatabase>
#include <QHeaderView>
#include <QPainter>
#include <QPainterPath>
#include <QSortFilterProxyModel>
#include <QToolTip>
#include <QUrl>

using namespace Graph;

RepositoryViewDelegate::RepositoryViewDelegate(const QSharedPointer<GitCache> &cache,
                                               const QSharedPointer<Graph::Cache> &graphCache,
                                               const QSharedPointer<GitBase> &git,
                                               CommitHistoryView *view)
   : mCache(cache)
   , mGraphCache(graphCache)
   , mGit(git)
   , mView(view)
{
}

void RepositoryViewDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
   const auto row = mView->hasActiveFilter()
       ? dynamic_cast<QSortFilterProxyModel *>(mView->model())->mapToSource(index).row()
       : index.row();

   const auto commit = mCache->commitInfo(row);

   if (commit.sha.isEmpty())
      return;

   p->setRenderHints(QPainter::Antialiasing);

   const auto defaultFontSize = GitQlientSettings().globalValue("HistoryView/FontSize", QFontDatabase::systemFont(QFontDatabase::GeneralFont).pointSize()).toInt();
   QStyleOptionViewItem newOpt(opt);
   newOpt.font.setPointSize(defaultFontSize - 1);

   if (newOpt.state & QStyle::State_Selected || newOpt.state & QStyle::State_MouseOver)
   {
      const auto nonGraphColumnBkgColor = newOpt.state & QStyle::State_Selected
          ? GitQlientStyles::getGraphSelectionColor()
          : GitQlientStyles::getGraphHoverColor();

      p->fillRect(newOpt.rect, nonGraphColumnBkgColor);

      if (index.column() == static_cast<int>(CommitHistoryColumns::Graph)
          && (mView->hasActiveFilter() || commit.sha != ZERO_SHA))
      {
         auto color = GitQlientStyles::getBranchColorAt(
             mView->hasActiveFilter() ? 0 : mGraphCache->getSacredTimeline(commit.sha) % GitQlientStyles::getTotalBranchColors());
         color.setAlpha(90);

         newOpt.rect.setWidth(newOpt.rect.width() - 3);

         p->fillRect(newOpt.rect, color);
      }
   }

   if (index.column() == static_cast<int>(CommitHistoryColumns::Graph))
   {
      newOpt.rect.setX(newOpt.rect.x() + 10);
      paintGraph(p, newOpt, commit);
   }
   else
   {
      const auto currentColVisualIndex = mView->header()->visualIndex(index.column());
      const auto firstNonGraphColVisualIndex
          = mView->header()->visualIndex(static_cast<int>(CommitHistoryColumns::Graph)) + 1;

      QColor laneColor;

      if (currentColVisualIndex == firstNonGraphColVisualIndex)
         laneColor = paintBranchHelper(p, newOpt, commit);

      if (index.column() == static_cast<int>(CommitHistoryColumns::Log))
      {
         paintLog(p, newOpt, laneColor, commit);
      }
      else
      {
         p->setPen(GitQlientStyles::getTextColor());
         newOpt.rect.setX(newOpt.rect.x() + 10);

         QTextOption textalignment(Qt::AlignLeft | Qt::AlignVCenter);
         auto text = index.data().toString();

         if (index.column() == static_cast<int>(CommitHistoryColumns::Date))
         {
            textalignment = QTextOption(Qt::AlignRight | Qt::AlignVCenter);
            const auto prev = QDateTime::fromString(mView->indexAbove(index).data().toString(), "dd MMM yyyy hh:mm");
            const auto current = QDateTime::fromString(text, "dd MMM yyyy hh:mm");

            if (current.date() == prev.date())
               text = current.toString("hh:mm");
            else
               text = current.toString("dd MMM yyyy - hh:mm");

            newOpt.rect.setWidth(newOpt.rect.width() - 5);
         }
         else if (index.column() == static_cast<int>(CommitHistoryColumns::Sha))
         {
            newOpt.font.setPointSize(defaultFontSize - 2);
            newOpt.font.setFamily("DejaVu Sans Mono");

            text = commit.sha != ZERO_SHA ? text.left(8) : "";
         }
         else if (index.column() == static_cast<int>(CommitHistoryColumns::Author) && commit.isSigned())
         {
            static const auto size = 15;
            static const auto offset = 5;
            QPixmap pic(QString::fromUtf8(commit.verifiedSignature() ? ":/icons/signed" : ":/icons/unsigned"));
            pic = pic.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            const auto inc = (newOpt.rect.height() - size) / 2;

            p->drawPixmap(QRect(newOpt.rect.x(), newOpt.rect.y() + inc, size, size), pic);

            newOpt.rect.setX(newOpt.rect.x() + size + offset);
         }

         QFontMetrics fm(newOpt.font);
         p->setFont(newOpt.font);

         if (const auto cursorColumn = mView->indexAt(mView->mapFromGlobal(QCursor::pos())).column();
             newOpt.state & QStyle::State_MouseOver && cursorColumn == index.column()
             && cursorColumn == static_cast<int>(CommitHistoryColumns::Sha))
         {
            p->setPen(gitQlientOrange);
         }

         p->drawText(newOpt.rect, fm.elidedText(text, Qt::ElideRight, newOpt.rect.width()), textalignment);
      }
   }
}

QSize RepositoryViewDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
   return QSize(LANE_WIDTH, ROW_HEIGHT);
}

bool RepositoryViewDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                                         const QModelIndex &index)
{
   const auto cursorColumn = mView->indexAt(mView->mapFromGlobal(QCursor::pos())).column();

   if (event->type() == QEvent::MouseButtonPress && cursorColumn == index.column()
       && cursorColumn == static_cast<int>(CommitHistoryColumns::Sha))
   {
      mColumnPressed = cursorColumn;
      return true;
   }
   else if (event->type() == QEvent::MouseButtonRelease && cursorColumn == index.column() && mColumnPressed != -1)
   {
      const auto text = index.data().toString();
      if (cursorColumn == static_cast<int>(CommitHistoryColumns::Sha) && text != ZERO_SHA)
      {
         QApplication::clipboard()->setText(text);
         QToolTip::showText(QCursor::pos(), tr("Copied!"), mView);
      }

      mColumnPressed = -1;
      return true;
   }

   return QStyledItemDelegate::editorEvent(event, model, option, index);
}

QColor RepositoryViewDelegate::paintBranchHelper(QPainter *p, const QStyleOptionViewItem &opt,
                                                 const Commit &commit) const
{
   const auto colorIndex = mView->hasActiveFilter() ? 0
       : commit.sha != ZERO_SHA                     ? mGraphCache->getSacredTimeline(commit.sha) % GitQlientStyles::getTotalBranchColors()
                                                    : -1;

   if (colorIndex != -1)
   {
      static const auto LINE_OFFSET = 6;

      const auto activeColor = GitQlientStyles::getBranchColorAt(colorIndex);

      static QPen lanePen(GitQlientStyles::getTextColor(), 2); // fast path here
      lanePen.setBrush(activeColor);
      lanePen.setColor(activeColor);

      p->save();
      p->setRenderHint(QPainter::Antialiasing);
      p->setPen(lanePen);
      p->drawLine(opt.rect.x() + 3, opt.rect.y() + LINE_OFFSET, opt.rect.x() + 3,
                  opt.rect.y() + ROW_HEIGHT - LINE_OFFSET);
      p->restore();

      return activeColor;
   }

   return QColor();
}

void RepositoryViewDelegate::paintGraphLane(QPainter *p, const State &lane, bool laneHeadPresent, int x1, int x2,
                                            const QColor &col, const QColor &activeCol, const QColor &mergeColor,
                                            bool isWip, bool hasChilds) const
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

   switch (lane.getType())
   {
      case StateType::Join:
      case StateType::JoinRight:
      case StateType::Head:
      case StateType::HeadRight: {
         p->drawArc(m, h, angleWidthRight, angleHeightUp, 0 * 16, spanAngle);
         break;
      }
      case StateType::JoinLeft: {
         p->drawArc(m, h, angleWidthLeft, angleHeightUp, 90 * 16, spanAngle);
         break;
      }
      case StateType::Tail:
      case StateType::TailRight: {
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
   if (!(isWip && !hasChilds))
   {
      if (!isWip && !hasChilds && (lane.getType() == StateType::Head || lane.isActive()))
         p->drawLine(m, h, m, 2 * h);
      else
      {
         switch (lane.getType())
         {
            case StateType::Active:
            case StateType::Inactive:
            case StateType::MergeFork:
            case StateType::MergeForkRight:
            case StateType::MergeForkLeft:
            case StateType::Join:
            case StateType::JoinRight:
            case StateType::JoinLeft:
            case StateType::Cross:
               p->drawLine(m, 0, m, 2 * h);
               break;
            case StateType::HeadLeft:
            case StateType::Branch:
               p->drawLine(m, h, m, 2 * h);
               break;
            case StateType::TailLeft:
            case StateType::Initial:
               p->drawLine(m, 0, m, h);
               break;
            default:
               break;
         }
      }
   }

   // center symbol
   auto isCommit = false;

   if (isWip)
   {
      isCommit = true;
      p->setPen(QPen(col, 2));
      p->setBrush(col);
      p->drawEllipse(m - r + 2, h - r + 2, 8, 8);
   }
   else
   {
      switch (lane.getType())
      {
         case StateType::Head:
         case StateType::Initial:
         case StateType::Branch:
         case StateType::MergeFork:
         case StateType::MergeForkRight:
            isCommit = true;
            p->setPen(QPen(col, 2));
            p->setBrush(mergeColor);
            p->drawEllipse(m - r + 2, h - r + 2, 8, 8);
            break;
         case StateType::MergeForkLeft:
            isCommit = true;
            p->setPen(QPen(col, 2));
            p->setBrush(laneHeadPresent ? mergeColor : GitQlientStyles::getBackgroundColor());
            p->drawEllipse(m - r + 2, h - r + 2, 8, 8);
            break;
         case StateType::Active: {
            isCommit = true;
            p->setPen(QPen(col, 2));
            p->setBrush(QColor(isWip ? col : GitQlientStyles::getBackgroundColor()));
            p->drawEllipse(m - r + 2, h - r + 2, 8, 8);
         }
         break;
         default:
            break;
      }
   }

   lanePen.setColor(mergeColor);
   p->setPen(lanePen);

   // horizontal line
   switch (lane.getType())
   {
      case StateType::MergeFork:
      case StateType::Join:
      case StateType::Head:
      case StateType::Tail:
      case StateType::Cross:
      case StateType::CrossEmpty:
         p->drawLine(x1 + (isCommit ? 10 : 0), h, x2, h);
         break;
      case StateType::MergeForkRight:
         p->drawLine(x1 + (isCommit ? 0 : 10), h, m - (isCommit ? 6 : 0), h);
         break;
      case StateType::MergeForkLeft:
      case StateType::HeadLeft:
      case StateType::TailLeft:
         p->drawLine(m + (isCommit ? 6 : 0), h, x2, h);
         break;
      default:
         break;
   }
}

QColor RepositoryViewDelegate::getMergeColor(const State &currentLane, const Commit &commit, int currentLaneIndex,
                                             const QColor &defaultColor, bool &isSet) const
{
   auto mergeColor = defaultColor;
   //= GitQlientStyles::getBranchColorAt((commit.getLanesCount() - 1) % GitQlientStyles::getTotalBranchColors());

   switch (currentLane.getType())
   {
      case StateType::HeadLeft:
      case StateType::HeadRight:
      case StateType::TailLeft:
      case StateType::TailRight:
      case StateType::MergeForkLeft:
      case StateType::JoinRight:
         isSet = true;
         mergeColor = defaultColor;
         break;
      case StateType::MergeForkRight:
      case StateType::JoinLeft:
         for (auto laneCount = 0; laneCount < currentLaneIndex; ++laneCount)
         {
            if (mGraphCache->getTimelineAt(commit.sha, laneCount) == StateType::JoinLeft)
            {
               mergeColor = GitQlientStyles::getBranchColorAt(laneCount % GitQlientStyles::getTotalBranchColors());
               isSet = true;
               break;
            }
         }
         break;
      default:
         break;
   }

   return mergeColor;
}

void RepositoryViewDelegate::paintGraph(QPainter *p, const QStyleOptionViewItem &opt, const Commit &commit) const
{
   p->save();
   p->setClipRect(opt.rect, Qt::IntersectClip);
   p->translate(opt.rect.topLeft());

   if (mView->hasActiveFilter())
   {
      const auto activeColor = GitQlientStyles::getBranchColorAt(0);
      paintGraphLane(p, StateType::Active, false, 0, LANE_WIDTH, activeColor, activeColor, activeColor, false,
                     commit.hasChilds());
   }
   else
   {
      if (commit.sha == ZERO_SHA)
      {
         const auto activeColor = GitQlientStyles::getBranchColorAt(0);
         QColor color = activeColor;

         if (mCache->pendingLocalChanges())
            color = gitQlientOrange;

         paintGraphLane(p, StateType::Branch, false, 0, LANE_WIDTH, color, activeColor, activeColor, true,
                        commit.parentsCount() != 0 && !commit.parents().contains(INIT_SHA));
      }
      else
      {
         const auto laneNum = mGraphCache->timelinesCount(commit.sha);
         const auto activeLane = mGraphCache->getSacredTimeline(commit.sha);
         const auto activeColor
             = GitQlientStyles::getBranchColorAt(activeLane % GitQlientStyles::getTotalBranchColors());
         auto x1 = 0;
         auto isSet = false;
         auto laneHeadPresent = false;
         auto mergeColor = GitQlientStyles::getBranchColorAt((laneNum - 1) % GitQlientStyles::getTotalBranchColors());

         for (auto i = laneNum - 1, x2 = LANE_WIDTH * laneNum; i >= 0; --i, x2 -= LANE_WIDTH)
         {
            x1 = x2 - LANE_WIDTH;

            auto currentLane = mGraphCache->getTimelineAt(commit.sha, i);

            if (!laneHeadPresent && i < laneNum - 1)
            {
               auto prevLane = mGraphCache->getTimelineAt(commit.sha, i + 1);
               laneHeadPresent
                   = prevLane.isHead() || prevLane == StateType::JoinRight || prevLane == StateType::JoinLeft;
            }

            if (currentLane != StateType::Empty)
            {
               auto color = activeColor;

               if (i != activeLane)
                  color = GitQlientStyles::getBranchColorAt(i % GitQlientStyles::getTotalBranchColors());

               if (!isSet)
                  mergeColor = getMergeColor(currentLane, commit, i, color, isSet);

               paintGraphLane(p, currentLane, laneHeadPresent, x1, x2, color, activeColor, mergeColor, false,
                              commit.hasChilds());

               if (mView->hasActiveFilter())
                  break;
            }
         }
      }
   }
   p->restore();
}

void RepositoryViewDelegate::paintLog(QPainter *p, const QStyleOptionViewItem &opt, const QColor &currentLangeColor,
                                      const Commit &commit) const
{
   if (commit.sha.isEmpty())
      return;

   auto offset = 5;

   paintTagBranch(p, opt, currentLangeColor, offset, commit);

   auto newOpt = opt;
   newOpt.rect.setX(opt.rect.x() + offset + 5);
   newOpt.rect.setY(newOpt.rect.y() + TEXT_HEIGHT_OFFSET);

   QFontMetrics fm(newOpt.font);

   p->setFont(newOpt.font);
   p->setPen(GitQlientStyles::getTextColor());
   p->drawText(newOpt.rect, fm.elidedText(commit.shortLog, Qt::ElideRight, newOpt.rect.width()),
               QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

void RepositoryViewDelegate::paintTagBranch(QPainter *painter, QStyleOptionViewItem o, const QColor &currentLangeColor,
                                            int &startPoint, const Commit &commit) const
{
   if (mCache->hasReferences(commit.sha) && !mView->hasActiveFilter())
   {
      struct RefConfig
      {
         QString name;
         QColor color;
         QString icon;
         bool isTag = false;
      };

      QVector<RefConfig> refs;
      const auto currentBranch = mGit->getCurrentBranch();

      if (startPoint <= 5)
         startPoint += 5;

      if ((currentBranch.isEmpty() || currentBranch == "HEAD"))
      {
         if (const auto ret = mGit->getLastCommit(); ret.success && commit.sha == ret.output.trimmed())
         {
            refs.append({ "detached", graphDetached, "" });
         }
      }

      static const auto suffix
          = QString::fromUtf8(GitQlientSettings().globalValue("colorSchema", 0).toInt() == 1 ? "bright" : "dark");
      const auto localBranches = mCache->getReferences(commit.sha, References::Type::LocalBranch);
      for (const auto &branch : localBranches)
      {
         refs.append({ branch, currentLangeColor, QString(":/icons/branch_indicator_%1").arg(suffix) });
      }

      const auto tags = mCache->getReferences(commit.sha, References::Type::LocalTag);
      for (const auto &tag : tags)
      {
         refs.append({ tag, graphTag, QString(":/icons/tag_indicator_%1").arg(suffix), true });
      }

      const auto remoteBranches = mCache->getReferences(commit.sha, References::Type::RemoteBranche);
      for (const auto &branch : remoteBranches)
      {
         refs.append({ branch, currentLangeColor, QString(":/icons/branch_indicator_%1").arg(suffix) });
      }

      auto offset = 5;
      const auto mark_spacing = 7; // Space between markers in pixels

      auto newOpt = o;
      newOpt.rect.setX(o.rect.x() + offset + 5);
      newOpt.rect.setY(newOpt.rect.y() + TEXT_HEIGHT_OFFSET);

      QFontMetrics fm(newOpt.font);
      QString finalText;

      auto tmpBuffer = 0;
      for (auto &iter : refs)
      {
         finalText += QString("%1 ").arg(iter.name);
         tmpBuffer += 20;
      }

      finalText.append(commit.shortLog);

      QString nameToDisplay;

      if (auto textWidth = fm.boundingRect(finalText).width(); textWidth + tmpBuffer >= o.rect.width() && GitQlientSettings().globalValue("HistoryView/PreferCommit", true).toBool())
         nameToDisplay = QString("...");

      for (auto &iter : refs)
      {
         const auto isCurrentSpot = iter.name == "detached" || iter.name == currentBranch;
         o.font.setBold(isCurrentSpot);

         if (nameToDisplay.isEmpty() || (!nameToDisplay.isEmpty() && nameToDisplay != QString("...")))
            nameToDisplay = iter.name;

         const QFontMetrics fm(o.font);
         const auto textBoundingRect = fm.boundingRect(nameToDisplay);
         const int textPadding = 5;
         const auto iconSize = ROW_HEIGHT - 4;
         const auto rectWidth = textBoundingRect.width() + 2 * textPadding + iconSize;

         painter->save();
         painter->setRenderHint(QPainter::Antialiasing);
         painter->setPen(QPen(iter.color, 2));

         QRectF markerRect(o.rect.x() + startPoint, o.rect.y() + 2, rectWidth, iconSize);
         {
            QPainterPath path;
            path.addRoundedRect(markerRect, 1, 1);
            painter->drawPath(path);
         }

         QRectF iconRect(o.rect.x() + startPoint, o.rect.y() + 2, iconSize, iconSize);
         {
            QPainterPath smallPath;
            smallPath.addRoundedRect(iconRect, 1, 1);
            painter->fillPath(smallPath, iter.color);
            painter->drawImage(iconRect, QImage(iter.icon));
         }

         {
            QRectF textRect(iconRect.x() + iconRect.width() + textPadding, o.rect.y() + TEXT_HEIGHT_OFFSET,
                            textBoundingRect.width(), iconSize);
            painter->setPen(GitQlientSettings().globalValue("colorSchema", 0).toInt() == 1 ? textColorBright
                                                                                           : textColorDark);
            painter->setFont(o.font);
            painter->drawText(textRect, Qt::AlignCenter, nameToDisplay);
         }

         painter->restore();

         startPoint += rectWidth + mark_spacing;
      }
   }
}
