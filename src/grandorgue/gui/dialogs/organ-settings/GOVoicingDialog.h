/*
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2026 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#ifndef GOVOICINGDIALOG_H
#define GOVOICINGDIALOG_H

#include <functional>
#include <vector>

#include <wx/dialog.h>

#include "model/pipe-config/GOPipeVoicing.h"

class wxButton;
class wxChoice;
class wxCloseEvent;
class wxPanel;
class wxSlider;
class wxSpinCtrlDouble;
class wxStaticText;
class wxWindow;

class GOPipeConfigNode;

/**
 * A modeless voicing window for the pipes below the selected ranks,
 * windchests or organ. Every control change is written to the pipes at once
 * (see GOPipeVoicing), so held notes are heard changing while a slider moves.
 * There is no Apply: Reset restores the values at the opening of the window,
 * Save writes the settings file and closes the window.
 */
class GOVoicingDialog : public wxDialog {
private:
  // The controls of one pipe note
  struct NoteControls {
    int midiNoteNumber;
    unsigned octaveI;
    wxSlider *p_Slider;
    wxSpinCtrlDouble *p_Spin;
    // the offset the two controls show now. NaN forces an update
    double shownOffset;
  };

  // The quick ramp controls of the octave that is made of
  // m_NoteControls[firstNoteControlI .. firstNoteControlI + nNoteControls)
  struct OctaveControls {
    unsigned firstNoteControlI;
    unsigned nNoteControls;
    wxSlider *p_LeftQuick;
    wxSlider *p_RightQuick;
  };

  GOPipeVoicing m_Voicing;
  std::function<bool()> m_SaveFunction;
  std::function<void(bool)> m_HoldAutoSaveFunction;
  GOPipeVoicing::Parameter m_Parameter;
  // true while the controls are being updated from the model, so that they
  // do not write back
  bool m_IsRefreshing;

  wxChoice *p_ParameterChoice;
  wxSlider *p_MasterSlider;
  wxSpinCtrlDouble *p_MasterSpin;
  wxButton *p_ResetButton;
  wxButton *p_SaveButton;
  std::vector<NoteControls> m_NoteControls;
  std::vector<OctaveControls> m_OctaveControls;

  void CreateTopRow(wxPanel *pPanel);
  void CreateNoteControls(wxWindow *pParent);
  void CreateButtons();

  // Sets the ranges of all sliders and spin controls for m_Parameter
  void ApplyParameterLimits();
  // Reads all the controls from the pipes
  void RefreshAll();
  // Updates the controls of the note that show another value than the pipes
  // have. The control pSource is not touched because the user is using it
  void RefreshNote(unsigned noteControlI, const wxWindow *pSource = nullptr);
  void RefreshQuick(unsigned octaveI);

  void OnParameterChanged();
  void OnMasterChanged(double offset);
  void OnNoteChanged(
    unsigned noteControlI, double offset, const wxWindow *pSource);
  void OnQuickChanged(unsigned octaveI);
  void OnReset();
  void OnSave();
  void OnCloseWindow(wxCloseEvent &event);

public:
  /**
   * @param pParent the parent window. The dialog is destroyed with it
   * @param mpNodes the selected config nodes. The first one is displayed
   * @param saveFunction writes the settings file; returns true on success
   * @param holdAutoSaveFunction holds the autosave of the settings file while
   *   the window is open, so that the voicing changes reach the file only when
   *   the user saves them. It is called with true when the window opens and
   *   with false when it is destroyed
   */
  GOVoicingDialog(
    wxWindow *pParent,
    const std::vector<GOPipeConfigNode *> &mpNodes,
    const std::function<bool()> &saveFunction,
    const std::function<void(bool)> &holdAutoSaveFunction);
  ~GOVoicingDialog();
};

#endif /* GOVOICINGDIALOG_H */
