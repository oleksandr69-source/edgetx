/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "prefs_simu.h"
#include "ui_prefs_simu.h"
#include "eeprominterface.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QPalette>

#if defined(USE_SDL)
#include "joystick.h"
#include "joystickdialog.h"
#endif

PrefsSimuPanel::PrefsSimuPanel(QWidget * parent) :
  PrefsPanel(parent),
  ui(new Ui::PrefsSimu)

{
  ui->setupUi(this);
  lock = true;

  ui->chkScrnshotClipbd->setChecked(g.snapToClpbrd());
  ui->chkScrnshotClipbd->setBindSave([&] { g.snapToClpbrd(ui->chkScrnshotClipbd->isChecked()); });
  ui->chkScrnshotClipbd->setBindPostChanged([&] { this->update(); });

  ui->leScrnshotPath->setText(g.snapshotDir());
  ui->leScrnshotPath->setBindSave([&] { g.snapshotDir(ui->leScrnshotPath->text()); });
  ui->leScrnshotPath->setBindEnabled([&] { return !ui->chkScrnshotClipbd->isChecked(); });

  ui->btnScrnshotFldr->setup(tr("Select snapshot folder"), g.snapshotDir(),ui->leScrnshotPath);
  ui->btnScrnshotFldr->setBindEnabled([&] { return !ui->chkScrnshotClipbd->isChecked(); });
  connect(ui->btnScrnshotFldr, &AutoFolderSelectButton::folderChanged, this, [&] (QString fldr)
  {
    if (!fldr.isEmpty()) {
      ui->chkScrnshotClipbd->setChecked(false);
    }
  });

  connect(ui->btnClearSavedPosn, &QPushButton::released, this, [&] () {
    SimulatorOptions opts = profile.simulatorOptions();
    opts.controlsState.clear();
    profile.simulatorOptions(opts);
  });

  ui->chkSaveControls->setValue(g.simuSW());
  ui->chkSaveControls->setBindSave([&] { g.simuSW(ui->chkSaveControls->isChecked()); });

  ui->chkJoystickWarning->setValue(g.disableJoystickWarning());
  ui->chkJoystickWarning->setBindSave([&] { g.disableJoystickWarning(ui->chkJoystickWarning->isChecked()); });

  ui->chkScrollButtons->setValue(g.simuScrollButtons());
  ui->chkScrollButtons->setBindSave([&] { g.simuScrollButtons(ui->chkScrollButtons->isChecked()); });

  ui->cboGenericKeysPos->addItems(AppData::simuGenericKeysPosList());
  ui->cboGenericKeysPos->setCurrentIndex(g.simuGenericKeysPos());
  ui->cboGenericKeysPos->setBindSave([&] {
    g.simuGenericKeysPos((AppData::SimuGenericKeysPos)ui->cboGenericKeysPos->currentIndex());
  });

  ui->dsbVolumeGain->setValue(profile.volumeGain() / 10.0);
  ui->dsbVolumeGain->setBindSave([&] { profile.volumeGain(ui->dsbVolumeGain->value() * 10.0); });

  ui->lblBackLightColor->setBindVisible([&] { return Boards::getCapability(board, Board::HasBacklightColor); });

  ui->cboBackLightColor->addItems(AppData::simuBackLightColorList());
  ui->cboBackLightColor->setCurrentIndex((int)g.backLight());
  ui->cboBackLightColor->setBindSave([&] { g.backLight(ui->cboBackLightColor->currentIndex()); });
  ui->cboBackLightColor->setBindVisible([&] { return Boards::getCapability(board, Board::HasBacklightColor); });

  ui->lblCaseColorSample->setBindSave([&] { profile.radioSimCaseColor(ui->lblCaseColorSample->palette().button().color()); });
  ui->lblCaseColorSample->setPalette(profile.radioSimCaseColor());

  ui->btnCaseColor->setup(ui->lblCaseColorSample->palette().button().color(), ui->lblCaseColorSample);

  ui->chkBtnClkUseOSTheme->setChecked(profile.simBtnClickedUseOSTheme());
  ui->chkBtnClkUseOSTheme->setBindSave([&] { return profile.simBtnClickedUseOSTheme(ui->chkBtnClkUseOSTheme->isChecked()); });
  ui->chkBtnClkUseOSTheme->setBindPostChanged([&] { this->update(); });

  ui->lblBtnClkColorSample->setPalette(profile.simBtnClickedColor());
  ui->lblBtnClkColorSample->setBindSave([&] { return profile.simBtnClickedColor(ui->lblBtnClkColorSample->palette().button().color()); });
  ui->lblBtnClkColorSample->setBindEnabled([&] { return !ui->chkBtnClkUseOSTheme->isChecked(); });
  ui->lblBtnClkColorSample->setBindVisible([&] { return !ui->chkBtnClkUseOSTheme->isChecked(); });

  ui->btnBtnClkColor->setup(ui->lblBtnClkColorSample->palette().button().color(), ui->lblBtnClkColorSample);
  ui->btnBtnClkColor->setBindEnabled([&] { return !ui->chkBtnClkUseOSTheme->isChecked(); });
  ui->btnBtnClkColor->setBindVisible([&] { return !ui->chkBtnClkUseOSTheme->isChecked(); });

#if defined(USE_SDL)
  ui->chkJoystickEnable->setValue(g.jsSupport());
  ui->chkJoystickEnable->setBindSave([&] { g.jsSupport(ui->chkJoystickEnable->isChecked()); });

  ui->cboJoystick->addItems(joysticksList());
  ui->cboJoystick->setCurrentText(profile.jsName());
  ui->cboJoystick->setBindEnabled([&] { return ui->chkJoystickEnable->isChecked();} );
  ui->cboJoystick->setBindSave([&] {
    if (ui->cboJoystick->isEnabled()) {
      profile.jsName(ui->cboJoystick->currentText());
      g.loadNamedJS();
    }
  });

  ui->btnJoystickCalib->setBindEnabled([&] { return ui->chkJoystickEnable->isChecked();} );
  connect(ui->btnJoystickCalib, &QPushButton::released, this, [this]() {
    profile.jsName(ui->cboJoystick->currentText());
    joystickDialog *jd = new joystickDialog(this);
    jd->exec();
    delete jd;
  });
#endif

  update();
  lock = false;
}

PrefsSimuPanel::~PrefsSimuPanel()
{
  delete ui;
}

//  use AbstractPanel::save()
void PrefsSimuPanel::save()
{
  AbstractPanel::save();
}

void PrefsSimuPanel::update()
{
  AbstractPanel::update();
}

#if defined(USE_SDL)
QStringList PrefsSimuPanel::joysticksList() {
  QStringList lst;
  Joystick *joystick = new Joystick(nullptr, 0, false, 0);

  if ( joystick && joystick->joystickNames.count() > 0 ) {
    lst = joystick->joystickNames;
    joystick->close();
  } else {
    lst << tr("No joysticks found");
  }

  return lst;
}
#endif
