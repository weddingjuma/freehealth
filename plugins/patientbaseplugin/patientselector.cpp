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
 *   Main developers : Eric MAEKER, <eric.maeker@gmail.com>                *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
/**
 * \class Patients::PatientSelector
 *
 * \brief Selector Widget for the recorded patients.
 * Allow user to search and select patient from the complete database. \n
 * Automatically removes the search filter on user changed to null.
 *
 * Bugs and evolutions:
 * In versions <= 0.8.0, the Core::IPatient was directly used in the patient view
 * and was filtered with the search text. The Core::IPatient was filtered using a SQL
 * query causing indexes to be lost after a search.
 * This was not a good behavior (because of model persistence lose),
 * so in later version we introduced a proxymodel over the Core::IPatient model.
 *
 * \sa Patient::PatientModel
 */

#include "patientselector.h"
#include "patientmodel.h"
#include "patientbar.h"
#include "constants_menus.h"
#include "constants_settings.h"

#include "ui_patientselector.h"

#include <coreplugin/icore.h>
#include <coreplugin/isettings.h>
#include <coreplugin/itheme.h>
#include <coreplugin/iuser.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/ipatient.h>
#include <coreplugin/imainwindow.h>
#include <coreplugin/constants_icons.h>
#include <coreplugin/constants_menus.h>
#include <coreplugin/modemanager/modemanager.h>

#include <utils/global.h>
#include <utils/widgets/datetimedelegate.h>
#include <translationutils/constanttranslations.h>

#include <QToolButton>
#include <QMenu>
#include <QSortFilterProxyModel>

#include <QDebug>

// Till this feature is not stable, you can remove the proxy model usage at anytime setting
// this to false
enum { UseProxyModel = false };

using namespace Patients;
using namespace Trans::ConstantTranslations;

static inline Core::ISettings *settings() {return Core::ICore::instance()->settings();}
static inline Core::ITheme *theme() {return Core::ICore::instance()->theme();}
static inline Core::IUser *user() {return Core::ICore::instance()->user();}
static inline Core::IMainWindow *mainWindow() {return Core::ICore::instance()->mainWindow();}
static inline Core::ActionManager *actionManager() { return Core::ICore::instance()->actionManager(); }
static inline Core::ModeManager *modeManager() {return Core::ICore::instance()->modeManager();}

namespace Patients {
namespace Internal {
class PatientSelectorPrivate
{
public:
    PatientSelectorPrivate(PatientSelector *parent) :
            ui(new Ui::PatientSelector),
            m_Model(0),
            m_proxyModel(0),
            q(parent)
    {
    }

    ~PatientSelectorPrivate()
    {
        delete ui;
    }

    void createSearchToolButtons()
    {
        m_SearchToolButton = new QToolButton();   // parent object will be redefined
        m_SearchToolButton->setPopupMode(QToolButton::InstantPopup);
        m_SearchToolButton->setIcon(theme()->icon(Core::Constants::ICONSEARCH));

        Core::Command *cmd;
        QStringList actions;
        actions
                << Constants::A_SEARCH_PATIENTS_BY_NAME
                << Constants::A_SEARCH_PATIENTS_BY_FIRSTNAME
                << Constants::A_SEARCH_PATIENTS_BY_NAMEFIRSTNAME
                << Constants::A_SEARCH_PATIENTS_BY_DOB;

        QList<QAction *> actionList;
        foreach(const QString &a, actions) {
            cmd = actionManager()->command(Core::Id(a));
            m_SearchToolButton->addAction(cmd->action());
            actionList << cmd->action();
        }

        int id = settings()->value(Constants::S_SEARCHMETHOD, 0).toInt();
        if (id < actionList.count() && id >= 0) {
            actionList.at(id)->trigger();
            actionList.at(id)->setChecked(true);
            m_SearchToolButton->setDefaultAction(actionList.at(id));
            m_SearchMethod = id;
        } else {
            m_SearchMethod = 0;
        }

        // add buttons to search line
        ui->searchLine->setLeftButton(m_SearchToolButton);

//        // add action to the navigation button
        m_NavigationToolButton = new QToolButton(q);   // parent object will be redefined
        m_NavigationToolButton->setPopupMode(QToolButton::InstantPopup);
        m_NavigationToolButton->setIcon(theme()->icon(Core::Constants::ICONPATIENT));
        m_NavigationMenu = new QMenu(m_NavigationToolButton);
        m_NavigationToolButton->setMenu(m_NavigationMenu);

//        // add buttons to search line
        ui->searchLine->setRightButton(m_NavigationToolButton);
    }

    void saveSettings()
    {
        settings()->setValue(Constants::S_SEARCHMETHOD, m_SearchMethod);
    }

public:
    Ui::PatientSelector *ui;
    PatientModel *m_Model;
    PatientSelector::FieldsToShow m_Fields;
    QToolButton *m_SearchToolButton, *m_NavigationToolButton;
    QMenu *m_NavigationMenu;
    int m_SearchMethod;
    QSortFilterProxyModel *m_proxyModel;

private:
    PatientSelector *q;
};
}  // End namespace Internal
}  // End namespace Patients


/** \brief Create a PatientSelector with \e fields to show. \sa PatientSelector::FieldsToShow */
PatientSelector::PatientSelector(QWidget *parent, const FieldsToShow fields) :
    QWidget(parent),
    d(new Internal::PatientSelectorPrivate(this))
{
    d->ui->setupUi(this);
    d->ui->searchLine->setDelayedSignals(true);

    // Get the active model
    d->m_proxyModel = new QSortFilterProxyModel(this);
    if (!PatientModel::activeModel()) {
        PatientModel *model = new PatientModel(this);
        PatientModel::setActiveModel(model);
        setPatientModel(model);
    } else {
        setPatientModel(PatientModel::activeModel());
    }

    // datetime delegate
    d->ui->tableView->setItemDelegateForColumn(Core::IPatient::DateOfBirth, new Utils::DateTimeDelegate(this, true));

    d->createSearchToolButtons();

    // Some connections
    connect(d->m_NavigationToolButton->menu(), SIGNAL(aboutToShow()), this, SLOT(updateNavigationButton()));
    connect(d->ui->searchLine, SIGNAL(textChanged(QString)), this, SLOT(refreshFilter(QString)));
    connect(d->ui->tableView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(changeIdentity(QModelIndex,QModelIndex)));
    connect(d->ui->tableView, SIGNAL(activated(QModelIndex)), this, SLOT(onPatientActivated(QModelIndex)));

    updatePatientActions(QModelIndex());

    if (fields == None) {
        d->m_Fields = FieldsToShow(settings()->value(Constants::S_SELECTOR_FIELDSTOSHOW, Default).toInt());
    } else {
        d->m_Fields = fields;
    }
    connect(user(), SIGNAL(userChanged()), this, SLOT(onUserChanged()));
}

PatientSelector::~PatientSelector()
{
    if (d) {
        d->saveSettings();
        delete d;
        d = 0;
    }
}

/** \brief Initialize view and actions, select the first available patient. */
void PatientSelector::initialize()
{
    // Here we assume that the Core::IPatient model is not already filtered
    QModelIndex current;
    if (!d->m_Model->currentPatient().isValid()) {
        current = d->m_Model->index(0,0);
        d->m_Model->blockSignals(true);
        d->m_Model->setCurrentPatient(current);
        d->m_Model->blockSignals(false);
    } else {
        current = d->m_Model->currentPatient();
    }

    if (UseProxyModel) {
        current = d->m_proxyModel->mapFromSource(current);
    }

    d->ui->tableView->selectRow(current.row());
    changeIdentity(current, QModelIndex());
}

void PatientSelector::updateNavigationButton()
{
    d->m_NavigationMenu->clear();

    Core::Command *cmd = actionManager()->command(Core::Constants::A_PATIENT_NEW);
    d->m_NavigationMenu->addAction(cmd->action());
    d->m_NavigationMenu->addSeparator();

    // add navigation actions
    Core::ActionContainer *navMenu = actionManager()->actionContainer(Core::Constants::M_PATIENTS_NAVIGATION);
    if (!navMenu)
        return;
    for(int i = 0; i < navMenu->menu()->actions().count(); ++i) {
        d->m_NavigationMenu->addAction(navMenu->menu()->actions().at(i));
    }
}

/** \brief Define the search mode of the selector. */
void PatientSelector::setSearchMode(const int search)
{
    d->m_SearchMethod = search;
}

/** \brief Define the PatientModel to use. */
void PatientSelector::setPatientModel(PatientModel *m)
{
    Q_ASSERT(m);
    d->m_Model = m;
    if (UseProxyModel) {
        d->m_proxyModel->setSourceModel(m);
        d->ui->tableView->setModel(d->m_proxyModel);
    } else {
        d->ui->tableView->setModel(m);
    }
    setFieldsToShow(d->m_Fields);

    d->ui->tableView->horizontalHeader()->setStretchLastSection(false);
    d->ui->tableView->horizontalHeader()->setResizeMode(Core::IPatient::BirthName, QHeaderView::Stretch);
    d->ui->tableView->horizontalHeader()->setResizeMode(Core::IPatient::SecondName, QHeaderView::Stretch);
    d->ui->tableView->horizontalHeader()->setResizeMode(Core::IPatient::Firstname, QHeaderView::Stretch);
    d->ui->tableView->horizontalHeader()->setResizeMode(Core::IPatient::FullName, QHeaderView::ResizeToContents);
    d->ui->tableView->horizontalHeader()->setResizeMode(Core::IPatient::IconizedGender, QHeaderView::ResizeToContents);
    d->ui->tableView->horizontalHeader()->setResizeMode(Core::IPatient::Title, QHeaderView::ResizeToContents);
    d->ui->tableView->horizontalHeader()->setResizeMode(Core::IPatient::DateOfBirth, QHeaderView::ResizeToContents);
    d->ui->tableView->horizontalHeader()->setResizeMode(Core::IPatient::FullAddress, QHeaderView::Stretch);
    d->ui->tableView->horizontalHeader()->setResizeMode(Core::IPatient::PractitionnerLkID, QHeaderView::ResizeToContents);

    if (UseProxyModel)
        d->ui->numberOfPatients->setText(QString::number(d->m_proxyModel->rowCount()));//m->numberOfFilteredPatients()));
    else
        d->ui->numberOfPatients->setText(QString::number(m->numberOfFilteredPatients()));
    d->ui->identity->setCurrentPatientModel(m);
    connect(d->m_Model, SIGNAL(currentPatientChanged(QModelIndex)), this, SLOT(setSelectedPatient(QModelIndex)));
}

/** \brief Define the fields to show using the FieldsToShow flag */
void PatientSelector::setFieldsToShow(const FieldsToShow fields)
{
    d->m_Fields = fields;
    // Hide all columns of the table view
    for(int i=0; i < Core::IPatient::NumberOfColumns; ++i) {
        d->ui->tableView->hideColumn(i);
    }
    // Show selected columns
    if (fields & PatientSelector::BirthName) {
        d->ui->tableView->showColumn(Core::IPatient::BirthName);
    }
    if (fields & PatientSelector::SecondName) {
        d->ui->tableView->showColumn(Core::IPatient::SecondName);
    }
    if (fields & PatientSelector::FirstName) {
        d->ui->tableView->showColumn(Core::IPatient::Firstname);
    }
    if (fields & PatientSelector::FullName) {
        d->ui->tableView->showColumn(Core::IPatient::FullName);
    }
    if (fields & PatientSelector::Gender) {
        d->ui->tableView->showColumn(Core::IPatient::IconizedGender);
    }
    if (fields & PatientSelector::Title) {
        d->ui->tableView->showColumn(Core::IPatient::Title);
    }
    if (fields & PatientSelector::DateOfBirth) {
        d->ui->tableView->showColumn(Core::IPatient::DateOfBirth);
    }
    if (fields & PatientSelector::FullAdress) {
        d->ui->tableView->showColumn(Core::IPatient::FullAddress);
    }
}

/**
 * \brief Define the selected patient (use this if patient was selected from outside the selector).
 * The index must be a Core::Patient model index.
*/
void PatientSelector::setSelectedPatient(const QModelIndex &index)
{
//    if (UseProxyModel) {
//        // Remove any filter
//        d->ui->searchLine->clear();
//        d->m_proxyModel->invalidate();
//        QModelIndex proxyIndex = d->m_proxyModel->mapFromSource(index);
//    }
    d->ui->tableView->selectRow(index.row());
    updatePatientActions(index);
}

/**
 * \brief Update the IdentityWidget with the new current identity.
 * \internal
 * This slot receive indexes from the proxymodel if the proxymodel is in use, otherwise
 * it receive indexes from the Core::IPatient model.
*/
void PatientSelector::changeIdentity(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    if (UseProxyModel) {
        QModelIndex source = d->m_proxyModel->mapToSource(current);
        d->ui->identity->setCurrentIndex(source);
        updatePatientActions(source);
    } else {
        d->ui->identity->setCurrentIndex(current);
        updatePatientActions(current);
    }
}

/*!
 * \brief Updates (enables/disables) the corresponding QActions for the given patient
 * \internal
 * Receive index from the Core::IPatient model (if the proxy model is in use you must
 * map the proxy index to the Core::IPatient index).
 */
void PatientSelector::updatePatientActions(const QModelIndex &index)
{
    const bool enabled = index.isValid();
    actionManager()->command(Core::Constants::A_PATIENT_VIEWIDENTITY)->action()->setEnabled(enabled);
    actionManager()->command(Core::Constants::A_PATIENT_REMOVE)->action()->setEnabled(enabled);
}

/**
 * \brief Refresh the search filter of the Patient::PatientModel
 * \internal
 * Refresh the search filter.
*/
void PatientSelector::refreshFilter(const QString &)
{
    if (!d->m_Model)
        return;
    QString text = d->ui->searchLine->text();

    qWarning() << "SELECTOR SEARCH FILTER" << text;

    QString name, firstname;
    if (UseProxyModel) {
        if (text.isEmpty()) {
            d->m_proxyModel->invalidate();
        } else {
            d->m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
            d->m_proxyModel->setFilterKeyColumn(Core::IPatient::BirthName);
            d->m_proxyModel->setFilterFixedString(text);
        }
    } else {
        switch (d->m_SearchMethod) {
        case SearchByName: name = text; break;
        case SearchByNameFirstname: name = text.mid(0,text.indexOf(";")).trimmed(); firstname = text.right(text.indexOf(";")); break;
        case SearchByFirstname: firstname = text; break;
        case SearchByDOB: break;
        }
        d->m_Model->setFilter(name, firstname);
        d->ui->numberOfPatients->setText(QString::number(d->m_Model->numberOfFilteredPatients()));
    }
}

/**
 * \brief Slot activated when the user selects a patient from the selector.
 * \internal
 * This slot receive proxy index if the proxymodel is in use.
 * \sa setSelectedPatient()
 */
void PatientSelector::onPatientActivated(const QModelIndex &index)
{
    QModelIndex idx = index;
    if (UseProxyModel)
        idx = d->m_proxyModel->mapToSource(index);

    if (d->m_Model && idx == d->m_Model->currentPatient()) {
        modeManager()->activateMode(Core::Constants::MODE_PATIENT_FILE);
        return;
    }

    mainWindow()->startProcessingSpinner();

    // Inform Core::IPatient model wrapper
    if (d->m_Model)
        d->m_Model->setCurrentPatient(idx);
}

/**
 * \brief Update the view on user changed
 * \internal
*/
void PatientSelector::onUserChanged()
{
    // TODO: reconnect user specific patient list (using the Practionner LKID)
    // reset the search filter
    d->ui->searchLine->clear();

    refreshFilter("");

    // re-initialize the view
    initialize();
}

/**
 * \brief Manages patientbar visibility and translatability
 * \internal
 */
bool PatientSelector::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::LanguageChange:
        d->ui->retranslateUi(this);
        break;
    case QEvent::Show:
        PatientBar::instance()->hide();
        break;
    case QEvent::Hide:
        PatientBar::instance()->show();
        break;
    default:
        break;
    }
    return QWidget::event(event);
}
