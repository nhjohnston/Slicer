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

#ifndef __qMRMLModelDisplayNodeWidgetPlugin_h
#define __qMRMLModelDisplayNodeWidgetPlugin_h

#include "qSlicerModelsWidgetsAbstractPlugin.h"

class Q_SLICER_QTMODULES_MODELS_WIDGETS_PLUGINS_EXPORT qMRMLModelDisplayNodeWidgetPlugin
  : public QObject
  , public qSlicerModelsWidgetsAbstractPlugin
{
  Q_OBJECT

public:
  qMRMLModelDisplayNodeWidgetPlugin(QObject* newParent = nullptr);

  QWidget* createWidget(QWidget* newParent) override;
  QString domXml() const override;
  QIcon icon() const override;
  QString includeFile() const override;
  bool isContainer() const override;
  QString name() const override;
};

#endif
