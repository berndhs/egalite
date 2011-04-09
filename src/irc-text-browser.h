#ifndef EGALITE_IRC_TEXT_BROWSER_H
#define EGALITE_IRC_TEXT_BROWSER_H

#include <QGraphicsTextItem>
#include <QGraphicsItem>
#include <QTimer>

namespace egalite
{

class IrcTextBrowser : public QGraphicsTextItem
{
Q_OBJECT

public:

  IrcTextBrowser (QGraphicsItem * parent=0);

  Q_INVOKABLE void setHtml (const QString & html);

private slots:

  void doActivateLink (const QString & link);

signals:

  void activatedLink (const QString & link);

private:

  void DebugCheck ();

};

} // namespace


#endif
