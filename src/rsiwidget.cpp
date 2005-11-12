/* This file is part of the KDE project
   Copyright (C) 2005 Tom Albers <tomalbers@kde.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <qpushbutton.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qdatetime.h>
#include <qlineedit.h>
#include <qimage.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qfileinfo.h>

#include "config.h"

#include <kwin.h>
#include <klocale.h>
#include <kapplication.h>
#include <kaccel.h>
#include <kdebug.h>
#include <kconfig.h>
#include <dcopclient.h>
#include <kmessagebox.h>
#include <kpassivepopup.h>

#include <stdlib.h>
#include <math.h>

#include "rsiwidget.h"
#include "rsitimer.h"
#include "rsidock.h"

RSIWidget::RSIWidget( QWidget *parent, const char *name )
    : QWidget( parent, name )
{
    kdDebug() << "Entering RSIWidget::RSIWidget" << endl;

   KMessageBox::information(parent,
                             i18n("Welcome to RSIBreak\n\n"
                                  "In your tray you can now see a clock. "
                                  "When you right-click on that you will see "
                                  "a menu, from which you can go to the "
                                  "configuration for example.\nWhen you want to "
                                  "know when the next break is, hover "
                                  "over the icon.\n\nUse RSIBreak wisely."),
                             i18n("Welcome"),
                             "dont_show_welcome_again_for_001");

    m_backgroundimage = new QPixmap(QApplication::desktop()->width(),
                                    QApplication::desktop()->height());

    m_tray = new RSIDock(this,"Tray Item");
    m_tray->show();

    m_timer = new RSITimer(this,"Timer");
    connect( m_timer, SIGNAL( breakNow() ), SLOT( maximize() ) );
    connect( m_timer, SIGNAL( setCounters() ), SLOT( setCounters() ) );
    connect( m_timer, SIGNAL( updateIdleAvg( int ) ), SLOT( updateIdleAvg( int ) ) );
    connect( m_timer, SIGNAL( minimize() ), SLOT( minimize() ) );
    connect( m_timer, SIGNAL( relax( int ) ), SLOT( updateRelaxMsg( int ) ) );

    connect( m_tray, SIGNAL( quitSelected() ), kapp, SLOT( quit() ) );
    connect( m_tray, SIGNAL( configChanged() ), SLOT( readConfig() ) );
    connect( m_tray, SIGNAL( dialogEntered() ), m_timer, SLOT( slotStop() ) );
    connect( m_tray, SIGNAL( dialogLeft() ), m_timer, SLOT( slotRestart() ) );
    connect( m_tray, SIGNAL( breakRequest() ), m_timer, SLOT( slotMaximize() ) );

    srand ( time(NULL) );

    QBoxLayout *topLayout = new QVBoxLayout( this, 5);

    m_countDown = new QLabel(this);
    m_countDown->setAlignment( AlignCenter );
    m_countDown->setBackgroundOrigin(QWidget::ParentOrigin);
    topLayout->addWidget(m_countDown);

    topLayout->addStretch(5);

    QBoxLayout *buttonRow = new QHBoxLayout( topLayout );

    m_miniButton = new QPushButton( i18n("Minimize"), this );
    buttonRow->addWidget( m_miniButton );
    connect( m_miniButton, SIGNAL( clicked() ), SLOT( slotMinimize() ) );

    m_accel = new KAccel(this);
    m_accel->insert("minimize", i18n("Minimize"),
                    i18n("Abort a break"),Qt::Key_Escape,
                    this, SLOT( slotMinimize() ));

    buttonRow->addStretch(10);

    m_timer_slide = new QTimer(this);
    connect(m_timer_slide, SIGNAL(timeout()),  SLOT(slotNewSlide()));

    m_popup = new KPassivePopup(m_tray);
    m_tool = new QLabel(m_popup);
    m_popup->setView(m_tool);

    readConfig();
}

RSIWidget::~RSIWidget()
{
    kdDebug() << "Entering RSIWidget::~RSIWidget" << endl;
    delete m_backgroundimage;
}

void RSIWidget::minimize()
{
    kdDebug() << "Entering RSIWidget::Minimize" << endl;
    m_timer_slide->stop();
    hide();
    loadImage();
}

void RSIWidget::maximize()
{
    kdDebug() << "Entering RSIWidget::Maximize" << endl;

    if (m_slideInterval>0)
        m_timer_slide->start( m_slideInterval*1000 );

    show(); // Keep it above the KWin calls.
    KWin::forceActiveWindow(winId());
    KWin::setOnAllDesktops(winId(),true);
    KWin::setState(winId(), NET::KeepAbove);
    KWin::setState(winId(), NET::FullScreen);
}

void RSIWidget::loadImage()
{
    if (m_files.count() == 0)
        return;

    // reset if all images are shown
    if (m_files_done.count() == m_files.count())
        m_files_done.clear();

    // get a net yet used number
    int j = (int) ((m_files.count()) * (rand() / (RAND_MAX + 1.0)));
    while (m_files_done.findIndex( QString::number(j) ) != -1)
        j = (int) ((m_files.count()) * (rand() / (RAND_MAX + 1.0)));

    // Use that number to load the right image
    m_files_done.append(QString::number(j));

    kdDebug() << "Loading: " << m_files[j] <<
                    "( " << j << " / "  << m_files.count() << " ) " << endl;

    QImage m = QImage( m_files[ j ]).smoothScale(
                        QApplication::desktop()->width(),
                        QApplication::desktop()->height(),
                        QImage::ScaleMax);


    if (m.isNull())
    {
        kdWarning() << "constructed a null-image" << endl;
        kdDebug() << "format: " <<
                QImageIO::imageFormat(m_files[j]) << endl;

        QImageIO iio;
        iio.setFileName(m_files[j]);
        if ( iio.read() )
            kdDebug() << "Read is ok" << endl;
        else
            kdDebug() << "Read failed" << endl;

        kdDebug() << iio.status() << endl;
        return;
    }

    if (m_backgroundimage->convertFromImage(m))
        kdDebug() << "New background image set" << endl;
    else
        kdWarning() << "Failed to set new background image" << endl;
}


void RSIWidget::findImagesInFolder(const QString& folder)
{
    kdDebug() << "Looking for pictures in " << folder << endl;

    if ( folder.isNull() )
        return;

    QDir dir( folder);

    // TODO: make an automated filter, maybe with QImageIO.
    QString ext("*.png *.jpg *.jpeg *.tif *.tiff *.gif *.bmp *.xpm *.ppm *.pnm *.xcf *.pcx");
    dir.setNameFilter(ext + " " + ext.upper());
    dir.setMatchAllDirs ( true );

    if ( !dir.exists() or !dir.isReadable() )
    {
        kdWarning() << "Folder does not exist or is not readable: "
                << folder << endl;
        return;
    }

    const QFileInfoList *list = dir.entryInfoList();
    if (!list)
        return;

    QFileInfoListIterator it( *list );
    QFileInfo *fi;

    while ( (fi = it.current()) != 0 )
    {
        if ( fi->isFile())
            m_files.append(fi->filePath());
        else if (fi->isDir() && m_searchRecursive &&
                 fi->fileName() != "." &&  fi->fileName() != "..")
            findImagesInFolder(fi->absFilePath());
        ++it;
    }
}

// -------------------------- SLOTS ------------------------//

void RSIWidget::slotNewSlide()
{
    kdDebug() << "Entering RSIWidget::slotNewSlide" << endl;

    loadImage();
    repaint( false );
}

void RSIWidget::slotLock()
{
    kdDebug() << "Entering RSIWidget::slotLock" << endl;

    QCString appname( "kdesktop" );
    int rsibreak_screen = qt_xscreen();
    if ( rsibreak_screen )
        appname.sprintf("kdesktop-screen-%d", rsibreak_screen );
    kapp->dcopClient()->send(appname, "KScreensaverIface", "lock()", "");
}

void RSIWidget::slotMinimize()
{
    kdDebug() << "slotMinize entered" << endl;
    m_timer->slotRestart();
}

void RSIWidget::setCounters()
{
    int s = (int)ceil(QTime::currentTime().msecsTo( m_timer->targetTime() )/1000);

    // TODO: tell something about tinyBreaks, bigBreaks.
    if (s > 0)
    {
        m_countDown->setText( QString::number( s ) );
        QToolTip::add(m_tray, i18n("One second remaining",
                      "%n seconds remaining",s));
    }
    else
    {
        m_countDown->setText (QString::null );
        QToolTip::add(m_tray, i18n("Waiting for the right moment to break"));
    }
}

void RSIWidget::updateIdleAvg( int idleAvg )
{
    if ( idleAvg < 0 )
        m_tray->setIcon( 0 );
    if ( idleAvg >=20 && idleAvg<40 )
        m_tray->setIcon( 1 );
    if ( idleAvg >=40 && idleAvg<50 )
        m_tray->setIcon( 2 );
    if ( idleAvg >=50 && idleAvg<60 )
        m_tray->setIcon( 3 );
    if ( idleAvg >=60 )
        m_tray->setIcon( 4 );
}

void RSIWidget::updateRelaxMsg( int sec )
{
  if ( sec > 0 )
  {
    m_tool->setText(i18n("Please relax for 1 second",
                       "Please relax for %n seconds",
                       sec ));
    m_popup->show();
  }
  else
  {
    m_popup->hide();
  }
}

// ----------------------------- EVENTS -----------------------//

void RSIWidget::paintEvent( QPaintEvent * )
{
    kdDebug() << "Entering RSIWidget::paintEvent" << endl;
    bitBlt( this, 0, 0, m_backgroundimage );
    m_countDown->setPaletteBackgroundPixmap( *m_backgroundimage );
}

void RSIWidget::closeEvent( QCloseEvent * )
{
    hide();
}

//--------------------------- CONFIG ----------------------------//


void RSIWidget::readConfig()
{
    kdDebug() << "Entering RSIWidget::readConfig" << endl;
    KConfig* config = kapp->config();
    QColor *Black = new QColor(Qt::black);
    QFont *t = new QFont(  QApplication::font().family(), 40, 75, true );

    config->setGroup("General Settings");
    m_countDown->setPaletteForegroundColor(
            config->readColorEntry("CounterColor", Black ) );
    m_miniButton->setHidden(
            config->readBoolEntry("HideMinimizeButton", false));
    m_countDown->setHidden(
            config->readBoolEntry("HideCounter", false));
    m_accel->setEnabled("minimize",
                   !config->readBoolEntry("DisableAccel", false));
    QString shortcut = config->readEntry("MinimizeKey", "Escape");
    m_accel->setShortcut("minimize",KShortcut(shortcut));

    m_countDown->setFont(
            config->readFontEntry("CounterFont", t ) );

    bool recursive =
            config->readBoolEntry("SearchRecursiveCheck", false);
    QString path = config->readEntry("ImageFolder");

    if (m_basePath != path || m_searchRecursive != recursive)
    {
        m_files.clear();
        m_files_done.clear();
        m_basePath = path;
        m_searchRecursive = recursive;
        findImagesInFolder( path );
        loadImage();
    }

    m_slideInterval = config->readNumEntry("SlideInterval", 2);

    delete Black;
    delete t;

    m_timer->slotReadConfig();
}

#include "rsiwidget.moc"
