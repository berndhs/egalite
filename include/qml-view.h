#ifndef EGALITE_QML_VIEW_H
#define EGALITE_QML_VIEW_H

#include <QDeclarativeView>
#include <QResizeEvent>

namespace egalite
{

class QmlView : public QDeclarativeView
{
Q_OBJECT
public:

  QmlView (QWidget *parent=0);

protected:

  void resizeEvent (QResizeEvent * event); 
};

} // namespace

#endif
