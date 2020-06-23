#include "FileDiffEditor.h"

#include <GitQlientStyles.h>

FileDiffEditor::FileDiffEditor(QWidget *parent)
   : FileDiffView(parent)
{
   setReadOnly(false);

   connect(this, &FileDiffView::cursorPositionChanged, this, &FileDiffEditor::highlightCurrentLine);

   highlightCurrentLine();
}

void FileDiffEditor::highlightCurrentLine()
{
   QList<QTextEdit::ExtraSelection> extraSelections;

   if (!isReadOnly())
   {
      QTextEdit::ExtraSelection selection;

      selection.format.setBackground(GitQlientStyles::getGraphSelectionColor());
      selection.format.setProperty(QTextFormat::FullWidthSelection, true);
      selection.cursor = textCursor();
      selection.cursor.clearSelection();
      extraSelections.append(selection);
   }

   setExtraSelections(extraSelections);
}
