#ifndef EGALITE_IRC_TEXT_BROWSER_H
#define EGALITE_IRC_TEXT_BROWSER_H


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


#include <QGraphicsTextItem>
#include <QGraphicsItem>
#include <QRectF>

namespace egalite
{

class IrcTextBrowser : public QGraphicsTextItem
{
Q_OBJECT

public:

  IrcTextBrowser (QGraphicsItem * parent=0);

  Q_PROPERTY(qreal height READ getHeight) 
  Q_PROPERTY(qreal width READ textWidth WRITE setTextWidth)
  Q_PROPERTY(QString name READ getName WRITE setName) 

  Q_INVOKABLE void noFunc ();

  Q_INVOKABLE void setHtml (const QString & html);
  Q_INVOKABLE void setWidth (qreal wid);
  Q_INVOKABLE QRectF boundingRect () const;
  Q_INVOKABLE qreal getHeight () const;
  Q_INVOKABLE QString getName () const;
  Q_INVOKABLE void setName (const QString & name);

private slots:

  void doActivateLink (const QString & link);

signals:

  void activatedLink (const QString & link);

private:

  void DebugCheck ();

};

} // namespace


#endif
