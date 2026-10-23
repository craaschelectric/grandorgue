/*
 * Copyright 2026 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#ifndef GOMIDIEVENTSTARTUPTAB_H
#define GOMIDIEVENTSTARTUPTAB_H

#include <functional>

#include <wx/panel.h>

#include "modification/GOModificationProxy.h"

class wxCheckBox;

class GODrawstop;

/**
 * The "Startup" tab of the MIDI event dialog of a stop, coupler, tremulant or
 * switch. It sets whether the stop is engaged when the organ loads and on
 * General Cancel, and optionally opens the voicing window of the stop.
 */
class GOMidiEventStartupTab : public wxPanel, public GOModificationProxy {
private:
  GODrawstop &r_drawstop;
  wxCheckBox *p_DefaultEngagedCheck;

  void OnDefaultEngagedClicked();

public:
  /**
   * @param parent the book control of the dialog
   * @param drawstop the stop that is configured. It must not be read-only
   * @param showVoicingFunction opens the voicing window of the stop. If it is
   *   empty then the voicing button is not shown
   */
  GOMidiEventStartupTab(
    wxWindow *parent,
    GODrawstop &drawstop,
    const std::function<void()> &showVoicingFunction);
};

#endif /* GOMIDIEVENTSTARTUPTAB_H */
