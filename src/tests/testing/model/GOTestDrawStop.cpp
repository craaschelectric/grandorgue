#include "GOTestDrawStop.h"

#include "config/GOConfigFileWriter.h"
#include "config/GOConfigWriter.h"
#include "model/GOSwitch.h"

#include "GOBuffer.h"
#include "GOSaveableObject.h"

#include "GOTestScope.h"

std::string GOTestDrawStop::CLASS_NAME = "GOTestDrawStop";
std::string GOTestDrawStop::TestFunctions::TEST_NAME
  = CLASS_NAME + "::Functions";

void GOTestDrawStop::TestFunctions::run() {
  GO_RUN_TEST(TestLogicalFunctions())
}

void GOTestDrawStop::TestFunctions::TestLogicalFunctions() {
  GOSwitch sw1(*controller);
  GOSwitch sw2(*controller);
  GOSwitch sw3(*controller);
  GOSwitch sw12(*controller);
  GOSwitch sw13(*controller);
  GOSwitch sw23(*controller);

  sw12.SetFunctionType(GODrawstop::FUNCTION_AND);
  sw13.SetFunctionType(GODrawstop::FUNCTION_OR);
  sw23.SetFunctionType(GODrawstop::FUNCTION_XOR);
  sw12.AddControllingDrawstop(&sw1, 1, "sw1");
  sw12.AddControllingDrawstop(&sw2, 2, "sw1");
  sw13.AddControllingDrawstop(&sw1, 1, "sw2");
  sw13.AddControllingDrawstop(&sw3, 2, "sw2");
  sw23.AddControllingDrawstop(&sw2, 1, "sw3");
  sw23.AddControllingDrawstop(&sw3, 2, "sw3");

  GOAssert(!sw12.IsEngaged(), "false AND false");
  GOAssert(!sw13.IsEngaged(), "false OR false");
  GOAssert(!sw23.IsEngaged(), "false XOR false");

  sw1.SetButtonState(true);
  GOAssert(!sw12.IsEngaged(), "true AND false");
  GOAssert(sw13.IsEngaged(), "true OR false");
  GOAssert(!sw23.IsEngaged(), "false XOR false");

  sw1.SetButtonState(false);
  sw2.SetButtonState(true);
  GOAssert(!sw12.IsEngaged(), "false AND true");
  GOAssert(!sw13.IsEngaged(), "false OR false");
  GOAssert(sw23.IsEngaged(), "true XOR false");

  sw1.SetButtonState(true);
  GOAssert(sw12.IsEngaged(), "true AND true");
  GOAssert(sw13.IsEngaged(), "true OR false");
  GOAssert(sw23.IsEngaged(), "true XOR false");

  sw1.SetButtonState(false);
  sw2.SetButtonState(false);
  sw3.SetButtonState(true);
  GOAssert(!sw12.IsEngaged(), "false AND false");
  GOAssert(sw13.IsEngaged(), "false OR true");
  GOAssert(sw23.IsEngaged(), "false XOR true");

  sw1.SetButtonState(true);
  GOAssert(!sw12.IsEngaged(), "true AND false");
  GOAssert(sw13.IsEngaged(), "true OR true");
  GOAssert(sw23.IsEngaged(), "false XOR true");

  sw1.SetButtonState(false);
  sw2.SetButtonState(true);
  GOAssert(!sw12.IsEngaged(), "false AND true");
  GOAssert(sw13.IsEngaged(), "false OR true");
  GOAssert(!sw23.IsEngaged(), "true XOR true");

  sw1.SetButtonState(true);
  GOAssert(sw12.IsEngaged(), "true AND true");
  GOAssert(sw13.IsEngaged(), "true OR true");
  GOAssert(!sw23.IsEngaged(), "true XOR true");

  sw23.ClearControllingDrawstops();
  sw13.ClearControllingDrawstops();
  sw12.ClearControllingDrawstops();
}

std::string GOTestDrawStop::TestInternalState::TEST_NAME
  = CLASS_NAME + "::InternalState";

static wxString WX_EMPTY = wxEmptyString;
static wxString WX_CRESCENDO_A = wxT("crescendo-a");

void GOTestDrawStop::TestInternalState::run() {
  GO_RUN_TEST(TestEngagedState())
}

void GOTestDrawStop::TestInternalState::TestEngagedState() {
  GOSwitch sw(*controller);

  GOAssert(!sw.IsEngaged(), "After init");

  sw.SetButtonState(true);
  GOAssert(sw.IsEngaged(), "After engaging");

  sw.SetButtonState(false);
  GOAssert(!sw.IsEngaged(), "After disengaging");

  sw.SetCombinationState(true, WX_EMPTY);
  GOAssert(sw.IsEngaged(), "After engaging from a combination");

  sw.SetCombinationState(false, WX_EMPTY);
  GOAssert(!sw.IsEngaged(), "After disengaging from a combination");

  sw.SetCombinationState(true, WX_CRESCENDO_A);
  GOAssert(sw.IsEngaged(), "After engaging from a crescendo");

  sw.SetCombinationState(false, WX_CRESCENDO_A);
  GOAssert(!sw.IsEngaged(), "After disengaging from a crescendo");

  sw.SetButtonState(true);
  GOAssert(sw.IsEngaged(), "After engaging");

  sw.SetCombinationState(false, WX_EMPTY);
  GOAssert(
    !sw.IsEngaged(), "After engaging and disengaging from a combination");

  sw.SetCombinationState(true, WX_CRESCENDO_A);
  GOAssert(sw.IsEngaged(), "After engaging from a crescendo");

  sw.SetButtonState(true);
  GOAssert(
    sw.IsEngaged(),
    "After engaging from a crescendo and an attempt to engage manually");

  sw.SetCombinationState(false, WX_CRESCENDO_A);
  GOAssert(
    !sw.IsEngaged(),
    "After an attempt to engage manually and disengaging from a crescendo");

  sw.SetButtonState(true);
  GOAssert(sw.IsEngaged(), "After engaging manually");

  sw.SetCombinationState(true, WX_CRESCENDO_A);
  GOAssert(sw.IsEngaged(), "After engaging manually and from a crescendo");

  sw.SetButtonState(false);
  GOAssert(
    sw.IsEngaged(),
    "After engaging from a crescendo and an attempt to disengage manually");

  sw.SetCombinationState(false, WX_CRESCENDO_A);
  GOAssert(
    sw.IsEngaged(),
    "After disengaging from a crescendo and an attempt to disengage manually");
}

std::string GOTestDrawStop::TestSavedState::TEST_NAME
  = CLASS_NAME + "::SavedState";

void GOTestDrawStop::TestSavedState::run() {
  GO_RUN_TEST(TestAutosaveKeepsSavedState())
}

/**
 * Saves the switch and returns the written DefaultToEngaged value.
 */
static wxString save_engaged_value(GOSwitch &sw, bool isLiveStateExcluded) {
  GOConfigFileWriter cfgFile;
  GOConfigWriter cfg(cfgFile, false);
  GOBuffer<uint8_t> content;

  cfg.SetLiveStateExcluded(isLiveStateExcluded);
  // Save() is protected in the drawstop, but public as a saveable object
  static_cast<GOSaveableObject &>(sw).Save(cfg);
  cfgFile.GetFileContent(content, false);

  const wxString text
    = wxString::FromUTF8((const char *)content.get(), content.GetSize());
  const int keyPos = text.Find(wxT("DefaultToEngaged="));

  return keyPos == wxNOT_FOUND ? wxString(wxT("absent"))
                               : text.Mid(keyPos + 17, 1);
}

void GOTestDrawStop::TestSavedState::TestAutosaveKeepsSavedState() {
  GOSwitch sw(*controller);

  sw.SetGroup(wxT("Switch001"));
  GOAssert(
    save_engaged_value(sw, true) == wxT("N"),
    "Autosave of a disengaged switch");

  sw.SetButtonState(true);
  GOAssert(
    save_engaged_value(sw, true) == wxT("N"),
    "Autosave must not store the engaged state");
  GOAssert(
    save_engaged_value(sw, false) == wxT("Y"),
    "An explicit save stores the engaged state");
  GOAssert(
    save_engaged_value(sw, true) == wxT("Y"),
    "Autosave keeps the state of the last explicit save");

  sw.SetButtonState(false);
  GOAssert(
    save_engaged_value(sw, true) == wxT("Y"),
    "Autosave must not store the disengaged state");
  GOAssert(
    save_engaged_value(sw, false) == wxT("N"),
    "An explicit save stores the disengaged state");
}
