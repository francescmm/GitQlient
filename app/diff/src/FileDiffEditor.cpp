#include <FileDiffEditor.h>

#include <GitQlientStyles.h>
#include <LineNumberArea.h>

FileDiffEditor::FileDiffEditor(QWidget *parent)
   : FileDiffView(parent)
{
   setReadOnly(false);

   addNumberArea(new LineNumberArea(this));

   connect(this, &FileDiffView::cursorPositionChanged, this, &FileDiffEditor::highlightCurrentLine);

   highlightCurrentLine();
}

void FileDiffEditor::highlightCurrentLine()
{
   QList<QTextEdit::ExtraSelection> extraSelections;

   if (!isReadOnly())
   {
      prevSelection.format.setBackground(QBrush("transparent"));
      prevSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
      prevSelection.cursor.clearSelection();

      QTextEdit::ExtraSelection selection;

      selection.format.setBackground(GitQlientStyles::getGraphSelectionColor());
      selection.format.setProperty(QTextFormat::FullWidthSelection, true);
      selection.cursor = textCursor();
      selection.cursor.clearSelection();

      extraSelections.append(prevSelection);
      extraSelections.append(selection);

      prevSelection = selection;
   }

   setExtraSelections(extraSelections);

   viewport()->update();
}
