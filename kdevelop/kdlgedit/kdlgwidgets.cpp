/***************************************************************************
                          kdlgwidgets.cpp  -  description                              
                             -------------------                                         
    begin                : Wed Mar 17 1999
    copyright            : (C) 1999 by Pascal Krahmer
    email                : pascal@beast.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/


#include "kdlgwidgets.h"
#include <kapp.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <kquickhelp.h>
#include "../ckdevelop.h"
#include "kdlgeditwidget.h"

#define btnsize 34


KDlgWidgets::KDlgWidgets(CKDevelop *parCKD, QWidget *parent, const char *name ) : QWidget(parent,name)
{
  pCKDevel = parCKD;
  scrview = new myScrollView(this);

  KQuickHelp::add(scrview->viewport(),
    i18n("<brown><b>\"Widgets\" tab<black></b>\n\n"
         "In this tab you will find all items\n"
         "you can add to the dialog. They are\n"
         "sorted in two groups :\n\n"
         "The <b><i>QT-Widgets</i></b>group is\n"
         "always visible and contains all native\n"
         "Qt widgets. Your program will not need\n"
         "the KDE libraries in order to run if you\n"
         "only use these items.\n\n"
         "The <b><i>KDE-Widgets</i></b>group is\n"
         "only visible if you have created a KDE\n"
         "application because it would not be\n"
         "logical to add KDE widgets needing KDE\n"
         "libraries to a Qt project."));
}

KDlgWidgets::~KDlgWidgets()
{
}

void KDlgWidgets::resizeEvent ( QResizeEvent * )
{
  scrview->setGeometry(0,0,width(),height());
}

void KDlgWidgets::myScrollView::addButton(const QPixmap &pix, QString name, bool isKDE)
{
  if (btnsCount >= MAX_BUTTONS)
    return;

  buttons[btnsCount] = new QToolButton( this );
  buttons[btnsCount] -> setPixmap(pix);
  if (pix.isNull())
    buttons[btnsCount] -> setText(name);
  buttons[btnsCount] -> setGeometry(0,0,btnsize,btnsize);
  buttons[btnsCount] -> setUsesBigPixmap(true);
  addChild(buttons[btnsCount]);
  btnsKDE[btnsCount] = isKDE;

  btnsCount++;
}

void KDlgWidgets::clicked_QWidget()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QWidget");
}

void KDlgWidgets::clicked_QLabel()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QLabel");
}

void KDlgWidgets::clicked_QPushButton()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QPushButton");
}

void KDlgWidgets::clicked_QLineEdit()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QLineEdit");
}

void KDlgWidgets::clicked_QCheckBox()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QCheckBox");
}
void KDlgWidgets::clicked_QListBox()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QListBox");
}
void KDlgWidgets::clicked_QLCDNumber()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QLCDNumber");
}

void KDlgWidgets::clicked_QRadioButton()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QRadioButton");
}

void KDlgWidgets::clicked_QComboBox()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QComboBox");
}
void KDlgWidgets::clicked_QProgressBar()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QProgressBar");
}
void KDlgWidgets::clicked_QMultiLineEdit()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QMultiLineEdit");
}
void KDlgWidgets::clicked_QSpinBox()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QSpinBox");
}
void KDlgWidgets::clicked_QScrollBar()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QScrollBar");
}
void KDlgWidgets::clicked_QSlider()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QSlider");
}
void KDlgWidgets::clicked_QGroupBox()
{
  pCKDevel->kdlg_get_edit_widget()->addItem("QGroupBox");
}
KDlgWidgets::myScrollView::~myScrollView()
{
  int i;
  for (i = 0; i<MAX_BUTTONS; i++)
    {
      if (buttons[i]);
        delete buttons[i];
    }
}

KDlgWidgets::myScrollView::myScrollView( QWidget * parent, const char * name, WFlags f )
  : QScrollView(parent,name,f)
{
  isKDEProject = true;
  setHScrollBarMode(QScrollView::AlwaysOff);
  viewport()->setBackgroundMode(PaletteBackground);
  setContentsPos(0,0);

  int i;
  for (i = 0; i<MAX_BUTTONS; i++)
    {
      buttons[i] = 0;
      btnsKDE[i] = false;
    }

  btnsCount = 0;

  #define macroAddButton(fn, wd, mt, ht) \
    addButton(QPixmap(KApplication::kde_datadir() + QString("/kdevelop/pics/mini/") + fn), wd); \
    connect(buttons[btnsCount-1], SIGNAL(clicked()), parent, SLOT(mt())); \
    KQuickHelp::add(buttons[btnsCount-1], QString("<brown><b>") + QString(wd) + QString("<black></b>\n\n") + QString(ht));

  macroAddButton("kdlg_QWidget.xpm",        "QWidget",         clicked_QWidget        ,i18n(
                 "This will insert a QWidget to the dialog.\n"
                 "Such an item may have serveral children items\n"
                 "or even more QWidgets as children. That means\n"
                 "this item lets you create complex hierarchies\n"
                 "for you dialog.\n\n"
                 "Another think you will need this item type for\n"
                 "are widgets like the tab view which need several\n"
                 "widgets they can bring to the top."));

  macroAddButton("kdlg_QLabel.xpm",         "QLabel",          clicked_QLabel         ,i18n("A QLabel can be used in order to display\nsome text or pixmap information in the dialog."));
  macroAddButton("kdlg_QPushButton.xpm",    "QPushButton",     clicked_QPushButton    ,i18n("This is the normal button often used in\ndialogs (i.e. the \"Ok\"-Button)."));
  macroAddButton("kdlg_QLineEdit.xpm",      "QLineEdit",       clicked_QLineEdit      ,i18n("Inserts a text field giving the user the\npossibility to enter of change a text."));
  macroAddButton("kdlg_QCheckBox.xpm",      "QCheckBox",       clicked_QCheckBox      ,i18n("Lets user can (de-)select some settings."));
  macroAddButton("kdlg_QLCDNumer.xpm",      "QLCDNumber",      clicked_QLCDNumber     ,i18n("Displays a number in the style of LC-displays\noften used in clocks."));
  macroAddButton("kdlg_QRadioButton.xpm",   "QRadioButton",    clicked_QRadioButton   ,i18n("Lets the user choose between several options."));
  macroAddButton("kdlg_QComboBox.xpm",      "QComboBox",       clicked_QComboBox      ,i18n(""));
  macroAddButton("kdlg_QListBox.xpm",      "QListBox",       clicked_QListBox      ,i18n(""));
  macroAddButton("kdlg_QMultiLineEdit.xpm",      "QMultiLineEdit",       clicked_QMultiLineEdit      ,i18n(""));
  macroAddButton("kdlg_QProgressBar.xpm",      "QProgressBar",       clicked_QProgressBar      ,i18n(""));
  macroAddButton("kdlg_QSpinBox.xpm",      "QSpinBox",       clicked_QSpinBox      ,i18n(""));
  macroAddButton("kdlg_QSlider.xpm",      "QSlider",       clicked_QSlider      ,i18n(""));
  macroAddButton("kdlg_QScrollBar.xpm",      "QScrollBar",       clicked_QScrollBar      ,i18n(""));
  macroAddButton("kdlg_QGroupBox.xpm",      "QGroupBox",       clicked_QGroupBox      ,i18n(""));
  #undef macroAddButton

  QFont f;
  f.setItalic(true);
  f.setUnderline(true);

  qtlab = new QLabel(i18n("QT-Widgets"), this);
  qtlab->setAlignment(AlignHCenter | AlignBottom);
  qtlab->setFont(f);
  kdelab = new QLabel(i18n("KDE-Widgets"), this);
  kdelab->setAlignment(AlignHCenter | AlignBottom);
  kdelab->setFont(f);
}

int KDlgWidgets::myScrollView::moveBtns(bool isKDE, int posy)
{
  int i;
  int posx = 0;
  for (i = 0; i<btnsCount; i++)
    {
      if ((buttons[i]) && (btnsKDE[i] == isKDE))
        {
          if (posx>width()-btnsize)
            {
              posy += btnsize;
              buttons[i]->setGeometry(0,posy,btnsize,btnsize);
              moveChild(buttons[i],0,posy);
              posx = btnsize;
            }
          else
            {
              buttons[i]->setGeometry(posx,posy,btnsize,btnsize);
              moveChild(buttons[i],posx,posy);
              posx += btnsize;
            }
        }
    }

  return posy;
}

void KDlgWidgets::myScrollView::resizeEvent ( QResizeEvent *e )
{

  QWidget::resizeEvent(e);

  qtlab->setGeometry(0,5,width(),20);
  moveChild(qtlab,0,5);

  int posy = 30;

  posy = moveBtns(false,posy);

  posy += btnsize+5;

  kdelab->setGeometry(0,posy,width(),20);
  moveChild(kdelab,0,posy);

  posy += 25;

  posy = moveBtns(true,posy);

  resizeContents(width(),posy+btnsize);
  setContentsPos(0,0);
}
