#ifndef EGALITE_IRC_TEXT_BROWSER_H
#define EGALITE_IRC_TEXT_BROWSER_H

#include <QGraphicsTextItem>
#include <QGraphicsItem>
#include <QRectF>
#include <QTimer>

namespace egalite
{

class IrcTextBrowser : public QGraphicsTextItem
{
Q_OBJECT

public:

  IrcTextBrowser (QGraphicsItem * parent=0);

  Q_PROPERTY(qreal height READ getHeight)

  Q_INVOKABLE void noFunc ();

  Q_INVOKABLE void setHtml (const QString & html);
  Q_INVOKABLE void setWidth (qreal wid);
  Q_INVOKABLE QRectF boundingRect () const;
  Q_INVOKABLE qreal getHeight () const;

private slots:

  void doActivateLink (const QString & link);

signals:

  void activatedLink (const QString & link);

private:

  void DebugCheck ();

};

} // namespace


#endif
