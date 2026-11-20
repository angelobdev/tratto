// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QColor>
#include <QList>
#include <QObject>
#include <QtQml/qqmlregistration.h>

// Current tool and its options for one editor. The toolbar edits it from QML;
// the canvas reads it from C++. Widths are indices into the presets of
// tratto/core/tools.h (0 = fine/small, 1 = medium, 2 = thick/large).
class EditorTools : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Tool tool READ tool WRITE setTool NOTIFY changed FINAL)

    Q_PROPERTY(int penColor MEMBER m_penColor NOTIFY changed FINAL)
    Q_PROPERTY(int penWidth MEMBER m_penWidth NOTIFY changed FINAL)
    Q_PROPERTY(int highlighterColor MEMBER m_highlighterColor NOTIFY changed FINAL)
    Q_PROPERTY(int highlighterWidth MEMBER m_highlighterWidth NOTIFY changed FINAL)
    Q_PROPERTY(bool straightHighlighter MEMBER m_straightHighlighter NOTIFY changed FINAL)
    Q_PROPERTY(EraserMode eraserMode MEMBER m_eraserMode NOTIFY changed FINAL)
    Q_PROPERTY(int eraserSize MEMBER m_eraserSize NOTIFY changed FINAL)
    Q_PROPERTY(bool eraseHighlighterOnly MEMBER m_eraseHighlighterOnly NOTIFY changed FINAL)
    Q_PROPERTY(SelectionMode selectionMode MEMBER m_selectionMode NOTIFY changed FINAL)
    Q_PROPERTY(TextFont textFont MEMBER m_textFont NOTIFY changed FINAL)
    Q_PROPERTY(int textSize MEMBER m_textSize NOTIFY changed FINAL)
    Q_PROPERTY(bool textBold MEMBER m_textBold NOTIFY changed FINAL)
    Q_PROPERTY(bool textItalic MEMBER m_textItalic NOTIFY changed FINAL)
    Q_PROPERTY(int textColor MEMBER m_textColor NOTIFY changed FINAL)

    // Presets, for the toolbar.
    Q_PROPERTY(QList<QColor> penColors READ penColors CONSTANT FINAL)
    Q_PROPERTY(QList<QColor> highlighterColors READ highlighterColors CONSTANT FINAL)
    Q_PROPERTY(QList<int> textSizes READ textSizes CONSTANT FINAL)

public:
    enum class Tool { Pen, Highlighter, Eraser, Text, Select, Image };
    Q_ENUM(Tool)
    enum class EraserMode { WholeStroke, Partial };
    Q_ENUM(EraserMode)
    enum class SelectionMode { Lasso, Rectangle };
    Q_ENUM(SelectionMode)
    enum class TextFont { Sans, Serif, Monospace };
    Q_ENUM(TextFont)

    explicit EditorTools(QObject *parent = nullptr);

    Tool tool() const { return m_tool; }
    void setTool(Tool tool);

    QList<QColor> penColors() const;
    QList<QColor> highlighterColors() const;
    QList<int> textSizes() const;

    // Read by the canvas.
    int penColorIndex() const { return m_penColor; }
    int highlighterColorIndex() const { return m_highlighterColor; }
    bool straightHighlighter() const { return m_straightHighlighter; }
    EraserMode eraserMode() const { return m_eraserMode; }
    bool eraseHighlighterOnly() const { return m_eraseHighlighterOnly; }
    SelectionMode selectionMode() const { return m_selectionMode; }

    // Values for the canvas, in pt.
    float penWidthPt() const;
    float highlighterWidthPt() const;
    float eraserRadiusPt() const;

signals:
    void changed();

private:
    Tool m_tool = Tool::Pen;
    int m_penColor = 1; // blue
    int m_penWidth = 1;
    int m_highlighterColor = 0;
    int m_highlighterWidth = 1;
    bool m_straightHighlighter = true;
    EraserMode m_eraserMode = EraserMode::WholeStroke;
    int m_eraserSize = 1;
    bool m_eraseHighlighterOnly = false;
    SelectionMode m_selectionMode = SelectionMode::Lasso;
    TextFont m_textFont = TextFont::Sans;
    int m_textSize = 16;
    bool m_textBold = false;
    bool m_textItalic = false;
    int m_textColor = 0; // black
};
