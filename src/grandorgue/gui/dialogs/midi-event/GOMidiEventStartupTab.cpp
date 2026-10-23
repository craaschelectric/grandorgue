/*
 * Copyright 2026 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#include "GOMidiEventStartupTab.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/intl.h>
#include <wx/sizer.h>

#include "model/GODrawstop.h"

GOMidiEventStartupTab::GOMidiEventStartupTab(
  wxWindow *parent,
  GODrawstop &drawstop,
  const std::function<void()> &showVoicingFunction)
  : wxPanel(parent, wxID_ANY), r_drawstop(drawstop) {
  wxBoxSizer *const pSizer = new wxBoxSizer(wxVERTICAL);

  p_DefaultEngagedCheck = new wxCheckBox(
    this, wxID_ANY, _("Engaged when the organ loads and on General Cancel"));
  p_DefaultEngagedCheck->SetValue(drawstop.IsDefaultEngaged());
  p_DefaultEngagedCheck->Bind(
    wxEVT_CHECKBOX, [this](wxCommandEvent &) { OnDefaultEngagedClicked(); });
  pSizer->Add(p_DefaultEngagedCheck, 0, wxALL, 10);

  if (showVoicingFunction) {
    wxButton *const pVoicingButton
      = new wxButton(this, wxID_ANY, _("Voicing for this stop..."));

    pVoicingButton->Bind(wxEVT_BUTTON, [showVoicingFunction](wxCommandEvent &) {
      showVoicingFunction();
    });
    pSizer->Add(pVoicingButton, 0, wxALL, 10);
  }
  SetSizer(pSizer);
}

void GOMidiEventStartupTab::OnDefaultEngagedClicked() {
  // It takes effect at once, the stop moves now
  r_drawstop.SetDefaultEngaged(p_DefaultEngagedCheck->GetValue());
  GOModificationProxy::OnIsModifiedChanged(true);
}
