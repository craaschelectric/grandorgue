/*
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2026 GrandOrgue contributors (see AUTHORS)
 * License GPL-2.0 or later
 * (https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
 */

#include "GOSoundToneBalanceFilter.h"

#include <cmath>

#include "sound/providers/GOSoundProvider.h"

void GOSoundToneBalanceFilter::Init(int8_t value) {
  m_Value = value;
  GOSoundOnePoleFilter::Type type;
  double hz = 0;

  if (value == 0)
    type = GOSoundOnePoleFilter::Type::TYPE_NONE;
  else if (value < 0) {
    type = GOSoundOnePoleFilter::Type::TYPE_LPF;
    hz = 16000 * pow(20.0 / 16000.0, abs(value) / 99.0);
  } else {
    type = GOSoundOnePoleFilter::Type::TYPE_HPF;
    hz = 20 * pow(800.0, value / 99.0);
  }

  GOSoundOnePoleFilter::Coeffs newCoeffs;

  GOSoundOnePoleFilter::computeCoeffs(type, hz, 0, m_samplerate, newCoeffs);
  m_coeffs.Publish(newCoeffs);
}

void GOSoundToneBalanceFilter::SetSamplerate(unsigned samplerate) {
  m_samplerate = samplerate;
  Init(m_Value);
}

void GOSoundToneBalanceFilter::State::Init() {
  m_coeffs = CoeffsVersioned::Snapshot();
  for (int i = 0; i < 2; i++)
    m_state[i] = 0;
}

void GOSoundToneBalanceFilter::State::EnsureCoeffs(
  const GOSoundProvider &provider) {
  if (
    provider.GetToneFilterCoeffs().RenewSnapshot(m_coeffs)
    && !m_coeffs.Get().isActive)
    for (int i = 0; i < 2; i++)
      m_state[i] = 0;
}
