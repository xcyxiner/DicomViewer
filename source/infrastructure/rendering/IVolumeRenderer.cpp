
#include "IVolumeRenderer.h"

IVolumeRenderer::IVolumeRenderer() = default;

void IVolumeRenderer::setRenderTarget(vtkSmartPointer<vtkRenderWindow>) {}

void IVolumeRenderer::setVolume(vtkSmartPointer<vtkImageData>) {}

void IVolumeRenderer::updateWindowLevel(double, double) {}

void IVolumeRenderer::reset() {}

void IVolumeRenderer::render() {}
