

#include "LoadSeriesUseCase.h"

#include <vtkFieldData.h>
#include <vtkImageData.h>
#include <vtkStringArray.h>

LoadSeriesUseCase::LoadSeriesUseCase(IDicomReader& dicomReader,
                                     ITaskQueue& taskQueue,
                                     IFrameCache& frameCache)
    : m_dicomReader(dicomReader)
    , m_taskQueue(taskQueue)
    , m_frameCache(frameCache)
{
}

std::future<std::shared_ptr<StackDisplaySet>>
LoadSeriesUseCase::loadSeriesAsync(const std::string& path)
{
  return m_taskQueue.submitWithResult(
      [this, path]() -> std::shared_ptr<StackDisplaySet>
      {
        m_dicomReader.open(path);
        std::vector<std::string> frameUids;
        auto frame = m_dicomReader.readFrame(0);
        std::visit(
            [&](auto&& frameVariant)
            {
              using T = std::decay_t<decltype(frameVariant)>;
              std::string sopUid;
              int frameIndex = 0;
              if constexpr (std::is_same_v<T, std::shared_ptr<Frame>>) {
                if (frameVariant) {
                  sopUid = frameVariant->getSopInstanceUid();
                  frameIndex = frameVariant->getFrameIndex();
                }
              }
              if constexpr (std::is_same_v<T, vtkSmartPointer<vtkImageData>>) {
                if (frameVariant) {
                  vtkStringArray* uidArray = vtkStringArray::SafeDownCast(
                      frameVariant->GetFieldData()->GetAbstractArray(
                          "SOPInstanceUIDs"));
                  if (uidArray && uidArray->GetNumberOfValues() > 0) {
                    sopUid = uidArray->GetValue(0);
                  }
                }
              }
              if (!sopUid.empty()) {
                frameUids.push_back(sopUid);
                m_frameCache.put(sopUid, frameIndex, frameVariant);
              }
            },
            frame);
        auto displaySet = std::make_shared<StackDisplaySet>();
        displaySet->setFrameUids(frameUids);
        displaySet->setCurrentIndex(0);
        return displaySet;
      });
}
