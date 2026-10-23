#ifndef GOTESTDRAWSTOP_H
#define GOTESTDRAWSTOP_H

#include "GOTest.h"

class GOTestDrawStop {
private:
  static std::string CLASS_NAME;

  class TestFunctions : public GOCommonControllerTest {
  private:
    static std::string TEST_NAME;

    /*
     * AND, OR and XOR functions of a switch controlled by other switches.
     */
    void TestLogicalFunctions();

  public:
    virtual void run() override;
    std::string GetName() override { return TEST_NAME; }
  };

  class TestInternalState : public GOCommonControllerTest {
  private:
    static std::string TEST_NAME;

    /*
     * Engaging and disengaging a switch manually, by combinations and by
     * crescendos.
     */
    void TestEngagedState();

  public:
    virtual void run() override;
    std::string GetName() override { return TEST_NAME; }
  };

  class TestSavedState : public GOCommonControllerTest {
  private:
    static std::string TEST_NAME;

    /*
     * No save writes the current state of a switch.
     */
    void TestSaveNeverStoresLiveState();

    /*
     * The default engaged state moves the switch, is saved and is applied by
     * General Cancel.
     */
    void TestDefaultEngaged();

  public:
    virtual void run() override;
    std::string GetName() override { return TEST_NAME; }
  };

  TestFunctions testFunctions;
  TestInternalState testInternalState;
  TestSavedState testSavedState;
};

#endif /* GOTESTDRAWSTOP_H */
