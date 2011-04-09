#include "irc-text-browser.h"

#include <QDebug>

namespace egalite
{

IrcTextBrowser::IrcTextBrowser (QGraphicsItem *prnt)
  :QGraphicsTextItem(prnt)
{
  setTextInteractionFlags (Qt::TextSelectableByMouse
                         | Qt::TextSelectableByKeyboard
                         | Qt::LinksAccessibleByMouse
                         | Qt::LinksAccessibleByKeyboard);
  connect (this, SIGNAL (linkActivated(const QString &)),
           this, SLOT (doActivateLink(const QString &)));
  DebugCheck ();
}

void
IrcTextBrowser::DebugCheck ()
{
  qDebug () << " -=---------------  IrcTextBrowser DebugCheck  "
            ;
  qDebug () << "                    I am            " << this;
  qDebug () << "                    parent          " << parent();
  qDebug () << "                    parentItem      " << parentItem();
}

void
IrcTextBrowser::setWidth (qreal wid)
{
  setTextWidth (wid);
  qDebug () << " IrcTextBrowser set Width " << wid 
            << " is now " << textWidth();
}

void
IrcTextBrowser::setHtml (const QString & html)
{
  QGraphicsTextItem::setHtml (html);
}

void
IrcTextBrowser::doActivateLink (const QString & link)
{
  qDebug () << "IrcTextBrowser link activated, re-emit " << link;
  DebugCheck ();
  emit activatedLink (link);
}



} // namespace