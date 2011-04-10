#include "irc-text-browser.h"

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