/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qSlicerVolumeRenderingPropertiesWidget_h
#define __qSlicerVolumeRenderingPropertiesWidget_h

// Qt includes
#include <QWidget>

// CTK includes
#include <ctkVTKObject.h>

// Slicer includes
#include "qSlicerVolumeRenderingModuleWidgetsExport.h"

class qSlicerVolumeRenderingPropertiesWidgetPrivate;
class vtkMRMLNode;
class vtkMRMLVolumeNode;
class vtkMRMLVolumeRenderingDisplayNode;

class Q_SLICER_MODULE_VOLUMERENDERING_WIDGETS_EXPORT qSlicerVolumeRenderingPropertiesWidget : public QWidget
{
  Q_OBJECT
  QVTK_OBJECT
public:
  typedef QWidget Superclass;
  qSlicerVolumeRenderingPropertiesWidget(QWidget* parent = nullptr);
  ~qSlicerVolumeRenderingPropertiesWidget() override;

  vtkMRMLNode* mrmlNode() const;
  vtkMRMLVolumeRenderingDisplayNode* mrmlVolumeRenderingDisplayNode() const;
  vtkMRMLVolumeNode* mrmlVolumeNode() const;

public slots:
  void setMRMLVolumeRenderingDisplayNode(vtkMRMLVolumeRenderingDisplayNode* node);
  /// Utility slot to set the volume rendering display node.
  void setMRMLNode(vtkMRMLNode* node);

protected slots:
  virtual void updateWidgetFromMRML();
  virtual void updateWidgetFromMRMLVolumeNode();

protected:
  QScopedPointer<qSlicerVolumeRenderingPropertiesWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerVolumeRenderingPropertiesWidget);
  Q_DISABLE_COPY(qSlicerVolumeRenderingPropertiesWidget);
};

#endif
