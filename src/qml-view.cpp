#include "qml-view.h"
#include <QDebug>

namespace egalite
{

QmlView::QmlView (QWidget *parent)
  :QDeclarativeView (parent)
{
  setResizeMode (QDeclarativeView::SizeRootObjectToView);
}

void
QmlView::resizeEvent (QResizeEvent *event)
{
  qDebug () << __PRETTY_FUNCTION__ << " resize event " << event;
  QDeclarativeView::resizeEvent (event);
}

} // namespace
