#include "IMprRenderer.h"

IMprRenderer::IMprRenderer() {}

void IMprRenderer::setRenderTarget(vtkSmartPointer<vtkRenderWindow> window,
                                   int plane)
{
  (void)window;
  (void)plane;
}

void IMprRenderer::setVolume(vtkSmartPointer<vtkImageData> volume)
{
  (void)volume;
}

void IMprRenderer::updateWindowLevel(double windowWidth, double windowCenter)
{
  (void)windowWidth;
  (void)windowCenter;
}

void IMprRenderer::reset() {}

void IMprRenderer::render() {}

void IMprRenderer::setSlicePosition(int plane, double position)
{
  (void)plane;
  (void)position;
}

void IMprRenderer::setWindowLevelCallback(
    std::function<void(double, double)> callback)
{
  (void)callback;
}

void IMprRenderer::setSliceIndexCallback(std::function<void(double)> callback)
{
  (void)callback;
}
