/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations includes
#include "qSlicerSegmentationsModule.h"
#include "qSlicerSegmentationsModuleWidget.h"
#include "qSlicerSegmentationsReader.h"
#include "qSlicerSegmentationsSettingsPanel.h"
#include "qSlicerSubjectHierarchySegmentationsPlugin.h"
#include "qSlicerSubjectHierarchySegmentsPlugin.h"
#include "vtkSlicerSegmentationsModuleLogic.h"
#include "vtkMRMLSegmentationsDisplayableManager3D.h"
#include "vtkMRMLSegmentationsDisplayableManager2D.h"
#include "qSlicerSegmentationsNodeWriter.h"

// Segment editor effects includes
#include "qSlicerSegmentEditorEffectFactory.h"
#include "qSlicerSegmentEditorPaintEffect.h"
#include "qSlicerSegmentEditorScissorsEffect.h"
#include "qSlicerSegmentEditorEraseEffect.h"

// Slicer includes
#include <qSlicerIOManager.h>
#include <qSlicerNodeWriter.h>
#include <vtkMRMLThreeDViewDisplayableManagerFactory.h>
#include <vtkMRMLSliceViewDisplayableManagerFactory.h>
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include <vtkSlicerConfigure.h> // For Slicer_USE_PYTHONQT

// Subject Hierarchy includes
#include "qSlicerSubjectHierarchyPluginHandler.h"

// Terminologies includes
#include "vtkSlicerTerminologiesModuleLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// PythonQt includes
#ifdef Slicer_USE_PYTHONQT
# include "PythonQt.h"
#endif

// Qt includes
#include <QDebug>

// DisplayableManager initialization
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkSlicerSegmentationsModuleMRMLDisplayableManager)

//-----------------------------------------------------------------------------
class qSlicerSegmentationsModulePrivate
{
public:
  qSlicerSegmentationsModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerSegmentationsModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentationsModulePrivate::qSlicerSegmentationsModulePrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerSegmentationsModule methods

//-----------------------------------------------------------------------------
qSlicerSegmentationsModule::qSlicerSegmentationsModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSegmentationsModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentationsModule::~qSlicerSegmentationsModule() = default;

//-----------------------------------------------------------------------------
QString qSlicerSegmentationsModule::helpText() const
{
  QString help = tr("Segmentations module manages segmentations. Each segmentation can contain"
                    " multiple segments, which correspond to one structure or ROI. Each segment"
                    " can contain multiple data representations for the same structure, and the"
                    " module supports automatic conversion between these representations"
                    " as well as advanced display settings and import/export features.");
  help += this->defaultDocumentationLink();
  return help;
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentationsModule::acknowledgementText() const
{
  return tr("This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s "
            "ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).");
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentationsModule::categories() const
{
  return QStringList() << "";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentationsModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Adam Rankin (Robarts)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentationsModule::dependencies() const
{
  return QStringList() << "Terminologies";
}

//-----------------------------------------------------------------------------
QIcon qSlicerSegmentationsModule::icon() const
{
  return QIcon(":/Icons/Segmentations.png");
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModule::setMRMLScene(vtkMRMLScene* scene)
{
  // Connect scene node added event to make connections enabling per-segment subject hierarchy actions
  qvtkReconnect(this->mrmlScene(), scene, vtkMRMLScene::NodeAddedEvent, this, SLOT(onNodeAdded(vtkObject*, vtkObject*)));

  Superclass::setMRMLScene(scene);

  // Subject hierarchy is instantiated before Segmentations, so need to connect to existing the quasi-singleton subject hierarchy node
  vtkCollection* shNodeCollection = scene->GetNodesByClass("vtkMRMLSubjectHierarchyNode");
  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(shNodeCollection->GetItemAsObject(0));
  shNodeCollection->Delete();
  this->onNodeAdded(scene, subjectHierarchyNode);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModule::setup()
{
  this->Superclass::setup();

  vtkSlicerSegmentationsModuleLogic* segmentationsLogic = vtkSlicerSegmentationsModuleLogic::SafeDownCast(this->logic());

  // Register subject hierarchy plugins
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchySegmentationsPlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchySegmentsPlugin());

  // Register IOs
  qSlicerIOManager* ioManager = qSlicerApplication::application()->ioManager();
  ioManager->registerIO(new qSlicerSegmentationsNodeWriter(this));
  ioManager->registerIO(new qSlicerSegmentationsReader(segmentationsLogic, this));

  // Register settings panel
  if (qSlicerApplication::application())
  {
    qSlicerSegmentationsSettingsPanel* panel = new qSlicerSegmentationsSettingsPanel();
    qSlicerApplication::application()->settingsDialog()->addPanel(tr("Segmentations"), panel);
    panel->setSegmentationsLogic(segmentationsLogic);
  }

  // Use the displayable manager class to make sure the the containing library is loaded
  vtkSmartPointer<vtkMRMLSegmentationsDisplayableManager3D> dm3d = vtkSmartPointer<vtkMRMLSegmentationsDisplayableManager3D>::New();
  vtkSmartPointer<vtkMRMLSegmentationsDisplayableManager2D> dm2d = vtkSmartPointer<vtkMRMLSegmentationsDisplayableManager2D>::New();
  // Register displayable managers
  vtkMRMLThreeDViewDisplayableManagerFactory::GetInstance()->RegisterDisplayableManager("vtkMRMLSegmentationsDisplayableManager3D");
  vtkMRMLSliceViewDisplayableManagerFactory::GetInstance()->RegisterDisplayableManager("vtkMRMLSegmentationsDisplayableManager2D");

  // Register default segment editor effects
  // C++ effects
  qSlicerSegmentEditorEffectFactory::instance()->registerEffect(new qSlicerSegmentEditorPaintEffect());
  qSlicerSegmentEditorEffectFactory::instance()->registerEffect(new qSlicerSegmentEditorEraseEffect());
  qSlicerSegmentEditorEffectFactory::instance()->registerEffect(new qSlicerSegmentEditorScissorsEffect());
  // Python effects
  // (otherwise it would be the responsibility of the module that embeds the segment editor widget)
#ifdef Slicer_USE_PYTHONQT
  if (!qSlicerCoreApplication::testAttribute(qSlicerCoreApplication::AA_DisablePython))
  {
    PythonQt::init();
    PythonQtObjectPtr context = PythonQt::self()->getMainModule();
    context.evalScript(QString("import SegmentEditorEffects; SegmentEditorEffects.registerEffects()"));
  }
#endif
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerSegmentationsModule::createWidgetRepresentation()
{
  qSlicerSegmentationsModuleWidget* moduleWidget = new qSlicerSegmentationsModuleWidget();
  return moduleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerSegmentationsModule::createLogic()
{
  return vtkSlicerSegmentationsModuleLogic::New();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModule::onNodeAdded(vtkObject* sceneObject, vtkObject* nodeObject)
{
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  if (!scene)
  {
    return;
  }

  // Get segmentations subject hierarchy plugin
  qSlicerSubjectHierarchySegmentationsPlugin* segmentationsPlugin =
    qobject_cast<qSlicerSubjectHierarchySegmentationsPlugin*>(qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Segmentations"));
  if (!segmentationsPlugin)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access segmentations subject hierarchy plugin";
    return;
  }

  // Connect segment added and removed events to plugin to update subject hierarchy accordingly
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(nodeObject);
  if (segmentationNode)
  {
    qvtkConnect(segmentationNode, vtkSegmentation::SegmentAdded, segmentationsPlugin, SLOT(onSegmentAdded(vtkObject*, void*)));
    qvtkConnect(segmentationNode, vtkSegmentation::SegmentRemoved, segmentationsPlugin, SLOT(onSegmentRemoved(vtkObject*, void*)));
    qvtkConnect(segmentationNode, vtkSegmentation::SegmentModified, segmentationsPlugin, SLOT(onSegmentModified(vtkObject*, void*)));
    qvtkConnect(segmentationNode, vtkMRMLSegmentationNode::DisplayModifiedEvent, segmentationsPlugin, SLOT(onDisplayNodeModified(vtkObject*)));
    qvtkConnect(segmentationNode, vtkSegmentation::SegmentsOrderModified, segmentationsPlugin, SLOT(onSegmentsOrderModified(vtkObject*, void*)));
  }

  // Connect subject hierarchy modified event to handle renaming segments from subject hierarchy
  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(nodeObject);
  if (subjectHierarchyNode)
  {
    qvtkConnect(subjectHierarchyNode, vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemModifiedEvent, segmentationsPlugin, SLOT(onSubjectHierarchyItemModified(vtkObject*, void*)));
    qvtkConnect(subjectHierarchyNode,
                vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemAboutToBeRemovedEvent,
                segmentationsPlugin,
                SLOT(onSubjectHierarchyItemAboutToBeRemoved(vtkObject*, void*)));
    qvtkConnect(subjectHierarchyNode,
                vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemChildrenReorderedEvent,
                segmentationsPlugin,
                SLOT(onSubjectHierarchyItemChildrenReordered(vtkObject*, void*)));
  }
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentationsModule::associatedNodeTypes() const
{
  return QStringList() << "vtkMRMLSegmentationNode";
}
