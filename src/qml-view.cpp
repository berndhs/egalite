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
  QDeclarativeView::resizeEvent (event);
}

} // namespace
