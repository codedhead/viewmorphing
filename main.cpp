#include "viewmorph_wizard.h"
#include <QtGui/QApplication>

#include <windows.h>

int main(int argc, char *argv[])
{
	AllocConsole();
	freopen("CONOUT$","w",stdout);
	QApplication a(argc, argv);
	ViewMorphWizard w;
	w.show();
	return a.exec();
}
