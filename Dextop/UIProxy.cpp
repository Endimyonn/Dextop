#include "UIProxy.h"
#include <iostream>

UIProxy::UIProxy(QObject* parent)
{

}

void UIProxy::testFunction1()
{
	qDebug() << "test function 1 called!";
}

QString UIProxy::testVariable1()
{
	return localTestVariable1;
}

void UIProxy::setTestVariable1(QString argNewValue)
{
	localTestVariable1 = argNewValue;
}