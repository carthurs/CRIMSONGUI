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


#include "gui/configdialog/configdialog.h"

#include <QMessageBox>
#include <QPushButton>

#include <QListWidget>
#include <QStackedWidget>
#include <QDialogButtonBox>

#include <QHBoxLayout>
#include <QVBoxLayout>


namespace qsapecng
{


ConfigDialog::ConfigDialog(QWidget* parent)
  : QDialog(parent)
{
  pages_ = new QStackedWidget;
  createPages();

  contents_ = new QListWidget;
  contents_->setViewMode(QListView::IconMode);
  contents_->setIconSize(QSize(64, 64));
  contents_->setMovement(QListView::Static);
  contents_->setMaximumWidth(128);
  contents_->setSpacing(12);
  createIcons();

  contents_->setCurrentRow(0);

  QDialogButtonBox* buttonBox = new QDialogButtonBox;
  QPushButton* closeButton = buttonBox->addButton(QDialogButtonBox::Close);
  QPushButton* applyButton = buttonBox->addButton(QDialogButtonBox::Apply);

  connect(this, SIGNAL(accepted()), this, SLOT(close()));
  connect(closeButton, SIGNAL(clicked(bool)), this, SLOT(checkBeforeClose()));
  connect(applyButton, SIGNAL(clicked(bool)), this, SLOT(apply()));

  QHBoxLayout* subLayout = new QHBoxLayout;
  subLayout->addWidget(contents_);
  subLayout->addWidget(pages_, Qt::AlignLeft);

  QVBoxLayout* mainLayout = new QVBoxLayout;
  mainLayout->addLayout(subLayout);
  mainLayout->addStretch(1);
  mainLayout->addSpacing(12);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  setWindowTitle(tr("Configuration"));
}


void ConfigDialog::changePage(QListWidgetItem* current, QListWidgetItem* previous)
{
  if(!current) {
    current = previous;
    pages_->setCurrentIndex(contents_->row(current));
  } else {
    checkPage();
    pages_->setCurrentIndex(contents_->row(current));
  }
}


void ConfigDialog::apply()
{
  ConfigPage* page =
    qobject_cast<ConfigPage*>(pages_->currentWidget());

  if(page)
    page->apply();
}


void ConfigDialog::checkPage()
{
  ConfigPage* page =
    qobject_cast<ConfigPage*>(pages_->currentWidget());

  if(page && page->isPageModified()) {
    QMessageBox::StandardButton button =
      QMessageBox::question(
        this,
        tr("Pending settings"),
        tr("Pending settings. Apply ?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
      );

    if(button == QMessageBox::Yes)
      page->apply();
  }
}


void ConfigDialog::checkBeforeClose()
{
  checkPage();
  emit accepted();
}


void ConfigDialog::createIcons()
{
  QListWidgetItem* generalButton = new QListWidgetItem(contents_);
  generalButton->setIcon(QIcon(":/images/icon.png"));
  generalButton->setText(tr("General"));
  generalButton->setTextAlignment(Qt::AlignHCenter);
  generalButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

  QListWidgetItem* fontButton = new QListWidgetItem(contents_);
  fontButton->setIcon(QIcon(":/images/font.png"));
  fontButton->setText(tr("Font"));
  fontButton->setTextAlignment(Qt::AlignHCenter);
  fontButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

  connect(contents_, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
    this, SLOT(changePage(QListWidgetItem*, QListWidgetItem*)));
}


void ConfigDialog::createPages()
{
  pages_->addWidget(generalPage_ = new GeneralPage);
  pages_->addWidget(fontPage_ = new FontPage);
}


}
