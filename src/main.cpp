/* 
   Copyright (C) 2005-2007,2009 Tom Albers <toma@kde.org>

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

#include <QDBusInterface>
#include <kuniqueapplication.h>
#include <kstartupinfo.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <stdio.h>
#include <stdlib.h>

#include "rsiwidget.h"

/**
 * @class RSIApplication
 * @author Tom Albers <toma@kde.org>
 * This class is a simple inheritance from KUniqueApplication
 * the reason that it is reimplemented is that when RSIBreak
 * is launched a second time it would in the orinal implementation
 * make the RSIWidget->show(). which we do not want. This
 * class misses that call.
 */
class RSIApplication : public KUniqueApplication
{
public:
    /**
     * Similar to KUniqueApplication::newInstance, only without
     * the call to raise the widget when a second instance is started.
     */
    int newInstance() {
        static bool secondMe = false;
        if ( secondMe ) {
            QDBusInterface dbus( "org.rsibreak.rsibreak", "/rsibreak",
                                 "org.rsibreak.rsiwidget" );
            dbus.call( "showWhereIAm" );
        } else {
            secondMe = true;
        }
        return 0;
    }
};

int main( int argc, char *argv[] )
{
    KAboutData aboutData( "rsibreak", 0,
                          ki18n( "RSIBreak" ),
                          "0.10-beta1",
                          ki18n( "Try to prevent Repetitive Strain Injury by "
                                 "reminding a user to rest." ),
                          KAboutData::License_GPL,
                          ki18n( "(c) 2005-2008, The RSIBreak developers" ),
                          KLocalizedString(),
                          "http://www.rsibreak.org" );

    aboutData.addAuthor( ki18n( "Tom Albers" ), ki18n( "Maintainer and Author" ),
                         "toma@kde.org", "http://www.omat.nl" );

    aboutData.addAuthor( ki18n( "Bram Schoenmakers" ), ki18n( "Former author" ),
                         "bramschoenmakers@kde.nl" );

    aboutData.addCredit( ki18n( "Tina Trillitzsch" ), ki18n( "Usability" ),
                         "t.trillitzsch@gmx.de" );

    aboutData.addCredit( ki18n( "Lee Olson" ), ki18n( "Logo and Icons" ),
                         "clearbeast@gmail.com" );

    aboutData.addCredit( ki18n( "Achim Bohnet" ),
                         ki18n( "Former packaging and tester" ),
                         "ach@mpe.mpg.de" );

    KCmdLineArgs::init( argc, argv, &aboutData );

    if ( !KUniqueApplication::start() ) {
        KStartupInfo::handleAutoAppStartedSending();
        fprintf( stderr, "RSIBreak is already running!\n" );
        exit( 0 );
    }

    RSIApplication a;
    a.disableSessionManagement();
    // else it will exit(0) when closing any config/about/etc window.
    a.setQuitOnLastWindowClosed( false );

    new RSIObject();

    return a.exec();
}
