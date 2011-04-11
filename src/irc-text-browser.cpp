#include "irc-text-browser.h"


/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2010, Bernd Stramm
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/


#include <QDebug>
#include <QTimer>

namespace egalite
{

IrcTextBrowser::IrcTextBrowser (QGraphicsItem *prnt)
  :QGraphicsTextItem(prnt)
{
  setTextInteractionFlags (Qt::TextSelectableByMouse
                         | Qt::TextSelectableByKeyboard
                         | Qt::LinksAccessibleByMouse
                         | Qt::LinksAccessibleByKeyboard);
  setObjectName ("IrcTextBrowser_");
  connect (this, SIGNAL (linkActivated(const QString &)),
           this, SLOT (doActivateLink(const QString &)));
  DebugCheck ();
  QTimer::singleShot (2000, this, SLOT (DebugCheck()));
}

QRectF
IrcTextBrowser::boundingRect () const
{
  return QGraphicsTextItem::boundingRect ();
}

void
IrcTextBrowser::noFunc ()
{
}

qreal
IrcTextBrowser::getHeight () const
{
  qDebug () << " asking " << objectName() << " for height ";
  qDebug () << "    answer " << boundingRect().height() 
            << " because of " << boundingRect();
  return boundingRect().height();
}

QString
IrcTextBrowser::getName () const
{
  return objectName();
}

void
IrcTextBrowser::setName (const QString & name)
{
  setObjectName (QString ("IrcTextBrowser_") + name);
}

void
IrcTextBrowser::DebugCheck ()
{
  qDebug () << " -=---------------  IrcTextBrowser DebugCheck  "
            ;
  qDebug () << "                    I am            " << this;
  qDebug () << "                    name            " << objectName();
  qDebug () << "                    parent          " << parent();
  qDebug () << "                    parentItem      " << parentItem();
}

void
IrcTextBrowser::setWidth (qreal wid)
{
  setTextWidth (wid);
  qDebug () << " IrcTextBrowser " << objectName() << " set Width " << wid 
            << " is now " << textWidth();
}

void
IrcTextBrowser::setHtml (const QString & html)
{
  qDebug () << "IrcTextBrowser " << objectName() << " set html " << html;
  QGraphicsTextItem::setHtml (html);
  qDebug () << "           textWidth " << textWidth();
}

void
IrcTextBrowser::doActivateLink (const QString & link)
{
  qDebug () << "IrcTextBrowser  " << objectName() << " link activated, re-emit " << link;
  DebugCheck ();
  emit activatedLink (link);
}



} // namespace