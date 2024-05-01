#pragma once
#include <qobject.h>
#include <qdebug.h>

class UIProxy : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString testVariable1 READ testVariable1 WRITE setTestVariable1 NOTIFY testVariable1Changed)

public:
	UIProxy(QObject* parent = new QObject());
	Q_INVOKABLE void testFunction1();
	QString testVariable1();

signals:
	void testVariable1Changed();

public slots:
	void setTestVariable1(QString);

private:
	QString localTestVariable1;
};