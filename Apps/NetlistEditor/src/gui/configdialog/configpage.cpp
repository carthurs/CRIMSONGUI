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


#include "gui/configdialog/configpage.h"
#include "gui/settings.h"

#include "logger/logger.h"

#include <QApplication>

#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>

#include <QtCore/QSignalMapper>

#include <QFontDialog>
#include <QColorDialog>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>


namespace qsapecng
{


GeneralPage::GeneralPage(QWidget* parent)
  : ConfigPage(parent)
{
  Settings settings;

  /*
   * Project Settings Box
   */
  QGroupBox* projectSettingsBox = new QGroupBox(
      tr("Project Settings"), this
    );
  projectSettingsBox->setCheckable(false);
  projectSettingsBox->setAlignment(Qt::AlignLeft);

  QHBoxLayout* projectSettingsLayout = new QHBoxLayout;
  projectSettingsLayout->addWidget(
    new QLabel(tr("Nothing to do here, that's good!! :-)")));

  projectSettingsBox->setLayout(projectSettingsLayout);

  /*
   * Log Settings Box
   */
  QGroupBox* logSettingsBox = new QGroupBox(
      tr("Log Settings"), this
    );
  projectSettingsBox->setCheckable(false);
  projectSettingsBox->setAlignment(Qt::AlignLeft);

  logLevel_ = new QComboBox;
  logLevel_->addItem(tr("Debug"), QVariant(sapecng::Logger::DEBUG));
  logLevel_->addItem(tr("Info"), QVariant(sapecng::Logger::INFO));
  logLevel_->addItem(tr("Warning"), QVariant(sapecng::Logger::WARNING));
  logLevel_->addItem(tr("Error"), QVariant(sapecng::Logger::ERROR));
  logLevel_->addItem(tr("Fatal"), QVariant(sapecng::Logger::FATAL));
  switch((sapecng::Logger::LogLevel) settings.logLvl())
  {
  case sapecng::Logger::DEBUG:
    logLevel_->setCurrentIndex(0);
    break;
  case sapecng::Logger::INFO:
    logLevel_->setCurrentIndex(1);
    break;
  case sapecng::Logger::WARNING:
    logLevel_->setCurrentIndex(2);
    break;
  case sapecng::Logger::ERROR:
    logLevel_->setCurrentIndex(3);
    break;
  case sapecng::Logger::FATAL:
    logLevel_->setCurrentIndex(4);
    break;
  }
  connect(logLevel_, SIGNAL(currentIndexChanged(int)),
    this, SLOT(levelChanged()));

  QLabel* logLevelLabel = new QLabel(tr("Log Level:"));
  logLevelLabel->setBuddy(logLevel_);

  QLabel* logColorLabel = new QLabel(tr("Log Colors"));
  logColorLabel->setAlignment(Qt::AlignCenter);

  colorMapper_ = new QSignalMapper(this);

  debugColor_ = new QPushButton;
  debugColor_->setAutoFillBackground(true);
  debugColor_->setText(tr("Debug"));
  debugColor_->setFlat(true);
  debugColor_->setPalette(QPalette(settings.debugColor()));
  colorMapper_->setMapping(debugColor_, sapecng::Logger::DEBUG);
  connect(debugColor_, SIGNAL(clicked()),
    colorMapper_, SLOT(map()));

  infoColor_ = new QPushButton;
  infoColor_->setAutoFillBackground(true);
  infoColor_->setText(tr("Info"));
  infoColor_->setFlat(true);
  infoColor_->setPalette(QPalette(settings.infoColor()));
  colorMapper_->setMapping(infoColor_, sapecng::Logger::INFO);
  connect(infoColor_, SIGNAL(clicked()),
    colorMapper_, SLOT(map()));

  warningColor_ = new QPushButton;
  warningColor_->setAutoFillBackground(true);
  warningColor_->setText(tr("Warning"));
  warningColor_->setFlat(true);
  warningColor_->setPalette(QPalette(settings.warningColor()));
  colorMapper_->setMapping(warningColor_, sapecng::Logger::WARNING);
  connect(warningColor_, SIGNAL(clicked()),
    colorMapper_, SLOT(map()));

  errorColor_ = new QPushButton;
  errorColor_->setAutoFillBackground(true);
  errorColor_->setText(tr("Error"));
  errorColor_->setFlat(true);
  errorColor_->setPalette(QPalette(settings.errorColor()));
  colorMapper_->setMapping(errorColor_, sapecng::Logger::ERROR);
  connect(errorColor_, SIGNAL(clicked()),
    colorMapper_, SLOT(map()));

  fatalColor_ = new QPushButton;
  fatalColor_->setAutoFillBackground(true);
  fatalColor_->setText(tr("Fatal"));
  fatalColor_->setFlat(true);
  fatalColor_->setPalette(QPalette(settings.fatalColor()));
  colorMapper_->setMapping(fatalColor_, sapecng::Logger::FATAL);
  connect(fatalColor_, SIGNAL(clicked()),
    colorMapper_, SLOT(map()));

  connect(colorMapper_, SIGNAL(mapped(int)),
    this, SLOT(changeColor(int)));

  QHBoxLayout* logLevelLayout = new QHBoxLayout;
  logLevelLayout->addWidget(logLevelLabel);
  logLevelLayout->addWidget(logLevel_);

  QHBoxLayout* logColorLayout = new QHBoxLayout;
  logColorLayout->addWidget(debugColor_);
  logColorLayout->addWidget(infoColor_);
  logColorLayout->addWidget(warningColor_);
  logColorLayout->addWidget(errorColor_);
  logColorLayout->addWidget(fatalColor_);

  QGridLayout* logSettingsLayout = new QGridLayout;
  logSettingsLayout->addLayout(logLevelLayout, 1, 1);
  logSettingsLayout->addWidget(logColorLabel, 0, 2);
  logSettingsLayout->addLayout(logColorLayout, 1, 2);
  logSettingsLayout->setHorizontalSpacing(32);
  logSettingsLayout->setColumnStretch(0, 1);
  logSettingsLayout->setColumnStretch(3, 1);

  logSettingsBox->setLayout(logSettingsLayout);

  /*
   * Main layout
   */
  QVBoxLayout* mainLayout = new QVBoxLayout;
  mainLayout->addWidget(projectSettingsBox);
  mainLayout->addWidget(logSettingsBox);
  mainLayout->addStretch();
  setLayout(mainLayout);
}


void GeneralPage::apply()
{
  SettingsManager manager;

  manager.setLogLvl(logLevel_->itemData(logLevel_->currentIndex()).toInt());
  manager.setDebugColor(debugColor_->palette().color(QPalette::Background));
  manager.setInfoColor(infoColor_->palette().color(QPalette::Background));
  manager.setWarningColor(warningColor_->palette().color(QPalette::Background));
  manager.setErrorColor(errorColor_->palette().color(QPalette::Background));
  manager.setFatalColor(fatalColor_->palette().color(QPalette::Background));

  setPageModified(false);
}


void GeneralPage::changeColor(int id)
{
  QColor color;
  sapecng::Logger::LogLevel level =
    (sapecng::Logger::LogLevel) id;

  switch(level)
  {
  case sapecng::Logger::DEBUG:
    color = QColorDialog::getColor(
        debugColor_->palette().color(QPalette::Background),
        this, tr("Choose a color")
      );
    if(color.isValid()) {
      debugColor_->setPalette(QPalette(color));
      setPageModified(true);
    }
    break;
  case sapecng::Logger::INFO:
    color = QColorDialog::getColor(
        infoColor_->palette().color(QPalette::Background),
        this, tr("Choose a color")
      );
    if(color.isValid()) {
      infoColor_->setPalette(QPalette(color));
      setPageModified(true);
    }
    break;
  case sapecng::Logger::WARNING:
    color = QColorDialog::getColor(
        warningColor_->palette().color(QPalette::Background),
        this, tr("Choose a color")
      );
    if(color.isValid()) {
      warningColor_->setPalette(QPalette(color));
      setPageModified(true);
    }
    break;
  case sapecng::Logger::ERROR:
    color = QColorDialog::getColor(
        errorColor_->palette().color(QPalette::Background),
        this, tr("Choose a color")
      );
    if(color.isValid()) {
      errorColor_->setPalette(QPalette(color));
      setPageModified(true);
    }
    break;
  case sapecng::Logger::FATAL:
    color = QColorDialog::getColor(
        fatalColor_->palette().color(QPalette::Background),
        this, tr("Choose a color")
      );
    if(color.isValid()) {
      fatalColor_->setPalette(QPalette(color));
      setPageModified(true);
    }
    break;
  }
}


void GeneralPage::levelChanged()
{
  setPageModified(true);
}


FontPage::FontPage(QWidget* parent)
  : ConfigPage(parent)
{
  fontPage_ = new QFontDialog;
  fontPage_->setCurrentFont(QApplication::font());
  fontPage_->setOption(QFontDialog::NoButtons, true);

  connect(fontPage_, SIGNAL(currentFontChanged(const QFont&)),
    this, SLOT(currentFontChanged()));

  QHBoxLayout* mainLayout = new QHBoxLayout;
  mainLayout->addWidget(fontPage_);
  setLayout(mainLayout);
}


void FontPage::apply()
{
  SettingsManager().setAppFont(fontPage_->currentFont());
  QApplication::setFont(fontPage_->currentFont());
  setPageModified(false);
}


void FontPage::currentFontChanged()
{
  setPageModified(true);
}


}
