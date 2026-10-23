/*
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2026 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#ifndef GOCONFIGWRITER_H
#define GOCONFIGWRITER_H

#include <wx/string.h>

class GOConfigEnum;
class GOConfigFileWriter;

class GOConfigWriter {
private:
  GOConfigFileWriter &m_ConfigFile;
  bool m_Prefix;
  bool m_IsLiveStateExcluded = false;

public:
  GOConfigWriter(GOConfigFileWriter &cfg, bool prefix);

  /**
   * Whether the objects that save a live performance state (the enclosure
   * positions and the virtual coupler throughs) must write their last loaded
   * or explicitly saved value instead of the live one. It is set by the
   * autosave. The stops never save their current state at all.
   */
  bool IsLiveStateExcluded() const { return m_IsLiveStateExcluded; }
  void SetLiveStateExcluded(bool isExcluded) {
    m_IsLiveStateExcluded = isExcluded;
  }

  void WriteString(wxString group, wxString key, wxString value);
  void WriteInteger(wxString group, wxString key, int value);
  void WriteEnum(
    const wxString &group,
    const wxString &key,
    const GOConfigEnum &confEnum,
    int value);
  void WriteFloat(wxString group, wxString key, float value);
  /**
   * Saves 0 as N, 1 as Y. -1 is not saved at all
   */
  void WriteBooleanTriple(
    const wxString &group, const wxString &key, int value);
  void WriteBoolean(wxString group, wxString key, bool value) {
    WriteBooleanTriple(group, key, (int)value);
  }
};

#endif
