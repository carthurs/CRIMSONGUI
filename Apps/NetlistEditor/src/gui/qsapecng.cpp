/*
    QSapecNG - Qt based SapecNG GUI front-end
    Copyright (C) 2009, Michele Caini

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "gui/qsapecngwindow.h"

#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>

#include <QMainWindow>


int main(int argc, char** argv)
{
  QApplication app(argc, argv);
//   QPixmap pixmap(":/images/qsapecng-splash-ng.png");
  QPixmap pixmap(":/images/qsapecng-splash.png");
  QSplashScreen splash(pixmap);
  splash.show();

/*
 ... // Loading some items
 splash->showMessage("Loaded modules");

 qApp->processEvents();

 ... // Establishing connections
 splash->showMessage("Established connections");

 qApp->processEvents();
*/

  qsapecng::QSapecNGWindow window;
  window.show();
  splash.finish(&window);

  return app.exec();
}
