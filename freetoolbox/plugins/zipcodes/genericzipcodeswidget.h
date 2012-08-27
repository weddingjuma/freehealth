/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2012 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program (COPYING.FREEMEDFORMS file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main Developer: Christian A. Reiter <christian.a.reiter@gmail.com     *
 *   Contributors:                                                         *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#ifndef GENERICZIPCODESWIDGET_H
#define GENERICZIPCODESWIDGET_H

#include <QtGui/QWidget>
#include "genericzipcodesstep.h"

namespace ZipCodes {
namespace Ui {
class GenericZipCodesWidget;
}

class GenericZipCodesWidget : public QWidget
{
    Q_OBJECT
public:
    GenericZipCodesWidget(QWidget *parent = 0);
    ~GenericZipCodesWidget();

private Q_SLOTS:
    void on_startJob_clicked();
    void on_downloadButton_clicked();
    void downloadFinished();

private:
    Ui::GenericZipCodesWidget *ui;
    GenericZipCodesStep *m_Step;
};

} // end ZipCodes
#endif // GENERICZIPCODESWIDGET_H