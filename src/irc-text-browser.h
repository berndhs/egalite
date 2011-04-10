#ifndef EGALITE_IRC_TEXT_BROWSER_H
#define EGALITE_IRC_TEXT_BROWSER_H

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
