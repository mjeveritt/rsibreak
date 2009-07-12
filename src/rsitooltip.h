/*
   Copyright (C) 2005 Bram Schoenmakers <bramschoenmakers@kde.nl>
   Copyright (C) 2006 Tom Albers <toma.org>

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

#ifndef RSITOOLTIP_H
#define RSITOOLTIP_H

#include "passivepopup.h"
#include <QPixmap>
#include <QLabel>

/**
 * @class RSIToolTip
 * This frame is shown as a tooltip, when the user hovers the
 * mouse above the docker's icon.
 * @author Bram Schoenmakers <bramschoenmakers@kde.nl>
 */
class RSIToolTip : public QWidget
{
    Q_OBJECT
public:
    explicit RSIToolTip( QWidget *parent, QSystemTrayIcon* );
    ~RSIToolTip();

public slots:
    void setCounters( int, int );
    /** Sets the pixmap for the tooltip. */
    void setPixmap( const QPixmap & );
    /** Sets the tooltip to suspended or unsuspended depending on the parameter. */
    void setSuspended( bool );
    /** Sets the tooltip to @param text */
    void setText( const QString &text );
    void setTimeout( int i ) {
        m_popup->setTimeout( i );
    };
    void showToolTip() {
        m_popup->show( m_icon );
    };


private:
    QLabel *mTinyLeft;
    QLabel *mBigLeft;
    QLabel *mIcon;
    PassivePopup *m_popup;
    QSystemTrayIcon *m_icon;

    bool m_suspended;
};

#endif // RSITOOLTIP_H
