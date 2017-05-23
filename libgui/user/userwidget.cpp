/*
 * userwidget.cpp
 * Copyright 2017 - ~, Apin <apin.klas@gmail.com>
 *
 * This file is part of Sultan.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "userwidget.h"
#include "ui_normalwidget.h"
#include "horizontalheader.h"
#include "tablewidget.h"
#include "tablemodel.h"
#include "tableitem.h"
#include "global_constant.h"
#include "message.h"
#include "useradddialog.h"
#include "tableview.h"
#include "userpermissiondialog.h"
#include "usersession.h"
#include "guiutil.h"
#include "headerwidget.h"
#include "db_constant.h"
#include <QMessageBox>
#include <QPushButton>

using namespace LibGUI;
using namespace LibG;

UserWidget::UserWidget(LibG::MessageBus *bus, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NormalWidget),
    mTableWidget(new TableWidget(this)),
    mAddDialog(new UserAddDialog(this))
{
    setMessageBus(bus);
    ui->setupUi(this);
    ui->labelTitle->setText(tr("User"));
    ui->verticalLayout->addWidget(mTableWidget);
    mTableWidget->initCrudButton();
    auto model = mTableWidget->getModel();
    model->setMessageBus(bus);
    model->addColumn("username", tr("Username"));
    model->addColumn("name", tr("Name"));
    model->addColumn("phone", tr("Phone"));
    model->addColumn("address", tr("Address"));
    model->addHeaderFilter("username", HeaderFilter{HeaderWidget::LineEdit, COMPARE::LIKE});
    model->addHeaderFilter("name", HeaderFilter{HeaderWidget::LineEdit, COMPARE::LIKE});
    model->setTypeCommand(MSG_TYPE::USER, MSG_COMMAND::QUERY);
    mTableWidget->setupTable();
    GuiUtil::setColumnWidth(mTableWidget->getTableView(), QList<int>() << 150 << 150 << 150 << 150);
    mTableWidget->getTableView()->horizontalHeader()->setStretchLastSection(true);
    model->refresh();
    auto button = mTableWidget->addActionButton(QIcon(":/images/16x16/key.png"));
    connect(button, SIGNAL(clicked(bool)), SLOT(permissionClicked()));
    connect(mTableWidget, SIGNAL(addClicked()), SLOT(addClicked()));
    connect(mTableWidget, SIGNAL(updateClicked(QModelIndex)), SLOT(updateClicked(QModelIndex)));
    connect(mTableWidget, SIGNAL(deleteClicked(QModelIndex)), SLOT(deleteClicked(QModelIndex)));
    connect(mAddDialog, SIGNAL(saveData(QVariantMap,int)), SLOT(saveRequested(QVariantMap,int)));
}

UserWidget::~UserWidget()
{
    delete ui;
}

void UserWidget::messageReceived(Message *msg)
{
    if(msg->isCommand(MSG_COMMAND::INSERT)) {
        if(msg->isSuccess()) {
            mAddDialog->hide();
            mTableWidget->getModel()->refresh();
        } else {
            QMessageBox::critical(mAddDialog, tr("Error"), msg->data("error").toString());
            mAddDialog->enableSave();
            return;
        }
    } else if(msg->isCommand(MSG_COMMAND::UPDATE)) {
        if(msg->isSuccess()) {
            mAddDialog->hide();
            mTableWidget->getModel()->refresh();
        } else {
            QMessageBox::critical(mAddDialog, tr("Error"), msg->data("error").toString());
            mAddDialog->enableSave();
            return;
        }
    } else if(msg->isCommand(MSG_COMMAND::DEL) && msg->isSuccess()) {
        mTableWidget->getModel()->refresh();
    }
}

void UserWidget::addClicked()
{
    mAddDialog->reset();
    mAddDialog->show();
}

void UserWidget::updateClicked(const QModelIndex &index)
{
    auto item = static_cast<TableItem*>(index.internalPointer());
    mAddDialog->fill(item->data());
    mAddDialog->show();
}

void UserWidget::deleteClicked(const QModelIndex &index)
{
    auto item = static_cast<TableItem*>(index.internalPointer());
    int ret = QMessageBox::question(this, tr("Confirmation"), tr("Are you sure want to remove user?"));
    if(ret == QMessageBox::Yes) {
        Message msg(MSG_TYPE::USER, MSG_COMMAND::DEL);
        msg.addData("id", item->id);
        sendMessage(&msg);
    }
}

void UserWidget::saveRequested(const QVariantMap &data, int id)
{
    Message msg(MSG_TYPE::USER, MSG_COMMAND::INSERT);
    if(id <= 0) {
        msg.setData(data);
    } else {
        msg.setCommand(MSG_COMMAND::UPDATE);
        msg.addData("id", id);
        msg.addData("data", data);
    }
    sendMessage(&msg);
}

void UserWidget::permissionClicked()
{
    const QModelIndex &index = mTableWidget->getTableView()->currentIndex();
    if(!index.isValid()) return;
    auto item = static_cast<TableItem*>(index.internalPointer());
    if(item->id == UserSession::id()) {
        QMessageBox::critical(this, tr("Error"), tr("You can't change your own"));
        return;
    }
    UserPermissionDialog dialog(item->data());
    connect(&dialog, SIGNAL(saveData(QVariantMap,int)), SLOT(saveRequested(QVariantMap,int)));
    dialog.exec();
}
