/*
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2026 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#ifndef GOPIPEVOICING_H
#define GOPIPEVOICING_H

#include <map>
#include <vector>

class GOPipeConfig;
class GOPipeConfigNode;

/**
 * Live per-pipe voicing of the pipes below some selected config nodes (ranks,
 * windchests or the whole organ).
 *
 * Every change is written to the pipes at once, so a playing note follows it
 * without any Apply. The values of the pipes at the moment of Select() are
 * remembered, so that Reset() can restore them.
 *
 * Offsets are the change relative to the ODF default of every pipe: an
 * untouched pipe has the offset 0 for every parameter. The ODF values
 * themselves are never modified.
 */
class GOPipeVoicing {
public:
  enum Parameter {
    GAIN,                   // dB
    MANUAL_TUNING,          // cent
    AUTO_TUNING_CORRECTION, // cent
    TONE_BALANCE            // -100..100
  };

private:
  // The pipe config values as they were at Select()
  struct PipeEntry {
    GOPipeConfigNode *pNode;
    float gain;
    float manualTuning;
    float autoTuningCorrection;
    int toneBalance;
  };

  std::vector<PipeEntry> m_Pipes;
  // the midi note number -> the indices in m_Pipes. The first one is displayed
  std::map<int, std::vector<unsigned>> m_PipeIndicesByNote;

  void CollectPipes(
    GOPipeConfigNode &node, std::vector<GOPipeConfigNode *> &outPNodes) const;

  static float getOffsetOf(const GOPipeConfig &config, Parameter parameter);
  static void setOffsetOf(
    GOPipeConfig &config, Parameter parameter, float offset);

  /**
   * Adds the user gain, the user amplitude and the tone balance of node and of
   * all its ancestors into the pipes below node and resets the values of
   * the non-pipe nodes to the ODF defaults
   * @param node
   * @param inheritedGainDb the gain of the ancestors
   * @param inheritedToneBalance the effective tone balance of the ancestors
   */
  static void foldNode(
    GOPipeConfigNode &node, float inheritedGainDb, int inheritedToneBalance);

public:
  /**
   * Selects the pipes below the nodes and remembers their current values. A
   * pipe reachable by several nodes is taken once.
   */
  void Select(const std::vector<GOPipeConfigNode *> &mpNodes);

  bool IsEmpty() const { return m_Pipes.empty(); }

  /** @return the midi note numbers of the selected pipes in ascending order */
  std::vector<int> GetNotes() const;

  /**
   * @return the offset of the first selected pipe with the midi note, or 0 if
   *   there is no such pipe
   */
  float GetOffset(Parameter parameter, int midiNoteNumber) const;

  /** Sets the offset of every selected pipe with the midi note */
  void SetOffset(Parameter parameter, int midiNoteNumber, float offset);

  /** Sets the same offset to all selected pipes */
  void SetAllOffsets(Parameter parameter, float offset);

  /**
   * Sets offsets growing linearly from firstOffset at firstNote to lastOffset
   * at lastNote (both inclusive) to the selected pipes in this note range
   */
  void SetRamp(
    Parameter parameter,
    int firstNote,
    int lastNote,
    float firstOffset,
    float lastOffset);

  /** @return true if any selected pipe differs from the state at Select() */
  bool IsModified() const;

  /** Restores the values that the selected pipes had at Select() */
  void Reset();

  /**
   * Moves the user gain, the user amplitude and the tone balance set on a
   * windchest, a rank or the organ root into the pipes below it, so that the
   * pipes carry all the voicing. The sound is unchanged. The tunings and the
   * ODF values are not touched.
   */
  static void foldIntoPipes(GOPipeConfigNode &root);
};

#endif /* GOPIPEVOICING_H */
