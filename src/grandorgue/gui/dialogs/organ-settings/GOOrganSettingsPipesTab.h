/*
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2026 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#ifndef GOORGANSETTINGSPIPESTAB_H
#define GOORGANSETTINGSPIPESTAB_H

#include <functional>
#include <utility>
#include <vector>

#include <wx/event.h>
#include <wx/panel.h>

#include "GOOrganSettingsTab.h"

class wxButton;
class wxCheckBox;
class wxChoice;
class wxComboBox;
class wxSpinButton;
class wxStaticText;
class wxTextCtrl;
class wxTreeCtrl;
class wxTreeEvent;
class wxTreeItemId;

class GOConfig;
class GOOrganModel;
class GOPipeConfigNode;

class GOOrganSettingsPipesTab : public GOOrganSettingsTab {
private:
  class TreeItemData;

  GOOrganModel &r_OrganModel;
  GOConfig &r_config;
  GOPipeConfigNode &r_RootNode;

  std::function<bool()> m_SaveFunction;
  std::function<void(bool)> m_HoldAutoSaveFunction;

  wxTreeCtrl *m_Tree;
  wxButton *m_VoicingButton;
  wxStaticText *m_MemoryDisplay;
  wxStaticText *m_BitDisplay;
  wxTextCtrl *m_Delay;
  wxSpinButton *m_DelaySpin;
  wxTextCtrl *m_ReleaseLength;
  wxSpinButton *m_ReleaseLengthSpin;
  wxComboBox *m_AudioGroup;
  wxString m_LastAudioGroup;
  wxCheckBox *m_IgnorePitch;
  bool m_LastIgnorePitch;
  wxChoice *m_BitsPerSample;
  int m_LastBitsPerSample;
  wxChoice *m_Compress;
  int m_LastCompress;
  wxChoice *m_Channels;
  int m_LastChannels;
  wxChoice *m_LoopLoad;
  int m_LastLoopLoad;
  wxChoice *m_AttackLoad;
  int m_LastAttackLoad;
  wxChoice *m_ReleaseLoad;
  int m_LastReleaseLoad;

  TreeItemData *p_LastTreeItemData;
  unsigned m_LoadChangeCnt;

  wxTreeItemId FillTree(wxTreeItemId parent, GOPipeConfigNode &config);
  bool TransferDataToWindow() override;

  void SetEmpty(wxChoice *choice);
  void RemoveEmpty(wxChoice *choice);

  void Load(bool isForce);

  /**
   * Walks item and every descendant exactly once, recording each visited
   * node's round-robin-assigned target group. Recording into outAssignments
   * instead of mutating the node immediately lets DistributeAudio() run the
   * routing pre-scan and the actual mutation off the same recorded list,
   * rather than two tree walks that would need to land on identical
   * assignments to stay consistent.
   * @param audio_group the audio group names being distributed round-robin
   * @param pos index into audio_group for the next visited node; advanced by
   *   this call
   * @param item the subtree root to walk
   * @param outAssignments (node, target group name) pairs are appended here,
   *   in walk order
   */
  void CollectAudioGroupAssignments(
    const std::vector<wxString> &audio_group,
    unsigned &pos,
    wxTreeItemId item,
    std::vector<std::pair<TreeItemData *, wxString>> &outAssignments);

  void OnTreeChanging(wxTreeEvent &e);
  void OnTreeChanged(wxTreeEvent &e);
  void OnTreeUpdated(wxCommandEvent &e) { Load(false); }
  void OnVoicingClicked(wxCommandEvent &e);
  void OnDelayChanged(wxCommandEvent &e);
  void OnDelaySpinChanged(wxSpinEvent &e);
  void OnReleaseLengthChanged(wxCommandEvent &e);
  void OnReleaseLengthSpinChanged(wxSpinEvent &e);
  void OnAudioGroupChanged(wxCommandEvent &e) { NotifyModified(); }
  void OnIgnorePitchChanged(wxCommandEvent &e) { NotifyModified(); }
  void OnBitsPerSampleChanged(wxCommandEvent &e);
  void OnCompressChanged(wxCommandEvent &e);
  void OnChannelsChanged(wxCommandEvent &e);
  void OnLoopLoadChanged(wxCommandEvent &e);
  void OnAttackLoadChanged(wxCommandEvent &e);
  void OnReleaseLoadChanged(wxCommandEvent &e);

public:
  /**
   * @param saveFunction writes the settings file. It is used by the voicing
   *   window. Returns true on success
   * @param holdAutoSaveFunction holds (true) or releases (false) the autosave.
   *   It is used by the voicing window
   */
  GOOrganSettingsPipesTab(
    GOOrganModel &organModel,
    GOOrganSettingsDialogBase *pDlg,
    const std::function<bool()> &saveFunction,
    const std::function<void(bool)> &holdAutoSaveFunction);

  void DistributeAudio() override;
  void ResetToDefault() override;
  void DiscardChanges() override;
  void ApplyChanges() override;

  DECLARE_EVENT_TABLE()
};

#endif /* GOORGANSETTINGSPIPESTAB_H */
