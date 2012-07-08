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
  \class Alert::BlockingAlertResult
  Contains the result of a blocking dialog:
    - user accepted,
    - override & user override comment,
    - read alerts (all view alerts are logged)
    - an AlertValidation to ease the validation process
*/

/**
  \fn Alert::BlockingAlertResult::BlockingAlertResult()
  Construct an empty invalid result (not accepted, not overridden);
*/

/**
  \fn void Alert::BlockingAlertResult::setOverriden(bool override)
  Define the user override status.
*/

/**
  \fn bool Alert::BlockingAlertResult::isOverridenByUser() const
  Return the user override status.
*/

/**
  \fn void Alert::BlockingAlertResult::setOverrideUserComment(const QString &comment)
  Define the user override comment.
*/

/**
  \fn QString Alert::BlockingAlertResult::overrideUserComment() const
  Return the user override comment.
*/

/**
  \fn void Alert::BlockingAlertResult::setAccepted(bool accepted)
  Define the status of the BlockingAlertDialog to accepted.
*/

/**
  \fn bool Alert::BlockingAlertResult::isAccepted() const
  Return true if the dialogue was accepted.
*/

/**
  \fn void Alert::BlockingAlertResult::setReadAlertUid(const QStringList &uids)
  Log all read alerts.
*/

/**
  \fn QStringList Alert::BlockingAlertResult::readAlertsUid() const
  Return all read alerts. If a blocking dialog is started with more than one alert, all alerts
  visualized by the user are loggued.
*/

/**
  \fn void Alert::BlockingAlertResult::setAlertValidation(const AlertValidation &validation)
  Set the Alert::AlertValidation according to the blocking dialog result.
*/

/**
  \fn AlertValidation Alert::BlockingAlertResult::alertValidation() const
  Return the Alert::AlertValidation suitable to the user actions.
*/

#include "blockingalertdialog.h"
#include "alertitem.h"
#include "alertcore.h"
#include "ui_BlockingAlertdialog.h"
#include "ui_BlockingAlertdialogoverridingcomment.h"

#include <coreplugin/icore.h>
#include <coreplugin/iuser.h>
#include <coreplugin/itheme.h>
#include <coreplugin/ipatient.h>
#include <coreplugin/constants_icons.h>
#include <coreplugin/imainwindow.h>

#include <utils/global.h>
#include <translationutils/constants.h>
#include <translationutils/trans_current.h>

#include <QDialogButtonBox>
#include <QToolButton>
#include <QScrollArea>
#include <QDesktopWidget>

#include <QDebug>

using namespace Alert;
using namespace Trans::ConstantTranslations;

static inline Core::ITheme *theme() {return Core::ICore::instance()->theme();}
static inline Core::IUser *user() {return Core::ICore::instance()->user();}
static inline Core::IPatient *patient() {return Core::ICore::instance()->patient();}

static void addAlertToLayout(const AlertItem &alert, bool showCategory, QLayout *lay)
{
    QLabel *label = new QLabel(lay->parentWidget());
    label->setTextFormat(Qt::RichText);
    label->setAlignment(Qt::AlignLeft);
    label->setWordWrap(true);
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    label->setText(alert.htmlToolTip(showCategory));
    lay->addWidget(label);
}

BlockingAlertDialog::BlockingAlertDialog(const QList<AlertItem> &items,
                                       const QString &themedIcon,
                                       const QList<QAbstractButton *> &buttons,
                                       QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BlockingAlertDialog),
    cui(0),
    _overrideButton(0),
    _remindLaterButton(0),
    _overrideCommentRequired(false),
    _remind(false)
{
    // Do we need to ask for an overriding comment
    foreach(const AlertItem &item, items) {
        if (item.isOverrideRequiresUserComment()) {
            _overrideCommentRequired = true;
            break;
        }
    }

    // Prepare the ui
    ui->setupUi(this);
    layout()->setSpacing(5);
    setWindowTitle(tkTr(Trans::Constants::BLOCKING_ALERT));
    setWindowModality(Qt::WindowModal);

    // Manage the general icon of the dialog
    if (!themedIcon.isEmpty() && QFile(theme()->iconFullPath(themedIcon, Core::ITheme::BigIcon)).exists()) {
        ui->generalIconLabel->setPixmap(theme()->icon(themedIcon, Core::ITheme::BigIcon).pixmap(64,64));
        setWindowIcon(theme()->icon(themedIcon));
    } else {
        int maxPriority = AlertItem::Low;
        for(int i=0; i<items.count();++i) {
            maxPriority = qMax(maxPriority, int(items.at(i).priority()));
        }
        ui->generalIconLabel->setPixmap(AlertItem::priorityBigIcon(AlertItem::Priority(maxPriority)).pixmap(64,64));
        setWindowIcon(AlertItem::priorityBigIcon(AlertItem::Priority(maxPriority)));
    }

    // Include alerts
    bool canRemind = true;
    QFont bold;
    bold.setBold(true);
    if (items.count()==1) {
        // No tabwidget
        const AlertItem &alert = items.at(0);
        if (!alert.isRemindLaterAllowed())
            canRemind = false;

        QWidget *alertContainer = new QWidget(this);
        QVBoxLayout *central = new QVBoxLayout(alertContainer);
        addAlertToLayout(alert, true, central);

        QWidget *scrollContainer = new QWidget(this);
        QVBoxLayout *containerLayout = new QVBoxLayout(scrollContainer);
        containerLayout->setMargin(0);
        scrollContainer->setLayout(containerLayout);

        QScrollArea *scroll = new QScrollArea(this);
        scroll->setBackgroundRole(QPalette::Background);
        scroll->setWidgetResizable(true);
        scroll->setFrameStyle(QFrame::NoFrame);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        scroll->setWidget(alertContainer);
        containerLayout->addWidget(scroll);

        ui->centralLayout->addWidget(scrollContainer);
        ui->nbLabel->hide();
    } else {
        // With tabwidget
        QHash<QString, QVBoxLayout *> categories;
        QHash<QString, int> nbInCategories;

        for(int i=0; i < items.count(); ++i) {
            const AlertItem &alert = items.at(i);
            if (!alert.isRemindLaterAllowed())
                canRemind = false;

            QString cat = alert.category();
            if (cat.isEmpty()) {
                cat = tkTr(Trans::Constants::ALERT);
            }

            // Get the category layout
            nbInCategories.insert(cat, nbInCategories.value(cat, 0) + 1);
            QVBoxLayout *lay = categories.value(cat, 0);
            if (!lay) {
                lay = new QVBoxLayout;
                categories.insert(cat, lay);
            } else {
                QFrame *line = new QFrame(this);
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                lay->addWidget(line);
            }
            addAlertToLayout(alert, false, lay);
        }

        QTabWidget *tab = new QTabWidget(this);
        // Create a tab for each category
        foreach(const QString &cat, categories.keys()) {
            QWidget *alertContainer = new QWidget(this);
            QVBoxLayout *central = categories.value(cat);
            alertContainer->setLayout(central);

            QWidget *scrollContainer = new QWidget(this);
            QVBoxLayout *containerLayout = new QVBoxLayout(scrollContainer);
            containerLayout->setMargin(0);
//            container->setLayout(containerLayout);

            QScrollArea *scroll = new QScrollArea(this);
            scroll->setBackgroundRole(QPalette::Background);
            scroll->setWidgetResizable(true);
            scroll->setFrameStyle(QFrame::NoFrame);
            scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

            scroll->setWidget(alertContainer);
            containerLayout->addWidget(scroll);

            tab->addTab(scrollContainer, QString("%1 (%2)").arg(cat).arg(nbInCategories.value(cat)));
        }
        ui->centralLayout->addWidget(tab);
        ui->nbLabel->setText(tkTr(Trans::Constants::_1_ALERTS).arg(items.count()));
    }

    // Add buttons
    _box = new QDialogButtonBox(Qt::Horizontal, this);
    QToolButton *accept = new QToolButton(this);
    accept->setMinimumHeight(22);
    accept->setText(tkTr(Trans::Constants::VALIDATE));
    accept->setIcon(theme()->icon(Core::Constants::ICONOK, Core::ITheme::SmallIcon));
    accept->setIconSize(QSize(16,16));
    accept->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    accept->setFont(bold);
    _box->addButton(accept, QDialogButtonBox::AcceptRole);

    if (canRemind) {
        _remindLaterButton = new QToolButton(this);
        _remindLaterButton->setMinimumHeight(22);
        _remindLaterButton->setText(tkTr(Trans::Constants::REMIND_LATER));
        _remindLaterButton->setIcon(theme()->icon(Core::Constants::ICONREMINDER, Core::ITheme::SmallIcon));
        _remindLaterButton->setIconSize(QSize(16,16));
        _remindLaterButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        _remindLaterButton->setFont(bold);
        _box->addButton(_remindLaterButton, QDialogButtonBox::AcceptRole);
    }

    _overrideButton = new QToolButton(this);
    _overrideButton->setMinimumHeight(22);
    _overrideButton->setText(tkTr(Trans::Constants::OVERRIDE));
    _overrideButton->setIcon(theme()->icon(Core::Constants::ICONOVERRIDE, Core::ITheme::SmallIcon));
    _overrideButton->setIconSize(QSize(16,16));
    _overrideButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    _overrideButton->setFont(bold);
    _box->addButton(_overrideButton, QDialogButtonBox::ResetRole);

    for(int i=0; i < buttons.count(); ++i) {
        buttons.at(i)->setIconSize(QSize(16,16));
        buttons.at(i)->setFont(bold);
        _box->addButton(buttons.at(i), QDialogButtonBox::ActionRole);
    }

    connect(accept, SIGNAL(clicked()), this, SLOT(accept()));
    if (_remindLaterButton)
        connect(_remindLaterButton, SIGNAL(clicked()), this, SLOT(remindLater()));
    connect(_overrideButton, SIGNAL(clicked()), this, SLOT(override()));
    ui->buttonLayout->setMargin(0);
    ui->buttonLayout->setSpacing(0);
    ui->buttonLayout->addWidget(_box);

    Utils::resizeAndCenter(this, QApplication::activeWindow());
}

BlockingAlertDialog::~BlockingAlertDialog()
{
    delete ui;
    if (cui) delete cui; cui=0;
}

// TODO: create a done(int r) and check if alert tagged with mustBeRead() was visualized by the user.

void BlockingAlertDialog::remindLater()
{
//    qWarning() << "BlockingAlertDialog::remindLater()";
    _remind = true;
    accept();
}

void BlockingAlertDialog::override()
{
    if (!_overrideCommentRequired) {
        reject();
        return;
    }

    // Append the comment
    cui = new Ui::BlockingAlertDialogOverridingComment;
    QWidget *w = new QWidget(this);
    cui->setupUi(w);
    ui->centralLayout->addWidget(w);
    connect(cui->validateComment, SIGNAL(clicked()), this, SLOT(validateUserOverridingComment()));

    _overrideButton->hide();
}

void BlockingAlertDialog::validateUserOverridingComment()
{
    if (!cui->overridingComment->toPlainText().isEmpty())
        reject();
}

QString BlockingAlertDialog::overridingComment() const
{
    if (cui)
        return cui->overridingComment->toPlainText();
    return QString::null;
}

void BlockingAlertDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
}

/** Execute a blocking alert dialog with the alerts \e item, using a general icon \e themedIcon.  Whatever is the result of the dialog, alerts are not modified. */
BlockingAlertResult BlockingAlertDialog::executeBlockingAlert(const AlertItem &item, const QString &themedIcon, QWidget *parent)
{
    QList<QAbstractButton*> noButtons;
    return executeBlockingAlert(QList<AlertItem>() << item, noButtons, themedIcon, parent);
}

/** Execute a blocking alert dialog with a list of alerts \e items, using a general icon \e themedIcon.  Whatever is the result of the dialog, alerts are not modified. */
BlockingAlertResult BlockingAlertDialog::executeBlockingAlert(const QList<AlertItem> &items, const QString &themedIcon, QWidget *parent)
{
    QList<QAbstractButton*> noButtons;
    return executeBlockingAlert(items, noButtons, themedIcon, parent);
}

/** Execute a blocking alert dialog with a list of alerts \e items, including extra-buttons \e buttons, using a general icon \e themedIcon. Whatever is the result of the dialog, alerts are not modified. */
BlockingAlertResult BlockingAlertDialog::executeBlockingAlert(const QList<AlertItem> &items, const QList<QAbstractButton*> &buttons, const QString &themedIcon, QWidget *parent)
{
    BlockingAlertResult result;
    BlockingAlertDialog dlg(items, themedIcon, buttons, parent);  // theme()->icon(themedIcon, Core::ITheme::BigIcon)
    if (dlg.exec()==QDialog::Accepted) {
        result.setAccepted(true);
        result.setRemindLaterRequested(dlg.isRemindLaterRequested());
    } else {
        result.setAccepted(false);
        result.setRemindLaterRequested(false);
        result.setOverriden(true);
        result.setOverrideUserComment(dlg.overridingComment());
    }
    return result;
}

/**
  Apply the blocking alert dialog result to alerts \e items. \n
  Alerts are modified in the list and Alert::AlertCore is informed of the modification.
  \sa Alert::AlertCore::updateAlert()
*/
bool BlockingAlertDialog::applyResultToAlerts(AlertItem &item, const BlockingAlertResult &result)
{
    if (result.isRemindLaterRequested()) {
        return item.remindLater();
    }

    QString validator;
    user() ? validator = user()->uuid() : validator = "UnknownUser";
    return item.validateAlert(validator, result.isOverridenByUser(), result.overrideUserComment(), QDateTime::currentDateTime());
}

/**
  Apply the blocking alert dialog result to alerts \e items. \n
  Alerts are modified in the list and Alert::AlertCore is informed of the modification.
  \sa Alert::AlertCore::updateAlert()
*/
bool BlockingAlertDialog::applyResultToAlerts(QList<AlertItem> &items, const BlockingAlertResult &result)
{
    if (result.isRemindLaterRequested()) {
        for(int i=0; i < items.count(); ++i) {
            AlertItem &item = items[i];
            item.remindLater();
        }
        return true;
    }

    bool ok = true;
    for(int i=0; i < items.count(); ++i) {
        AlertItem &item = items[i];
        if (!applyResultToAlerts(item, result))
            ok = false;
    }
    return ok;
}
