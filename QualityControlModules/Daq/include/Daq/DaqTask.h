///
/// \file   DaqTask.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_EXAMPLE_EXAMPLETASK_H
#define QC_MODULE_EXAMPLE_EXAMPLETASK_H

#include "QualityControl/TaskInterface.h"

class TH1F;

using namespace AliceO2::QualityControl::Core;

namespace AliceO2 {
namespace QualityControlModules {
namespace Daq {

/// \brief Example Quality Control Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
class DaqTask /*final*/: public TaskInterface // todo add back the "final" when doxygen is fixed
{
  public:
    /// \brief Constructor
    DaqTask();
    /// Destructor
    ~DaqTask() override;

    // Definition of the methods for the template method pattern
    void initialize() override;
    void startOfActivity(Activity &activity) override;
    void startOfCycle() override;
    void monitorDataBlock(DataSetReference block) override;
    void endOfCycle() override;
    void endOfActivity(Activity &activity) override;
    void reset() override;

  private:

    TH1F *mPayloadSize, *mIds;
    TH1F *mNumberSubblocks;
    TH1F *mSubPayloadSize;
};

}
}
}

#endif //QC_MODULE_EXAMPLE_EXAMPLETASK_H
