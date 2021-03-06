/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2016 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
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
 *  along with this program (COPYING file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *   Main developers : Jerome Pinguet <jerome@jerome.cc>                   *
 *  Contributors:                                                          *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
/**
 * \class PatientImport::PatientImportDialog
 * Import patient(s) from external softwares.
 */


#include "patientimportdialog_p.h"


using namespace PatientImport;
using namespace Internal;

static inline Core::ITheme *theme()  { return Core::ICore::instance()->theme(); }

PatientImportDialog::PatientImportDialog(QWidget *parent) :
    QDialog(parent),
    d(new PatientImportDialogPrivate(this))
{
    d->createUi();
    setSoftwareComboBoxItems(PatientImport::Internal::PatientimportPlugin::softwareList());
    d->ui->softwareLabel->setText(tr("Software"));
    d->ui->startPushButton->setText(tr("Start import"));
    d->ui->startPushButton->setEnabled(false);
    d->connectUi();
    //d->getReportInformation();
    //d->_sendButton->setText(tr("&Send report"));
    setWindowTitle(tr("Patient import dialog"));
    Utils::centerWidget(this, parent);
}

PatientImportDialog::~PatientImportDialog()
{
//    if (d)
//        delete d;
//    d = 0;
}

void PatientImportDialog::changeEvent(QEvent *e)
{
    Q_UNUSED(e);
return;
}

void PatientImportDialog::setSoftwareComboBoxItems(const QStringList &softlist)
{
    d->ui->softwareComboBox->clear();
    d->ui->softwareComboBox->addItems(softlist);
    d->ui->softwareComboBox->setCurrentIndex(-1);
}

void PatientImportDialog::startImport()
{
    d->ui->startPushButton->setEnabled(false);
    ProcessPatientImport(d->fileName(),d->ui->softwareComboBox->currentText());
}
