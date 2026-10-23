/*
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2026 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#include "GOVoicingDialog.h"

#include <algorithm>
#include <cmath>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/gdicmn.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include "model/pipe-config/GOPipeConfigNode.h"

// How a parameter is shown
struct ParameterLimits {
  // slider steps per unit
  double sliderScale;
  // slider range in units. The spin controls accept more
  double sliderLimit;
  double spinLimit;
  double spinStep;
  int nDigits;
};

static const ParameterLimits PARAMETER_LIMITS[] = {
  {10.0, 24.0, 60.0, 0.1, 1},    // GAIN, dB
  {10.0, 100.0, 1800.0, 0.5, 1}, // MANUAL_TUNING, cent
  {10.0, 100.0, 1800.0, 0.5, 1}, // AUTO_TUNING_CORRECTION, cent
  {1.0, 100.0, 100.0, 1.0, 0}    // TONE_BALANCE
};

static const int NOTES_PER_OCTAVE = 12;
static const int NOTE_SLIDER_HEIGHT = 110;
static const int SPIN_WIDTH = 64;

static wxString note_name(int midiNoteNumber) {
  static const char *const NAMES[NOTES_PER_OCTAVE]
    = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

  return wxString::Format(
    wxT("%s%d"),
    NAMES[midiNoteNumber % NOTES_PER_OCTAVE],
    midiNoteNumber / NOTES_PER_OCTAVE - 1);
}

static int offset_to_slider(double offset, const ParameterLimits &limits) {
  const double limit = limits.sliderLimit * limits.sliderScale;

  return (int)std::clamp(
    std::round(offset * limits.sliderScale), -limit, limit);
}

static double slider_to_offset(int sliderValue, const ParameterLimits &limits) {
  return sliderValue / limits.sliderScale;
}

GOVoicingDialog::GOVoicingDialog(
  wxWindow *pParent,
  const std::vector<GOPipeConfigNode *> &mpNodes,
  const std::function<bool()> &saveFunction,
  const std::function<void(bool)> &holdAutoSaveFunction)
  : wxDialog(
    pParent,
    wxID_ANY,
    _("Voicing"),
    wxDefaultPosition,
    wxDefaultSize,
    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    m_SaveFunction(saveFunction),
    m_HoldAutoSaveFunction(holdAutoSaveFunction),
    m_Parameter(GOPipeVoicing::GAIN),
    m_IsRefreshing(false),
    p_ParameterChoice(nullptr),
    p_MasterSlider(nullptr),
    p_MasterSpin(nullptr),
    p_ResetButton(nullptr),
    p_SaveButton(nullptr) {
  if (m_HoldAutoSaveFunction)
    m_HoldAutoSaveFunction(true);
  m_Voicing.Select(mpNodes);
  if (!mpNodes.empty() && mpNodes.front())
    SetTitle(wxString::Format(_("Voicing - %s"), mpNodes.front()->GetName()));

  wxBoxSizer *const pMainSizer = new wxBoxSizer(wxVERTICAL);
  wxPanel *const pTop = new wxPanel(this);

  CreateTopRow(pTop);
  pMainSizer->Add(pTop, 0, wxEXPAND | wxALL, 5);

  // The note controls scroll, so the window can be small (a 1024x600 screen)
  wxScrolledWindow *const pScroll = new wxScrolledWindow(
    this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL | wxVSCROLL);

  pScroll->SetScrollRate(10, 10);
  pScroll->SetMinSize(wxSize(200, 150));
  CreateNoteControls(pScroll);
  pScroll->FitInside();
  pMainSizer->Add(pScroll, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);

  wxBoxSizer *const pButtons = new wxBoxSizer(wxHORIZONTAL);

  p_ResetButton = new wxButton(this, wxID_ANY, _("Reset"));
  p_SaveButton = new wxButton(this, wxID_ANY, _("Save"));
  pButtons->AddStretchSpacer(1);
  pButtons->Add(p_ResetButton, 0, wxALL, 5);
  pButtons->Add(p_SaveButton, 0, wxALL, 5);
  pMainSizer->Add(pButtons, 0, wxEXPAND | wxALL, 5);
  SetSizer(pMainSizer);

  const wxRect displayRect = wxGetClientDisplayRect();

  SetSize(
    std::min(1000, displayRect.width - 16),
    std::min(560, displayRect.height - 60));

  p_ResetButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { OnReset(); });
  p_SaveButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { OnSave(); });
  Bind(wxEVT_CLOSE_WINDOW, &GOVoicingDialog::OnCloseWindow, this);

  ApplyParameterLimits();
  RefreshAll();
}

void GOVoicingDialog::CreateTopRow(wxPanel *pPanel) {
  wxBoxSizer *const pRow = new wxBoxSizer(wxHORIZONTAL);

  p_ParameterChoice = new wxChoice(pPanel, wxID_ANY);
  p_ParameterChoice->Append(_("Gain (dB)"));
  p_ParameterChoice->Append(_("Manual tuning (cent)"));
  p_ParameterChoice->Append(_("Auto-tuning correction (cent)"));
  p_ParameterChoice->Append(_("Tone balance"));
  p_ParameterChoice->SetSelection(m_Parameter);
  p_ParameterChoice->Bind(
    wxEVT_CHOICE, [this](wxCommandEvent &) { OnParameterChanged(); });
  pRow->Add(
    new wxStaticText(pPanel, wxID_ANY, _("Parameter:")),
    0,
    wxALIGN_CENTER_VERTICAL | wxRIGHT,
    5);
  pRow->Add(p_ParameterChoice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);

  pRow->Add(
    new wxStaticText(pPanel, wxID_ANY, _("MSTR (all pipes):")),
    0,
    wxALIGN_CENTER_VERTICAL | wxRIGHT,
    5);
  p_MasterSlider = new wxSlider(
    pPanel, wxID_ANY, 0, -1, 1, wxDefaultPosition, wxSize(200, -1));
  p_MasterSpin = new wxSpinCtrlDouble(
    pPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1));
  p_MasterSlider->Bind(wxEVT_SLIDER, [this](wxCommandEvent &) {
    OnMasterChanged(slider_to_offset(
      p_MasterSlider->GetValue(), PARAMETER_LIMITS[m_Parameter]));
  });
  p_MasterSpin->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent &) {
    OnMasterChanged(p_MasterSpin->GetValue());
  });
  pRow->Add(p_MasterSlider, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  pRow->Add(p_MasterSpin, 0, wxALIGN_CENTER_VERTICAL);
  pPanel->SetSizer(pRow);
}

void GOVoicingDialog::CreateNoteControls(wxWindow *pParent) {
  wxBoxSizer *const pOctaves = new wxBoxSizer(wxVERTICAL);
  const std::vector<int> notes = m_Voicing.GetNotes();
  const unsigned nNotes = notes.size();
  unsigned noteI = 0;

  m_NoteControls.reserve(nNotes);
  while (noteI < nNotes) {
    const int octave = notes[noteI] / NOTES_PER_OCTAVE;
    const unsigned octaveI = m_OctaveControls.size();
    wxStaticBoxSizer *const pBox = new wxStaticBoxSizer(
      wxVERTICAL, pParent, wxString::Format(_("Octave %d"), octave - 1));
    wxWindow *const pBoxWindow = pBox->GetStaticBox();
    wxBoxSizer *const pNotesRow = new wxBoxSizer(wxHORIZONTAL);
    OctaveControls octaveControls;

    octaveControls.firstNoteControlI = m_NoteControls.size();
    for (; noteI < nNotes && notes[noteI] / NOTES_PER_OCTAVE == octave;
         noteI++) {
      const unsigned noteControlI = m_NoteControls.size();
      wxBoxSizer *const pColumn = new wxBoxSizer(wxVERTICAL);
      NoteControls noteControls;

      noteControls.midiNoteNumber = notes[noteI];
      noteControls.octaveI = octaveI;
      noteControls.shownOffset = std::nan("");
      noteControls.p_Slider = new wxSlider(
        pBoxWindow,
        wxID_ANY,
        0,
        -1,
        1,
        wxDefaultPosition,
        wxSize(-1, NOTE_SLIDER_HEIGHT),
        wxSL_VERTICAL | wxSL_INVERSE);
      noteControls.p_Spin = new wxSpinCtrlDouble(
        pBoxWindow,
        wxID_ANY,
        wxEmptyString,
        wxDefaultPosition,
        wxSize(SPIN_WIDTH, -1));
      noteControls.p_Slider->Bind(
        wxEVT_SLIDER, [this, noteControlI](wxCommandEvent &) {
          OnNoteChanged(
            noteControlI,
            slider_to_offset(
              m_NoteControls[noteControlI].p_Slider->GetValue(),
              PARAMETER_LIMITS[m_Parameter]),
            m_NoteControls[noteControlI].p_Slider);
        });
      noteControls.p_Spin->Bind(
        wxEVT_SPINCTRLDOUBLE, [this, noteControlI](wxSpinDoubleEvent &) {
          OnNoteChanged(
            noteControlI,
            m_NoteControls[noteControlI].p_Spin->GetValue(),
            m_NoteControls[noteControlI].p_Spin);
        });
      pColumn->Add(
        new wxStaticText(pBoxWindow, wxID_ANY, note_name(notes[noteI])),
        0,
        wxALIGN_CENTER_HORIZONTAL);
      pColumn->Add(noteControls.p_Slider, 1, wxALIGN_CENTER_HORIZONTAL);
      pColumn->Add(noteControls.p_Spin, 0, wxALIGN_CENTER_HORIZONTAL);
      pNotesRow->Add(pColumn, 0, wxALL, 2);
      m_NoteControls.push_back(noteControls);
    }
    octaveControls.nNoteControls
      = m_NoteControls.size() - octaveControls.firstNoteControlI;

    // QUICK: the left and the right end of a linear ramp over the octave
    wxBoxSizer *const pQuick = new wxBoxSizer(wxHORIZONTAL);

    octaveControls.p_LeftQuick = new wxSlider(
      pBoxWindow, wxID_ANY, 0, -1, 1, wxDefaultPosition, wxSize(120, -1));
    octaveControls.p_RightQuick = new wxSlider(
      pBoxWindow, wxID_ANY, 0, -1, 1, wxDefaultPosition, wxSize(120, -1));
    octaveControls.p_LeftQuick->Bind(
      wxEVT_SLIDER,
      [this, octaveI](wxCommandEvent &) { OnQuickChanged(octaveI); });
    octaveControls.p_RightQuick->Bind(
      wxEVT_SLIDER,
      [this, octaveI](wxCommandEvent &) { OnQuickChanged(octaveI); });
    pQuick->Add(
      new wxStaticText(pBoxWindow, wxID_ANY, _("QUICK")),
      0,
      wxALIGN_CENTER_VERTICAL | wxRIGHT,
      4);
    pQuick->Add(octaveControls.p_LeftQuick, 1);
    pQuick->Add(octaveControls.p_RightQuick, 1);
    pBox->Add(pNotesRow, 1, wxEXPAND);
    pBox->Add(pQuick, 0, wxEXPAND | wxTOP, 3);
    pOctaves->Add(pBox, 0, wxEXPAND | wxALL, 3);
    m_OctaveControls.push_back(octaveControls);
  }
  pParent->SetSizer(pOctaves);
}

void GOVoicingDialog::ApplyParameterLimits() {
  const ParameterLimits &limits = PARAMETER_LIMITS[m_Parameter];
  const int sliderMax = (int)(limits.sliderLimit * limits.sliderScale);
  const bool wasRefreshing = m_IsRefreshing;

  m_IsRefreshing = true;
  p_MasterSlider->SetRange(-sliderMax, sliderMax);
  p_MasterSpin->SetRange(-limits.spinLimit, limits.spinLimit);
  p_MasterSpin->SetIncrement(limits.spinStep);
  p_MasterSpin->SetDigits(limits.nDigits);
  for (NoteControls &noteControls : m_NoteControls) {
    // another parameter is shown now, so every control must be updated
    noteControls.shownOffset = std::nan("");
    noteControls.p_Slider->SetRange(-sliderMax, sliderMax);
    noteControls.p_Spin->SetRange(-limits.spinLimit, limits.spinLimit);
    noteControls.p_Spin->SetIncrement(limits.spinStep);
    noteControls.p_Spin->SetDigits(limits.nDigits);
  }
  for (OctaveControls &octaveControls : m_OctaveControls) {
    octaveControls.p_LeftQuick->SetRange(-sliderMax, sliderMax);
    octaveControls.p_RightQuick->SetRange(-sliderMax, sliderMax);
  }
  m_IsRefreshing = wasRefreshing;
}

void GOVoicingDialog::RefreshNote(
  unsigned noteControlI, const wxWindow *pSource) {
  NoteControls &noteControls = m_NoteControls[noteControlI];
  const double offset
    = m_Voicing.GetOffset(m_Parameter, noteControls.midiNoteNumber);

  // Writing a control is slow, so only the controls with a new value are
  // written, and never the one the user is moving
  if (offset != noteControls.shownOffset) {
    const int sliderValue
      = offset_to_slider(offset, PARAMETER_LIMITS[m_Parameter]);

    noteControls.shownOffset = offset;
    if (
      pSource != noteControls.p_Slider
      && noteControls.p_Slider->GetValue() != sliderValue)
      noteControls.p_Slider->SetValue(sliderValue);
    if (pSource != noteControls.p_Spin)
      noteControls.p_Spin->SetValue(offset);
  }
}

void GOVoicingDialog::RefreshQuick(unsigned octaveI) {
  const OctaveControls &octaveControls = m_OctaveControls[octaveI];
  const ParameterLimits &limits = PARAMETER_LIMITS[m_Parameter];
  const unsigned lastI
    = octaveControls.firstNoteControlI + octaveControls.nNoteControls - 1;
  const int leftValue = offset_to_slider(
    m_Voicing.GetOffset(
      m_Parameter,
      m_NoteControls[octaveControls.firstNoteControlI].midiNoteNumber),
    limits);
  const int rightValue = offset_to_slider(
    m_Voicing.GetOffset(m_Parameter, m_NoteControls[lastI].midiNoteNumber),
    limits);

  if (octaveControls.p_LeftQuick->GetValue() != leftValue)
    octaveControls.p_LeftQuick->SetValue(leftValue);
  if (octaveControls.p_RightQuick->GetValue() != rightValue)
    octaveControls.p_RightQuick->SetValue(rightValue);
}

GOVoicingDialog::~GOVoicingDialog() {
  if (m_HoldAutoSaveFunction)
    m_HoldAutoSaveFunction(false);
}

void GOVoicingDialog::RefreshAll() {
  m_IsRefreshing = true;
  for (unsigned nNoteControls = m_NoteControls.size(), noteControlI = 0;
       noteControlI < nNoteControls;
       noteControlI++)
    RefreshNote(noteControlI);
  for (unsigned nOctaves = m_OctaveControls.size(), octaveI = 0;
       octaveI < nOctaves;
       octaveI++)
    RefreshQuick(octaveI);
  p_ResetButton->Enable(m_Voicing.IsModified());
  m_IsRefreshing = false;
}

void GOVoicingDialog::OnParameterChanged() {
  m_Parameter = (GOPipeVoicing::Parameter)p_ParameterChoice->GetSelection();
  ApplyParameterLimits();
  p_MasterSlider->SetValue(0);
  p_MasterSpin->SetValue(0);
  RefreshAll();
}

void GOVoicingDialog::OnMasterChanged(double offset) {
  if (!m_IsRefreshing) {
    m_Voicing.SetAllOffsets(m_Parameter, (float)offset);
    RefreshAll();
    m_IsRefreshing = true;
    p_MasterSlider->SetValue(
      offset_to_slider(offset, PARAMETER_LIMITS[m_Parameter]));
    p_MasterSpin->SetValue(offset);
    m_IsRefreshing = false;
  }
}

void GOVoicingDialog::OnNoteChanged(
  unsigned noteControlI, double offset, const wxWindow *pSource) {
  if (!m_IsRefreshing) {
    const unsigned octaveI = m_NoteControls[noteControlI].octaveI;

    m_Voicing.SetOffset(
      m_Parameter, m_NoteControls[noteControlI].midiNoteNumber, (float)offset);
    m_IsRefreshing = true;
    RefreshNote(noteControlI, pSource);
    RefreshQuick(octaveI);
    p_ResetButton->Enable();
    m_IsRefreshing = false;
  }
}

void GOVoicingDialog::OnQuickChanged(unsigned octaveI) {
  if (!m_IsRefreshing) {
    const OctaveControls &octaveControls = m_OctaveControls[octaveI];
    const ParameterLimits &limits = PARAMETER_LIMITS[m_Parameter];
    const unsigned lastI
      = octaveControls.firstNoteControlI + octaveControls.nNoteControls - 1;

    m_Voicing.SetRamp(
      m_Parameter,
      m_NoteControls[octaveControls.firstNoteControlI].midiNoteNumber,
      m_NoteControls[lastI].midiNoteNumber,
      (float)slider_to_offset(octaveControls.p_LeftQuick->GetValue(), limits),
      (float)slider_to_offset(octaveControls.p_RightQuick->GetValue(), limits));
    m_IsRefreshing = true;
    for (unsigned noteControlI = octaveControls.firstNoteControlI;
         noteControlI <= lastI;
         noteControlI++)
      RefreshNote(noteControlI);
    p_ResetButton->Enable();
    m_IsRefreshing = false;
  }
}

void GOVoicingDialog::OnReset() {
  m_Voicing.Reset();
  RefreshAll();
}

void GOVoicingDialog::OnSave() {
  if (!m_SaveFunction || m_SaveFunction()) {
    // the values are saved, so a close must not ask again
    m_Voicing.Select({});
    Close();
  } else
    wxMessageBox(
      _("The settings could not be saved"),
      _("Voicing"),
      wxOK | wxICON_ERROR,
      this);
}

void GOVoicingDialog::OnCloseWindow(wxCloseEvent &event) {
  bool isClosing = true;

  if (event.CanVeto() && m_Voicing.IsModified()) {
    const int answer = wxMessageBox(
      _("Save the voicing changes? Choose No to restore the values from when "
        "this window was opened."),
      _("Voicing"),
      wxYES_NO | wxCANCEL | wxICON_QUESTION,
      this);

    if (answer == wxYES)
      isClosing = !m_SaveFunction || m_SaveFunction();
    else if (answer == wxNO)
      m_Voicing.Reset();
    else
      isClosing = false;
  }
  if (isClosing)
    Destroy();
  else
    event.Veto();
}
