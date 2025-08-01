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
#include "qSlicerSubjectHierarchySegmentsPlugin.h"
#include "qSlicerSubjectHierarchySegmentationsPlugin.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyPluginHandler.h"
#include "qSlicerSubjectHierarchyDefaultPlugin.h"

// Terminologies includes
#include "qSlicerTerminologyItemDelegate.h"
#include "vtkSlicerTerminologiesModuleLogic.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include <vtkMRMLSliceNode.h>
#include "vtkMRMLSubjectHierarchyConstants.h"
#include "vtkMRMLSubjectHierarchyNode.h"

// Qt includes
#include <QAction>
#include <QDebug>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QWidgetAction>

// Slicer includes
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"
#include "qMRMLSliceWidget.h"

// MRML widgets includes
#include "qMRMLNodeComboBox.h"

// CTK includes
#include "ctkDoubleSlider.h"

//-----------------------------------------------------------------------------
class qSlicerSubjectHierarchySegmentsPluginPrivate : public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSubjectHierarchySegmentsPlugin);

protected:
  qSlicerSubjectHierarchySegmentsPlugin* const q_ptr;

public:
  qSlicerSubjectHierarchySegmentsPluginPrivate(qSlicerSubjectHierarchySegmentsPlugin& object);
  ~qSlicerSubjectHierarchySegmentsPluginPrivate() override;
  void init();

public:
  QIcon SegmentIcon;

  QAction* ShowOnlyCurrentSegmentAction{ nullptr };
  QAction* ShowAllSegmentsAction{ nullptr };
  QAction* JumpSlicesAction{ nullptr };
  QAction* CloneSegmentAction{ nullptr };
  QAction* OpacityAction{ nullptr };
  QMenu* OpacityMenu{ nullptr };
  ctkDoubleSlider* OpacitySlider{ nullptr };
};

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchySegmentsPluginPrivate methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentsPluginPrivate::qSlicerSubjectHierarchySegmentsPluginPrivate(qSlicerSubjectHierarchySegmentsPlugin& object)
  : q_ptr(&object)
  , SegmentIcon(QIcon(":Icons/Segment.png"))
{
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPluginPrivate::init()
{
  Q_Q(qSlicerSubjectHierarchySegmentsPlugin);

  // Show only current segment action
  this->ShowOnlyCurrentSegmentAction = new QAction(qSlicerSubjectHierarchySegmentsPlugin::tr("Show only this segment"), q);
  QObject::connect(this->ShowOnlyCurrentSegmentAction, SIGNAL(triggered()), q, SLOT(showOnlyCurrentSegment()));

  // Show all segments action
  this->ShowAllSegmentsAction = new QAction(qSlicerSubjectHierarchySegmentsPlugin::tr("Show all segments"), q);
  QObject::connect(this->ShowAllSegmentsAction, SIGNAL(triggered()), q, SLOT(showAllSegments()));

  // Jump slices action
  this->JumpSlicesAction = new QAction(qSlicerSubjectHierarchySegmentsPlugin::tr("Jump slices"), q);
  QObject::connect(this->JumpSlicesAction, SIGNAL(triggered()), q, SLOT(jumpSlices()));

  // Clone segment action
  this->CloneSegmentAction = new QAction(qSlicerSubjectHierarchySegmentsPlugin::tr("Clone"), q);
  qSlicerSubjectHierarchyAbstractPlugin::setActionPosition(this->CloneSegmentAction,                           //
                                                           qSlicerSubjectHierarchyAbstractPlugin::SectionNode, //
                                                           0,                                                  // "Rename" action's index
                                                           1.0);                                               // put it right after
  QObject::connect(this->CloneSegmentAction, SIGNAL(triggered()), q, SLOT(cloneSegment()));

  this->OpacityMenu = new QMenu(qSlicerSubjectHierarchySegmentsPlugin::tr("Opacity"));
  this->OpacitySlider = new ctkDoubleSlider(this->OpacityMenu);
  this->OpacitySlider->setOrientation(Qt::Horizontal);
  this->OpacitySlider->setRange(0.0, 1.0);
  this->OpacitySlider->setSingleStep(0.1);
  QObject::connect(this->OpacitySlider, SIGNAL(valueChanged(double)), q, SLOT(setOpacityForCurrentItem(double)));
  QWidgetAction* opacityAction = new QWidgetAction(this->OpacityMenu);
  opacityAction->setDefaultWidget(this->OpacitySlider);
  this->OpacityMenu->addAction(opacityAction);

  this->OpacityAction = new QAction(qSlicerSubjectHierarchySegmentsPlugin::tr("Opacity"), q);
  this->OpacityAction->setToolTip(qSlicerSubjectHierarchySegmentsPlugin::tr("Set segment opacity in the sub-menu"));
  this->OpacityAction->setMenu(this->OpacityMenu);
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentsPluginPrivate::~qSlicerSubjectHierarchySegmentsPluginPrivate() = default;

//-----------------------------------------------------------------------------
// qSlicerSubjectHierarchySegmentsPlugin methods

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentsPlugin::qSlicerSubjectHierarchySegmentsPlugin(QObject* parent)
  : Superclass(parent)
  , d_ptr(new qSlicerSubjectHierarchySegmentsPluginPrivate(*this))
{
  this->m_Name = QString("Segments");

  Q_D(qSlicerSubjectHierarchySegmentsPlugin);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerSubjectHierarchySegmentsPlugin::~qSlicerSubjectHierarchySegmentsPlugin() = default;

//----------------------------------------------------------------------------
double qSlicerSubjectHierarchySegmentsPlugin::canReparentItemInsideSubjectHierarchy(vtkIdType itemID, vtkIdType parentItemID) const
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return 0.0;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return 0.0;
  }
  if (parentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID //
      || shNode->GetItemAttribute(itemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName()).empty())
  {
    // Cannot reparent if item is not a segment or there is no parent
    return 0.0;
  }

  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(parentItemID));
  if (segmentationNode)
  {
    // If item is segment and parent is segmentation then can reparent
    return 1.0;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchySegmentsPlugin::reparentItemInsideSubjectHierarchy(vtkIdType itemID, vtkIdType parentItemID)
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return false;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return false;
  }
  if (parentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID                                      //
      || shNode->GetItemAttribute(itemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName()).empty() //
      || shNode->GetItemParent(itemID) == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    // Cannot reparent if item is not a segment or there is no parent
    return false;
  }

  // Get source and target segmentation items
  vtkMRMLSegmentationNode* fromSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(shNode->GetItemParent(itemID)));
  vtkMRMLSegmentationNode* toSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(parentItemID));
  if (!fromSegmentationNode || !toSegmentationNode)
  {
    return false;
  }

  if (fromSegmentationNode == toSegmentationNode)
  {
    // If source and target segmentation are the same, then no reparenting is needed.
    // The segmentation plugin has already taken care of the item reordering.
    return true;
  }

  // Get segment ID
  std::string segmentId(shNode->GetItemAttribute(itemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName()));

  // Perform reparenting
  // Note: No actual subject hierarchy reparenting is done, because the function call below triggers
  //   removal and addition of segment subject hierarchy nodes as the segment is removed from source
  //   segmentation and added to target segmentation
  bool success = toSegmentationNode->GetSegmentation()->CopySegmentFromSegmentation(fromSegmentationNode->GetSegmentation(), segmentId, true);

  // Notify user if failed to reparent
  if (!success)
  {
    // If the two source representations are the same, then probably the segment IDs were duplicate
    if (fromSegmentationNode->GetSegmentation()->GetSourceRepresentationName() == toSegmentationNode->GetSegmentation()->GetSourceRepresentationName())
    {
      QString message = tr("Segment ID of the moved segment (%1) might exist in the target segmentation.\n"
                           "Please check the error window for details.")
                          .arg(segmentId.c_str());
      QMessageBox::warning(nullptr, tr("Failed to move segment between segmentations"), message);
      return false;
    }

    // Otherwise source representation has to be changed
    QString message = tr("Cannot convert source representation '%1' into target source '%2',"
                         "thus unable to move segment '%3' from segmentation '%4' to '%5'.\n\n"
                         "Would you like to change the source representation of '%5' to '%1'?\n\n"
                         "Note: This may result in unwanted data loss in %5.")
                        .arg(fromSegmentationNode->GetSegmentation()->GetSourceRepresentationName().c_str())
                        .arg(toSegmentationNode->GetSegmentation()->GetSourceRepresentationName().c_str())
                        .arg(segmentId.c_str())
                        .arg(fromSegmentationNode->GetName())
                        .arg(toSegmentationNode->GetName());
    QMessageBox::StandardButton answer =
      QMessageBox::question(nullptr, tr("Failed to move segment between segmentations"), message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::Yes)
    {
      // Convert target segmentation to source representation of source segmentation
      bool successfulConversion = toSegmentationNode->GetSegmentation()->CreateRepresentation(fromSegmentationNode->GetSegmentation()->GetSourceRepresentationName());
      if (!successfulConversion)
      {
        QString message = tr("Failed to convert %1 to %2").arg(toSegmentationNode->GetName()).arg(fromSegmentationNode->GetSegmentation()->GetSourceRepresentationName().c_str());
        QMessageBox::warning(nullptr, tr("Conversion failed"), message);
        return false;
      }

      // Change source representation of target to that of source
      toSegmentationNode->GetSegmentation()->SetSourceRepresentationName(fromSegmentationNode->GetSegmentation()->GetSourceRepresentationName());

      // Retry reparenting
      return this->reparentItemInsideSubjectHierarchy(itemID, parentItemID);
    }
    return false;
  }

  return true;
}

//---------------------------------------------------------------------------
double qSlicerSubjectHierarchySegmentsPlugin::canOwnSubjectHierarchyItem(vtkIdType itemID) const
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return 0.0;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return 0.0;
  }

  // Segment
  if (!shNode->GetItemAttribute(itemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName()).empty())
  {
    return 1.0;
  }

  return 0.0;
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchySegmentsPlugin::roleForPlugin() const
{
  return "Segment";
}

//---------------------------------------------------------------------------
const QString qSlicerSubjectHierarchySegmentsPlugin::helpText() const
{
  // return QString("<p style=\" margin-top:4px; margin-bottom:1px; margin-left:0px; margin-right:0px; "
  //   "-qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'sans-serif'; font-size:9pt; "
  //   "font-weight:600; color:#000000;\">Create new Contour set from scratch</span></p>"
  //   "<p style=\" margin-top:0px; margin-bottom:11px; margin-left:26px; margin-right:0px; "
  //   "-qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'sans-serif'; "
  //   "font-size:9pt; color:#000000;\">Right-click on an existing Study node and select 'Create child contour set'. "
  //   "This menu item is only available for Study level nodes</span></p>");
  // TODO:
  return QString();
}

//-----------------------------------------------------------------------------
QString qSlicerSubjectHierarchySegmentsPlugin::tooltip(vtkIdType itemID) const
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return tr("Invalid");
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return tr("Invalid");
  }
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->mrmlScene();
  if (!scene)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid MRML scene";
    return tr("Invalid");
  }

  // Get basic tooltip from abstract plugin
  QString tooltipString = Superclass::tooltip(itemID);

  if (scene->IsImporting())
  {
    // During import SH node may be created before the segmentation is read into the scene,
    // so don't attempt to access the segment yet
    return tooltipString;
  }

  // Get segmentation node and segment
  vtkMRMLSegmentationNode* segmentationNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyItem(itemID, scene);
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to find segmentation node for segment subject hierarchy item " << shNode->GetItemName(itemID).c_str();
    return tooltipString;
  }
  vtkSegment* segment = vtkSlicerSegmentationsModuleLogic::GetSegmentForSegmentSubjectHierarchyItem(itemID, scene);
  if (!segment)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to get segment for segment subject hierarchy item " << shNode->GetItemName(itemID).c_str();
    return tooltipString;
  }

  // Representations
  std::vector<std::string> containedRepresentationNames;
  segment->GetContainedRepresentationNames(containedRepresentationNames);
  QString representations;
  if (containedRepresentationNames.empty())
  {
    representations = tr("None");
  }
  else
  {
    for (std::vector<std::string>::iterator reprIt = containedRepresentationNames.begin(); reprIt != containedRepresentationNames.end(); ++reprIt)
    {
      if (!representations.isEmpty())
      {
        representations.append(", ");
      }
      representations.append(reprIt->c_str());
    }
  }

  // Color
  double color[3] = { 0.0, 0.0, 0.0 };
  segment->GetColor(color);
  // Tags
  std::map<std::string, std::string> tagsMap;
  segment->GetTags(tagsMap);
  QString tags;
  if (tagsMap.empty())
  {
    tags = tr("None");
  }
  else
  {
    for (std::map<std::string, std::string>::iterator tagIt = tagsMap.begin(); tagIt != tagsMap.end(); ++tagIt)
    {
      if (!tags.isEmpty())
      {
        tags.append(", ");
      }
      tags.append(QString::fromStdString(tagIt->first) + ": " + QString::fromStdString(tagIt->second));
    }
  }
  tooltipString.append(tr("Segment - ID: %1, Representations: %2, Color: (%3, %4, %5)\nTags: %6")
                         .arg(QString::fromStdString(segmentationNode->GetSegmentation()->GetSegmentIdBySegment(segment)))
                         .arg(representations)
                         .arg((int)(color[0] * 255))
                         .arg((int)(color[1] * 255))
                         .arg((int)(color[2] * 255))
                         .arg(tags));
  return tooltipString;
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchySegmentsPlugin::icon(vtkIdType itemID)
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return QIcon();
  }

  Q_D(qSlicerSubjectHierarchySegmentsPlugin);

  // Contour set
  if (this->canOwnSubjectHierarchyItem(itemID))
  {
    return d->SegmentIcon;
  }

  // Item unknown by plugin
  return QIcon();
}

//---------------------------------------------------------------------------
QIcon qSlicerSubjectHierarchySegmentsPlugin::visibilityIcon(int visible)
{
  // Have the default plugin (which is not registered) take care of this
  return qSlicerSubjectHierarchyPluginHandler::instance()->defaultPlugin()->visibilityIcon(visible);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::setDisplayVisibility(vtkIdType itemID, int visible)
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  // Get segmentation node and display node
  vtkMRMLSegmentationNode* segmentationNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyItem(itemID, shNode->GetScene());
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to find segmentation node for segment subject hierarchy item " << shNode->GetItemName(itemID).c_str();
    return;
  }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (!displayNode)
  {
    qCritical() << Q_FUNC_INFO << ": No display node for segmentation";
    return;
  }

  // Get segment ID
  std::string segmentId = shNode->GetItemAttribute(itemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName());

  // Set visibility
  displayNode->SetSegmentVisibility(segmentId, (bool)visible);

  // Trigger update of visibility icon
  shNode->ItemModified(itemID);
}

//-----------------------------------------------------------------------------
int qSlicerSubjectHierarchySegmentsPlugin::getDisplayVisibility(vtkIdType itemID) const
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return -1;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return -1;
  }
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->mrmlScene();
  if (!scene)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid MRML scene";
    return -1;
  }

  if (scene->IsImporting())
  {
    // during import SH node may be created before the segmentation is read into the scene,
    // so don't attempt to access the segment yet
    return -1;
  }

  // Get segmentation node and display node
  vtkMRMLSegmentationNode* segmentationNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyItem(itemID, scene);
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to find segmentation node for segment subject hierarchy item " << shNode->GetItemName(itemID).c_str();
    return -1;
  }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (!displayNode)
  {
    return -1;
  }

  // Get segment ID
  std::string segmentId = shNode->GetItemAttribute(itemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName());

  // Get visibility
  return (displayNode->GetSegmentVisibility(segmentId) ? 1 : 0);
}

//-----------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::setDisplayColor(vtkIdType itemID, QColor color, QMap<int, QVariant> terminologyMetaData)
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return;
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->mrmlScene();
  if (!scene)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid MRML scene";
    return;
  }

  // Get segment
  vtkSegment* segment = vtkSlicerSegmentationsModuleLogic::GetSegmentForSegmentSubjectHierarchyItem(itemID, scene);
  if (!segment)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to get segment for segment subject hierarchy item " << shNode->GetItemName(itemID).c_str();
    return;
  }

  // Set terminology metadata
  if (terminologyMetaData.contains(qSlicerTerminologyItemDelegate::TerminologyRole))
  {
    segment->SetTerminology(terminologyMetaData[qSlicerTerminologyItemDelegate::TerminologyRole].toString().toStdString());
  }
  if (terminologyMetaData.contains(qSlicerTerminologyItemDelegate::DefaultTerminologyRole))
  {
    vtkMRMLSegmentationNode* segmentationNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyItem(itemID, shNode->GetScene());
    if (segmentationNode)
    {
      vtkSlicerTerminologiesModuleLogic::SetDefaultTerminologyEntryAsString(segmentationNode,
                                                                            terminologyMetaData[qSlicerTerminologyItemDelegate::DefaultTerminologyRole].toString().toStdString());
    }
  }
  if (terminologyMetaData.contains(qSlicerTerminologyItemDelegate::NameRole))
  {
    segment->SetName(terminologyMetaData[qSlicerTerminologyItemDelegate::NameRole].toString().toUtf8().constData());
  }
  if (terminologyMetaData.contains(qSlicerTerminologyItemDelegate::NameAutoGeneratedRole))
  {
    segment->SetNameAutoGenerated(terminologyMetaData[qSlicerTerminologyItemDelegate::NameAutoGeneratedRole].toBool());
  }
  if (terminologyMetaData.contains(qSlicerTerminologyItemDelegate::ColorAutoGeneratedRole))
  {
    segment->SetColorAutoGenerated(terminologyMetaData[qSlicerTerminologyItemDelegate::ColorAutoGeneratedRole].toBool());
  }

  // Set color.
  // We can end up in an infinite loop if the color cannot be set (by constantly triggering an update
  // due to detected change), therefore we compare the color carefully (clamp to valid range and use
  // a small tolerance for comparison).
  double oldColor[3];
  segment->GetColor(oldColor);
  double newColor[3] = { color.redF(), color.greenF(), color.blueF() };
  bool colorChanged = false;
  for (int i = 0; i < 3; i++)
  {
    if (!(oldColor[i] >= 0.0)) // use ! >= instead of < to include NaN values
    {
      oldColor[i] = 0.0;
    }
    else if (oldColor[i] > 1.0)
    {
      oldColor[i] = 1.0;
    }
    if (!(newColor[i] >= 0.0)) // use ! >= instead of < to include NaN values
    {
      newColor[i] = 0.0;
    }
    else if (newColor[i] > 1.0)
    {
      newColor[i] = 1.0;
    }
    if (fabs(oldColor[i] - newColor[i]) > 1e-6)
    {
      colorChanged = true;
    }
  }
  if (colorChanged)
  {
    segment->SetColor(newColor);

    // Trigger update of color swatch
    shNode->ItemModified(itemID);
  }
}

//-----------------------------------------------------------------------------
QColor qSlicerSubjectHierarchySegmentsPlugin::getDisplayColor(vtkIdType itemID, QMap<int, QVariant>& terminologyMetaData) const
{
  if (itemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid input item";
    return QColor(0, 0, 0, 0);
  }
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return QColor(0, 0, 0, 0);
  }
  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->mrmlScene();
  if (!scene)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid MRML scene";
    return QColor(0, 0, 0, 0);
  }

  if (scene->IsImporting())
  {
    // During import SH node may be created before the segmentation is read into the scene,
    // so don't attempt to access the segment yet
    return QColor(0, 0, 0, 0);
  }

  // Get segment
  vtkSegment* segment = vtkSlicerSegmentationsModuleLogic::GetSegmentForSegmentSubjectHierarchyItem(itemID, scene);
  if (!segment)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to get segment for segment subject hierarchy item " << shNode->GetItemName(itemID).c_str();
    return QColor(0, 0, 0, 0);
  }

  // Get terminology metadata

  terminologyMetaData.clear();
  std::string tagValue;
  terminologyMetaData[qSlicerTerminologyItemDelegate::TerminologyRole] = QVariant(QString::fromStdString(segment->GetTerminology()));

  vtkMRMLSegmentationNode* segmentationNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyItem(itemID, shNode->GetScene());
  if (segmentationNode)
  {
    std::string defaultEntry = vtkSlicerTerminologiesModuleLogic::GetDefaultTerminologyEntryAsString(segmentationNode);
    if (!defaultEntry.empty())
    {
      terminologyMetaData[qSlicerTerminologyItemDelegate::DefaultTerminologyRole] = QString::fromStdString(defaultEntry);
    }
  }

  terminologyMetaData[qSlicerTerminologyItemDelegate::NameRole] = segment->GetName();
  terminologyMetaData[qSlicerTerminologyItemDelegate::NameAutoGeneratedRole] = segment->GetNameAutoGenerated();
  terminologyMetaData[qSlicerTerminologyItemDelegate::ColorAutoGeneratedRole] = segment->GetColorAutoGenerated();

  // Get and return color
  double* colorArray = segment->GetColor();
  return QColor::fromRgbF(colorArray[0], colorArray[1], colorArray[2]);
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchySegmentsPlugin::itemContextMenuActions() const
{
  Q_D(const qSlicerSubjectHierarchySegmentsPlugin);
  QList<QAction*> actions;
  actions << d->CloneSegmentAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::showContextMenuActionsForItem(vtkIdType itemID)
{
  Q_D(const qSlicerSubjectHierarchySegmentsPlugin);

  qSlicerSubjectHierarchySegmentationsPlugin* segmentationsPlugin =
    qobject_cast<qSlicerSubjectHierarchySegmentationsPlugin*>(qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Segmentations"));

  // Segments plugin shows all segmentations plugin functions in segment context menu
  segmentationsPlugin->showContextMenuActionsForItem(itemID);

  // Owned Segment
  if (this->canOwnSubjectHierarchyItem(itemID) && this->isThisPluginOwnerOfItem(itemID))
  {
    d->CloneSegmentAction->setVisible(true);
  }
}

//---------------------------------------------------------------------------
QList<QAction*> qSlicerSubjectHierarchySegmentsPlugin::visibilityContextMenuActions() const
{
  Q_D(const qSlicerSubjectHierarchySegmentsPlugin);

  QList<QAction*> actions;
  actions << d->OpacityAction << d->ShowOnlyCurrentSegmentAction << d->ShowAllSegmentsAction << d->JumpSlicesAction;
  return actions;
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::showVisibilityContextMenuActionsForItem(vtkIdType itemID)
{
  Q_D(const qSlicerSubjectHierarchySegmentsPlugin);

  qSlicerSubjectHierarchySegmentationsPlugin* segmentationsPlugin =
    qobject_cast<qSlicerSubjectHierarchySegmentationsPlugin*>(qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Segmentations"));

  // Segments plugin shows all segmentations plugin functions in segment context menu
  segmentationsPlugin->showContextMenuActionsForItem(itemID);

  // Owned Segment
  if (this->canOwnSubjectHierarchyItem(itemID) && this->isThisPluginOwnerOfItem(itemID))
  {
    d->ShowOnlyCurrentSegmentAction->setVisible(true);
    d->ShowAllSegmentsAction->setVisible(true);
    d->JumpSlicesAction->setVisible(true);
    d->OpacityAction->setVisible(true);

    // Set current segment opacity to the opacity slider
    vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
    if (!shNode)
    {
      qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
      return;
    }

    vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->mrmlScene();
    if (!scene)
    {
      qCritical() << Q_FUNC_INFO << ": Invalid MRML scene";
      return;
    }
    vtkMRMLSegmentationNode* segmentationNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyItem(itemID, shNode->GetScene());
    if (!segmentationNode)
    {
      qCritical() << Q_FUNC_INFO << ": Unable to find segmentation node for segment subject hierarchy item " << shNode->GetItemName(itemID).c_str();
      return;
    }
    std::string segmentId = shNode->GetItemAttribute(itemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName());
    vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
    if (!displayNode)
    {
      qCritical() << Q_FUNC_INFO << ": Unable to find segmentation display node for segment subject hierarchy item " << shNode->GetItemName(itemID).c_str();
      return;
    }

    d->OpacitySlider->setValue(displayNode->GetSegmentOpacity3D(segmentId));
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::editProperties(vtkIdType itemID)
{
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }

  // Switch to segmentations module and select parent segmentation node
  vtkMRMLSegmentationNode* segmentationNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyItem(itemID, shNode->GetScene());
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to find segmentation node for segment subject hierarchy item " << shNode->GetItemName(itemID).c_str();
    return;
  }

  qSlicerApplication::application()->openNodeModule(segmentationNode);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::showOnlyCurrentSegment()
{
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }
  vtkIdType currentItemID = qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (currentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current item";
    return;
  }

  // Get segmentation node and display node
  vtkIdType segmentationShItemID = shNode->GetItemParent(currentItemID);
  if (segmentationShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to find segmentation subject hierarchy item for segment item " << shNode->GetItemName(currentItemID).c_str();
    return;
  }
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(segmentationShItemID));
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to find segmentation node for segment subject hierarchy item " << shNode->GetItemName(currentItemID).c_str();
    return;
  }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (!displayNode)
  {
    qCritical() << Q_FUNC_INFO << ": No display node for segmentation";
    return;
  }

  // Hide all segments except the current one
  std::vector<vtkIdType> segmentShItemIDs;
  shNode->GetItemChildren(segmentationShItemID, segmentShItemIDs);
  std::vector<vtkIdType>::iterator segmentIt;
  for (segmentIt = segmentShItemIDs.begin(); segmentIt != segmentShItemIDs.end(); ++segmentIt)
  {
    vtkIdType segmentItemID = (*segmentIt);
    bool visible = false;
    if (segmentItemID == currentItemID)
    {
      visible = true;
    }

    // Get segment ID
    std::string segmentId = shNode->GetItemAttribute(segmentItemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName());
    // Set visibility
    displayNode->SetSegmentVisibility(segmentId, visible);
    // Trigger update of visibility icon
    shNode->ItemModified(segmentItemID);
  }
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::showAllSegments()
{
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }
  vtkIdType currentItemID = qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (currentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current item";
    return;
  }

  // Get segmentation node and display node
  vtkIdType segmentationShItemID = shNode->GetItemParent(currentItemID);
  if (segmentationShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to find segmentation subject hierarchy item for segment item " << shNode->GetItemName(currentItemID).c_str();
    return;
  }
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(segmentationShItemID));
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to find segmentation node for segment subject hierarchy item " << shNode->GetItemName(currentItemID).c_str();
    return;
  }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (!displayNode)
  {
    qCritical() << Q_FUNC_INFO << ": No display node for segmentation";
    return;
  }

  // Show all segments
  std::vector<vtkIdType> segmentShItemIDs;
  shNode->GetItemChildren(segmentationShItemID, segmentShItemIDs);
  std::vector<vtkIdType>::iterator segmentIt;
  for (segmentIt = segmentShItemIDs.begin(); segmentIt != segmentShItemIDs.end(); ++segmentIt)
  {
    vtkIdType segmentItemID = (*segmentIt);
    // Get segment ID
    std::string segmentId = shNode->GetItemAttribute(segmentItemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName());
    // Set visibility
    displayNode->SetSegmentVisibility(segmentId, true);
    // Trigger update of visibility icon
    shNode->ItemModified(segmentItemID);
  }
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::jumpSlices()
{
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }
  vtkIdType currentItemID = qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (currentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current item";
    return;
  }

  // Get segmentation node
  vtkIdType segmentationShItemID = shNode->GetItemParent(currentItemID);
  if (segmentationShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to find segmentation subject hierarchy item for segment item " << shNode->GetItemName(currentItemID).c_str();
    return;
  }
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(segmentationShItemID));
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to find segmentation node for segment subject hierarchy item " << shNode->GetItemName(currentItemID).c_str();
    return;
  }

  // Get segment ID
  std::string segmentId = shNode->GetItemAttribute(currentItemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName());

  // Get center position of segment
  double* segmentCenterPosition = segmentationNode->GetSegmentCenterRAS(segmentId);
  if (!segmentCenterPosition)
  {
    return;
  }

  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  if (!layoutManager)
  {
    // Application is closing
    return;
  }
  for (const QString& sliceViewName : layoutManager->sliceViewNames())
  {
    // Check if segmentation is visible in this view
    qMRMLSliceWidget* sliceWidget = layoutManager->sliceWidget(sliceViewName);
    vtkMRMLSliceNode* sliceNode = sliceWidget->mrmlSliceNode();
    if (!sliceNode || !sliceNode->GetID())
    {
      continue;
    }
    bool visibleInView = false;
    int numberOfDisplayNodes = segmentationNode->GetNumberOfDisplayNodes();
    for (int displayNodeIndex = 0; displayNodeIndex < numberOfDisplayNodes; displayNodeIndex++)
    {
      vtkMRMLDisplayNode* segmentationDisplayNode = segmentationNode->GetNthDisplayNode(displayNodeIndex);
      if (segmentationDisplayNode && segmentationDisplayNode->IsDisplayableInView(sliceNode->GetID()))
      {
        visibleInView = true;
        break;
      }
    }
    if (!visibleInView)
    {
      continue;
    }
    sliceNode->JumpSliceByCentering(segmentCenterPosition[0], segmentCenterPosition[1], segmentCenterPosition[2]);
  }
}

//------------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::cloneSegment()
{
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }
  vtkIdType currentItemID = qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (currentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current item";
    return;
  }

  // Get segmentation node
  vtkIdType segmentationShItemID = shNode->GetItemParent(currentItemID);
  if (segmentationShItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to find segmentation subject hierarchy item for segment item " << shNode->GetItemName(currentItemID).c_str();
    return;
  }
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(shNode->GetItemDataNode(segmentationShItemID));
  if (!segmentationNode || !segmentationNode->GetSegmentation())
  {
    qCritical() << Q_FUNC_INFO << ": Unable to find segmentation node for segment subject hierarchy item " << shNode->GetItemName(currentItemID).c_str();
    return;
  }

  // Get segment ID
  std::string segmentId = shNode->GetItemAttribute(currentItemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName());

  // Get segment and segmentation object
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  vtkSegment* segment = segmentation->GetSegment(segmentId);
  if (!segment)
  {
    qCritical() << Q_FUNC_INFO << " failed: error getting the segment object";
    return;
  }

  // Copy
  vtkNew<vtkSegment> segmentCopy;
  segmentCopy->DeepCopy(segment);

  // Find the next segment's ID, because we want to insert the copied segment right below the current segment
  int segmentIndex = segmentation->GetSegmentIndex(segmentId);
  std::string insertBeforeSegmentId;
  if (segmentIndex + 1 < segmentation->GetNumberOfSegments())
  {
    insertBeforeSegmentId = segmentation->GetNthSegmentID(segmentIndex + 1);
  }

  std::string targetSegmentId = segmentation->GenerateUniqueSegmentID(segmentId);
  if (!segmentation->AddSegment(segmentCopy, targetSegmentId, insertBeforeSegmentId))
  {
    qCritical() << Q_FUNC_INFO << " failed: error adding cloned segment '" << segmentId.c_str() << "' to segmentation";
    return;
  }
}
//---------------------------------------------------------------------------
bool qSlicerSubjectHierarchySegmentsPlugin::showItemInView(vtkIdType itemID, vtkMRMLAbstractViewNode* viewNode, vtkIdList* allItemsToShow)
{
  Q_UNUSED(itemID);
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return false;
  }
  vtkIdType currentItemID = qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (currentItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current item";
    return false;
  }

  // Get the segmentation node's itemID and use the segmentation plugin to show it.
  vtkIdType segmentationItemId = shNode->GetItemParent(currentItemID);
  qSlicerSubjectHierarchySegmentationsPlugin* segmentationsPlugin =
    qobject_cast<qSlicerSubjectHierarchySegmentationsPlugin*>(qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Segmentations"));
  return segmentationsPlugin->showItemInView(segmentationItemId, viewNode, allItemsToShow);
}

//---------------------------------------------------------------------------
void qSlicerSubjectHierarchySegmentsPlugin::setOpacityForCurrentItem(double opacity)
{
  // Get currently selected node and scene
  vtkMRMLSubjectHierarchyNode* shNode = qSlicerSubjectHierarchyPluginHandler::instance()->subjectHierarchyNode();
  if (!shNode)
  {
    qCritical() << Q_FUNC_INFO << ": Failed to access subject hierarchy node";
    return;
  }
  vtkIdType currentItemID = qSlicerSubjectHierarchyPluginHandler::instance()->currentItem();
  if (!currentItemID)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid current subject hierarchy item!";
    return;
  }

  vtkMRMLScene* scene = qSlicerSubjectHierarchyPluginHandler::instance()->mrmlScene();
  if (!scene)
  {
    qCritical() << Q_FUNC_INFO << ": Invalid MRML scene";
    return;
  }
  vtkMRMLSegmentationNode* segmentationNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyItem(currentItemID, shNode->GetScene());
  if (!segmentationNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to find segmentation node for segment subject hierarchy item " << shNode->GetItemName(currentItemID).c_str();
    return;
  }
  std::string segmentId = shNode->GetItemAttribute(currentItemID, vtkMRMLSegmentationNode::GetSegmentIDAttributeName());
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (!displayNode)
  {
    qCritical() << Q_FUNC_INFO << ": Unable to find segmentation display node for segment subject hierarchy item " << shNode->GetItemName(currentItemID).c_str();
    return;
  }

  displayNode->SetSegmentOpacity(segmentId, opacity);
}
