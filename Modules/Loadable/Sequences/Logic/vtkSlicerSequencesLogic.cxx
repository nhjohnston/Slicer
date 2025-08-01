/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// #define ENABLE_PERFORMANCE_PROFILING

// Sequence Logic includes
#include "vtkSlicerSequencesLogic.h"

// MRMLSequence includes
#include "vtkMRMLLinearTransformSequenceStorageNode.h"
#include "vtkMRMLSequenceBrowserNode.h"
#include "vtkMRMLSequenceNode.h"
#include "vtkMRMLSequenceStorageNode.h"
#include "vtkMRMLVolumeSequenceStorageNode.h"

// MRML includes
#include "vtkCacheManager.h"
#include "vtkMRMLCameraNode.h"
#include "vtkMRMLI18N.h"
#include "vtkMRMLLabelMapVolumeNode.h"
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLMessageCollection.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLTransformNode.h"
#ifdef ENABLE_PERFORMANCE_PROFILING
# include "vtkTimerLog.h"
#endif

// VTK includes
#include <vtkAbstractTransform.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkTimerLog.h>
#include <vtksys/SystemTools.hxx>

// STL includes
#include <algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSequencesLogic);

//----------------------------------------------------------------------------
vtkSlicerSequencesLogic::vtkSlicerSequencesLogic() = default;

//----------------------------------------------------------------------------
vtkSlicerSequencesLogic::~vtkSlicerSequencesLogic() = default;

//----------------------------------------------------------------------------
void vtkSlicerSequencesLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerSequencesLogic::RegisterNodes()
{
  if (this->GetMRMLScene() == nullptr)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLSequenceBrowserNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::UpdateFromMRMLScene()
{
  if (this->GetMRMLScene() == nullptr)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  // TODO: probably we need to add observer to all vtkMRMLSequenceBrowserNode-type nodes
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (node == nullptr)
  {
    vtkErrorMacro("An invalid node is attempted to be added");
    return;
  }
  if (node->IsA("vtkMRMLSequenceBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeAdded: Have a vtkMRMLSequenceBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node); // remove any previous observation that might have been added
    vtkNew<vtkIntArray> events;
    events->InsertNextValue(vtkMRMLSequenceBrowserNode::ProxyNodeModifiedEvent);
    events->InsertNextValue(vtkMRMLSequenceBrowserNode::SequenceNodeModifiedEvent);
    events->InsertNextValue(vtkCommand::ModifiedEvent);
    vtkObserveMRMLNodeEventsMacro(node, events.GetPointer());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (node == nullptr)
  {
    vtkErrorMacro("An invalid node is attempted to be removed");
    return;
  }
  if (node->IsA("vtkMRMLSequenceBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeRemoved: Have a vtkMRMLSequenceBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node);
  }
}

//----------------------------------------------------------------------------
vtkMRMLSequenceNode* vtkSlicerSequencesLogic::AddSequence(const char* filename, vtkMRMLMessageCollection* userMessages /*=nullptr*/)
{
  if (this->GetMRMLScene() == nullptr || filename == nullptr)
  {
    return nullptr;
  }
  vtkNew<vtkMRMLSequenceNode> sequenceNode;
  vtkNew<vtkMRMLSequenceStorageNode> sequenceStorageNode;
  vtkNew<vtkMRMLVolumeSequenceStorageNode> volumeSequenceStorageNode;

  vtkMRMLStorageNode* storageNode = nullptr;
  if (sequenceStorageNode->SupportedFileType(filename))
  {
    storageNode = sequenceStorageNode;
  }
  else if (volumeSequenceStorageNode->SupportedFileType(filename))
  {
    storageNode = volumeSequenceStorageNode;
  }
  else
  {
    vtkErrorToMessageCollectionMacro(userMessages, "vtkSlicerSequencesLogic::AddSequence", "Failed to read sequence. Unsupported file type: " << filename);
    return nullptr;
  }

  // check for local or remote files
  int useURI = 0; // false;
  if (this->GetMRMLScene()->GetCacheManager() != nullptr)
  {
    useURI = this->GetMRMLScene()->GetCacheManager()->IsRemoteReference(filename);
    vtkDebugMacro("AddSequence: file name is remote: " << filename);
  }
  const char* localFile = 0;
  if (useURI)
  {
    storageNode->SetURI(filename);
    // reset filename to the local file name
    localFile = ((this->GetMRMLScene())->GetCacheManager())->GetFilenameFromURI(filename);
  }
  else
  {
    storageNode->SetFileName(filename);
    localFile = filename;
  }
  const std::string fname(localFile ? localFile : "");
  // the sequence name is based on the file name (vtksys call should work even if
  // file is not on disk yet)
  std::string name = vtksys::SystemTools::GetFilenameName(fname);
  vtkDebugMacro("AddSequence: got Sequence name = " << name.c_str());

  // check to see which node can read this type of file
  if (storageNode->SupportedFileType(name.c_str()))
  {
    std::string baseName = storageNode->GetFileNameWithoutExtension(fname.c_str());
    std::string uname(this->GetMRMLScene()->GetUniqueNameByString(baseName.c_str()));
    sequenceNode->SetName(uname.c_str());

    this->GetMRMLScene()->SaveStateForUndo();

    this->GetMRMLScene()->AddNode(storageNode);

    // Set the scene so that SetAndObserveStorageNodeID can find the node in the scene.
    sequenceNode->SetScene(this->GetMRMLScene());
    sequenceNode->SetAndObserveStorageNodeID(storageNode->GetID());

    this->GetMRMLScene()->AddNode(sequenceNode);

    // now set up the reading
    vtkDebugMacro("AddSequence: calling read on the storage node");
    int success = storageNode->ReadData(sequenceNode);
    if (!success)
    {
      vtkErrorMacro("AddSequence: error reading " << filename);
      if (userMessages)
      {
        userMessages->AddMessages(storageNode->GetUserMessages());
      }
      this->GetMRMLScene()->RemoveNode(sequenceNode);
      this->GetMRMLScene()->RemoveNode(storageNode);
      return nullptr;
    }
  }
  else
  {
    vtkErrorMacro("Couldn't read file: " << filename);
    return nullptr;
  }

  return sequenceNode.GetPointer();
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::UpdateAllProxyNodes()
{
  double updateStartTimeSec = vtkTimerLog::GetUniversalTime();

  vtkMRMLScene* scene = this->GetMRMLScene();
  if (scene == nullptr)
  {
    vtkErrorMacro("vtkSlicerSequencesLogic::UpdateAllProxyNodes failed: scene is invalid");
    return;
  }
  std::vector<vtkMRMLNode*> browserNodes;
  int numBrowserNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLSequenceBrowserNode", browserNodes);
  for (int i = 0; i < numBrowserNodes; i++)
  {
    vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(browserNodes[i]);
    if (browserNode == nullptr)
    {
      vtkErrorMacro("Browser node is invalid");
      continue;
    }
    if (!browserNode->GetPlaybackActive())
    {
      this->LastSequenceBrowserUpdateTimeSec.erase(browserNode);
      continue;
    }
    if (this->LastSequenceBrowserUpdateTimeSec.find(browserNode) == this->LastSequenceBrowserUpdateTimeSec.end())
    {
      // we just started to play now, no need to update output nodes yet
      this->LastSequenceBrowserUpdateTimeSec[browserNode] = updateStartTimeSec;
      continue;
    }
    // play is already in progress
    double elapsedTimeSec = updateStartTimeSec - this->LastSequenceBrowserUpdateTimeSec[browserNode];
    // compute how many items we need to jump; if not enough time passed to jump at least to the next item
    // then we don't do anything (let the elapsed time cumulate)
    int selectionIncrement = floor(elapsedTimeSec * browserNode->GetPlaybackRateFps() + 0.5); // floor with +0.5 is rounding
    if (selectionIncrement > 0)
    {
      this->LastSequenceBrowserUpdateTimeSec[browserNode] = updateStartTimeSec;
      if (!browserNode->GetPlaybackItemSkippingEnabled())
      {
        selectionIncrement = 1;
      }
      browserNode->SelectNextItem(selectionIncrement);
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::UpdateProxyNodesFromSequences(vtkMRMLSequenceBrowserNode* browserNode)
{
#ifdef ENABLE_PERFORMANCE_PROFILING
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
#endif

  if (this->UpdateProxyNodesFromSequencesInProgress.count(browserNode) > 0 || this->UpdateSequencesFromProxyNodesInProgress.count(browserNode) > 0)
  {
    // avoid infinite loops (caused by triggering a node update during a node update)
    return;
  }
  if (browserNode == nullptr)
  {
    vtkWarningMacro("vtkSlicerSequencesLogic::UpdateProxyNodesFromSequences failed: browserNode is invalid");
    return;
  }

  if (browserNode->GetRecordingActive())
  {
    // don't update proxy nodes while recording (sequence is updated from proxy nodes)
    return;
  }

  if (browserNode->GetMasterSequenceNode() == nullptr)
  {
    browserNode->RemoveAllProxyNodes();
    return;
  }

  vtkMRMLScene* scene = browserNode->GetScene();
  if (scene == nullptr)
  {
    vtkErrorMacro("vtkSlicerSequencesLogic::UpdateProxyNodesFromSequences failed: scene is invalid");
    return;
  }

  this->UpdateProxyNodesFromSequencesInProgress.insert(browserNode);

  int selectedItemNumber = browserNode->GetSelectedItemNumber();
  std::string indexValue("0");
  if (selectedItemNumber >= 0 && selectedItemNumber < browserNode->GetNumberOfItems())
  {
    indexValue = browserNode->GetMasterSequenceNode()->GetNthIndexValue(selectedItemNumber);
  }

  /// Pause rending to speed up the update
  if (this->GetApplicationLogic())
  {
    this->GetApplicationLogic()->PauseRender();
  }

  std::vector<vtkMRMLSequenceNode*> synchronizedSequenceNodes;
  browserNode->GetSynchronizedSequenceNodes(synchronizedSequenceNodes, true);

  // Store the previous modified state of nodes to allow calling EndModify when all the nodes are updated (to prevent multiple renderings on partial update)
  std::vector<std::pair<vtkMRMLNode*, int>> nodeModifiedStates;

  for (std::vector<vtkMRMLSequenceNode*>::iterator sourceSequenceNodeIt = synchronizedSequenceNodes.begin(); sourceSequenceNodeIt != synchronizedSequenceNodes.end();
       ++sourceSequenceNodeIt)
  {
    vtkMRMLSequenceNode* synchronizedSequenceNode = (*sourceSequenceNodeIt);
    if (synchronizedSequenceNode == nullptr)
    {
      vtkErrorMacro("Synchronized sequence node is invalid");
      continue;
    }
    if (!browserNode->GetPlayback(synchronizedSequenceNode))
    {
      continue;
    }

    vtkSmartPointer<vtkMRMLNode> sourceDataNode;
    int missingItemMode = browserNode->GetMissingItemMode(synchronizedSequenceNode);
    if (browserNode->GetSaveChanges(synchronizedSequenceNode))
    {
      // we want to save changes, therefore we have to make sure there is at least one data node. If no data
      // node is available for the current index, an empty one is added based on the missingItemMode
      if (synchronizedSequenceNode->GetNumberOfDataNodes() > 0)
      {
        sourceDataNode = synchronizedSequenceNode->GetDataNodeAtValue(indexValue, /* exactMatchRequired= */ true);
        if (sourceDataNode == nullptr)
        {
          // No source node is available for the current exact index, add one now.
          if (missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemCreateFromPrevious)
          {
            // Add a copy of the closest (previous) item into the sequence at the exact index.
            sourceDataNode = synchronizedSequenceNode->GetDataNodeAtValue(indexValue, /* exactMatchRequired= */ false);
            if (sourceDataNode)
            {
              sourceDataNode = synchronizedSequenceNode->SetDataNodeAtValue(sourceDataNode, indexValue);
            }
          }
          else
          {
            sourceDataNode = synchronizedSequenceNode->GetNthDataNode(0);
            if (sourceDataNode)
            {
              vtkSmartPointer<vtkMRMLNode> emptyNode = vtkSmartPointer<vtkMRMLNode>::Take(sourceDataNode->CreateNodeInstance());
              if (missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemCreateFromDefault)
              {
                // Add an empty node at the index
                sourceDataNode = synchronizedSequenceNode->SetDataNodeAtValue(emptyNode, indexValue);
              }
              else if (missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemSetToDefault)
              {
                // Do not add a node at this index
                sourceDataNode = emptyNode;
              }
            }
          }
        }
      }
      else
      {
        // There are no data nodes in the sequence.
        // Insert a new node node in the sequence based on missingItemMode.
        // Except if selectedItemNumber < 0, because that means that there is no timepoint in the sequence
        // and we should not add one (because then we could never delete the last item in a sequence when recording is enabled).
        sourceDataNode = browserNode->GetProxyNode(synchronizedSequenceNode);
        if (sourceDataNode && selectedItemNumber >= 0)
        {
          if (missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemCreateFromPrevious)
          {
            // Add proxy node at the index
            sourceDataNode = synchronizedSequenceNode->SetDataNodeAtValue(sourceDataNode, indexValue);
          }
          else
          {
            vtkSmartPointer<vtkMRMLNode> emptyNode = vtkSmartPointer<vtkMRMLNode>::Take(sourceDataNode->CreateNodeInstance());
            if (missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemCreateFromDefault)
            {
              // Add an empty node at the index
              sourceDataNode = synchronizedSequenceNode->SetDataNodeAtValue(emptyNode, indexValue);
            }
            else if (missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemSetToDefault)
            {
              // Do not add a node at this index
              sourceDataNode = emptyNode;
            }
          }
        }
      }
    }
    else
    {
      // Save changes is disabled.
      if (missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemCreateFromPrevious)
      {
        // Since we are not saving changes, we don't need to create missing item, we just need to display the current node.
        sourceDataNode = synchronizedSequenceNode->GetDataNodeAtValue(indexValue, /* exactMatchRequired= */ false);
      }
      if (missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemCreateFromDefault //
          || missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemSetToDefault   //
          || missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemIgnore         //
          || missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemDisplayHidden)
      {
        // We are not saving changes, but we may need to reset the proxy node to the default
        sourceDataNode = synchronizedSequenceNode->GetDataNodeAtValue(indexValue, /* exactMatchRequired= */ true);
        if (!sourceDataNode                                                     //
            && missingItemMode != vtkMRMLSequenceBrowserNode::MissingItemIgnore //
            && missingItemMode != vtkMRMLSequenceBrowserNode::MissingItemDisplayHidden)
        {
          // item is missing, use an empty node as source node for the proxy node
          sourceDataNode = synchronizedSequenceNode->GetNthDataNode(0);
          if (sourceDataNode)
          {
            vtkSmartPointer<vtkMRMLNode> emptyNode = vtkSmartPointer<vtkMRMLNode>::Take(sourceDataNode->CreateNodeInstance());
            sourceDataNode = emptyNode;
          }
        }
      }
    }

    if (sourceDataNode == nullptr)
    {
      if (missingItemMode == vtkMRMLSequenceBrowserNode::MissingItemDisplayHidden)
      {
        vtkMRMLNode* targetProxyNode = browserNode->GetProxyNode(synchronizedSequenceNode);
        vtkMRMLDisplayNode* proxyDisplayNode = vtkMRMLDisplayNode::SafeDownCast(targetProxyNode);
        if (proxyDisplayNode)
        {
          // Hide the proxy node if the source node is not available
          proxyDisplayNode->VisibilityOff();
        }
      }

      // no source node is available
      continue;
    }

    // Get the current target output node
    vtkMRMLNode* targetProxyNode = browserNode->GetProxyNode(synchronizedSequenceNode);
    if (targetProxyNode != nullptr)
    {
      // a proxy node with the requested role exists already
      if (strcmp(targetProxyNode->GetClassName(), sourceDataNode->GetClassName()) != 0)
      {
        // this node is of a different class, cannot be reused
        targetProxyNode = nullptr;
      }
    }

    bool newTargetProxyNodeWasCreated = false;
    if (targetProxyNode == nullptr)
    {
      // Create the proxy node (and display nodes) if they don't exist yet
      targetProxyNode = browserNode->AddProxyNode(sourceDataNode, synchronizedSequenceNode);
      newTargetProxyNodeWasCreated = true;
    }

    if (targetProxyNode == nullptr)
    {
      // failed to add target output node
      continue;
    }

    // Update the target node with the contents of the source node

    // Mostly it is a shallow copy (for example for volumes, models)
    std::pair<vtkMRMLNode*, int> nodeModifiedState(targetProxyNode, targetProxyNode->StartModify());
    nodeModifiedStates.push_back(nodeModifiedState);

    // TODO: if we really want to force non-mutable nodes in the sequence then we have to deep-copy, but that's slow.
    // Make sure that by default/most of the time shallow-copy is used.
    bool shallowCopy = browserNode->GetSaveChanges(synchronizedSequenceNode);
    targetProxyNode->CopyContent(sourceDataNode, !shallowCopy);

    // Singleton nodes must not be renamed, as they are often expected to exist by a specific name
    if (browserNode->GetOverwriteProxyName(synchronizedSequenceNode) && !targetProxyNode->GetSingletonTag())
    {
      // Generation of target proxy node name: base node name [IndexName = IndexValue IndexUnit]
      std::string indexName = synchronizedSequenceNode->GetIndexName();
      std::string unit = synchronizedSequenceNode->GetIndexUnit();
      // Save the base name (without the index name and value)
      targetProxyNode->SetAttribute("Sequences.BaseName", synchronizedSequenceNode->GetName());
      std::ostringstream targetProxyNodeNameStr;
      targetProxyNodeNameStr << synchronizedSequenceNode->GetName() << " [";
      if (browserNode->GetIndexDisplayMode() == vtkMRMLSequenceBrowserNode::IndexDisplayAsIndexValue)
      {
        if (!indexName.empty())
        {
          targetProxyNodeNameStr << indexName << "=";
        }
        targetProxyNodeNameStr << indexValue;
        if (!unit.empty())
        {
          targetProxyNodeNameStr << unit;
        }
      }
      else
      {
        targetProxyNodeNameStr << (selectedItemNumber + 1) << "/" << (synchronizedSequenceNode->GetNumberOfDataNodes());
      }
      targetProxyNodeNameStr << "]";
      targetProxyNode->SetName(targetProxyNodeNameStr.str().c_str());
    }

    if (newTargetProxyNodeWasCreated)
    {
      vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(targetProxyNode);
      if (displayableNode)
      {
        displayableNode->CreateDefaultSequenceDisplayNodes();
      }
      // Add default storage node now to avoid proxy node update when "Add data" dialog is invoked
      vtkMRMLStorableNode* storableNode = vtkMRMLStorableNode::SafeDownCast(targetProxyNode);
      if (storableNode)
      {
        storableNode->AddDefaultStorageNode();
      }
    }
  }

  // Finalize modifications, all at once. These will fire the node modified events and update renderers.
  for (std::vector<std::pair<vtkMRMLNode*, int>>::iterator nodeModifiedStateIt = nodeModifiedStates.begin(); nodeModifiedStateIt != nodeModifiedStates.end(); ++nodeModifiedStateIt)
  {
    (nodeModifiedStateIt->first)->EndModify(nodeModifiedStateIt->second);
  }

  this->UpdateProxyNodesFromSequencesInProgress.erase(browserNode);

#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkInfoMacro("UpdateProxyNodesFromSequences: " << timer->GetElapsedTime() << "sec\n");
#endif

  /// Resume rending
  if (this->GetApplicationLogic())
  {
    this->GetApplicationLogic()->ResumeRender();
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::UpdateSequencesFromProxyNodes(vtkMRMLSequenceBrowserNode* browserNode, vtkMRMLNode* proxyNode)
{
  if (this->UpdateProxyNodesFromSequencesInProgress.count(browserNode) > 0 || this->UpdateSequencesFromProxyNodesInProgress.count(browserNode) > 0)
  {
    // this update is due to updating from sequence nodes
    return;
  }
  if (browserNode->GetPlaybackActive())
  {
    // don't accept node modifications while replaying
    return;
  }
  if (!browserNode)
  {
    vtkErrorMacro("vtkSlicerSequencesLogic::UpdateSequencesFromProxyNodes failed: invalid browser node");
    return;
  }
  vtkMRMLSequenceNode* masterNode = browserNode->GetMasterSequenceNode();
  if (!masterNode)
  {
    vtkErrorMacro("Cannot record node modification: master sequence node is invalid");
    return;
  }

  if (!proxyNode)
  {
    vtkDebugMacro("vtkSlicerSequencesLogic::UpdateSequencesFromProxyNodes: update if all proxy nodes is not implemented yet");
    // TODO: update all proxy nodes if there is no info about which one was modified
    return;
  }

  this->UpdateSequencesFromProxyNodesInProgress.insert(browserNode);

  if (browserNode->GetRecordingActive())
  {
    // Record proxy node state into new sequence item
    // If we only record when the master node is modified, then skip if it was not the master node that was modified
    bool saveState = false;
    if (browserNode->GetRecordMasterOnly())
    {
      vtkMRMLNode* masterProxyNode = browserNode->GetProxyNode(masterNode);
      if (masterProxyNode != nullptr && masterProxyNode->GetID() != nullptr //
          && strcmp(proxyNode->GetID(), masterProxyNode->GetID()) == 0)
      {
        // master proxy node is changed
        saveState = true;
      }
    }
    else
    {
      // if we don't record on master node changes only then we always save the state
      saveState = true;
    }
    if (saveState)
    {
      browserNode->SaveProxyNodesState();
    }
  }
  else
  {
    // Update sequence item from proxy node
    vtkMRMLSequenceNode* sequenceNode = browserNode->GetSequenceNode(proxyNode);
    if (sequenceNode && browserNode->GetSelectedItemNumber() >= 0)
    {
      if (browserNode->GetSaveChanges(sequenceNode))
      {
        std::string indexValue = masterNode->GetNthIndexValue(browserNode->GetSelectedItemNumber());
        // If missing item mode is MissingItemSetToDefault then it means that the current time point
        // may not have a corresponding item in the sequence.
        // In this case, updating the closest item is only necessary when there is an
        // exact match; otherwise, it should be skipped with the proxy node
        // modifications reverted.
        bool itemMayNotBeInSequence = (browserNode->GetMissingItemMode(sequenceNode) == vtkMRMLSequenceBrowserNode::MissingItemSetToDefault);
        int closestItemNumber = sequenceNode->GetItemNumberFromIndexValue(indexValue, /* exactMatchRequired= */ itemMayNotBeInSequence);
        if (closestItemNumber >= 0)
        {
          std::string closestIndexValue = sequenceNode->GetNthIndexValue(closestItemNumber);
          sequenceNode->UpdateDataNodeAtValue(proxyNode, indexValue, true /* shallow copy*/);
        }
        else if (itemMayNotBeInSequence)
        {
          // This timepoint is not in the sequence, revert the modifications (reset the proxy node to default)
          // to make it clear to the user that changes are not saved (even though "SaveChanges" was enabled).
          vtkSmartPointer<vtkMRMLNode> emptyNode = vtkSmartPointer<vtkMRMLNode>::Take(proxyNode->CreateNodeInstance());
          proxyNode->CopyContent(emptyNode);
        }
      }
    }
  }
  this->UpdateSequencesFromProxyNodesInProgress.erase(browserNode);
}

//---------------------------------------------------------------------------
vtkMRMLSequenceNode* vtkSlicerSequencesLogic::AddSynchronizedNode(vtkMRMLNode* sNode, vtkMRMLNode* proxyNode, vtkMRMLNode* bNode)
{
  vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(sNode);
  vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(bNode);

  if (browserNode == nullptr)
  {
    vtkWarningMacro("vtkSlicerSequencesLogic::AddSynchronizedNode failed: browser node is invalid");
    return nullptr;
  }

  // Be consistent with removing synchronized sequence node - stop playback/recording
  browserNode->SetPlaybackActive(false);
  browserNode->SetRecordingActive(false);

  // Create an empty sequence node if the sequence node is nullptr
  if (sequenceNode == nullptr)
  {
    if (!this->GetMRMLScene())
    {
      vtkErrorMacro("Scene is invalid");
      return nullptr;
    }
    sequenceNode = vtkMRMLSequenceNode::SafeDownCast(this->GetMRMLScene()->CreateNodeByClass("vtkMRMLSequenceNode"));
    if (browserNode->GetMasterSequenceNode() != nullptr)
    {
      // Make the new sequence node compatible with the current master node
      sequenceNode->SetIndexName(browserNode->GetMasterSequenceNode()->GetIndexName());
      sequenceNode->SetIndexUnit(browserNode->GetMasterSequenceNode()->GetIndexUnit());
    }
    this->GetMRMLScene()->AddNode(sequenceNode);
    sequenceNode->Delete(); // Can release the pointer - ownership has been transferred to the scene
    if (proxyNode != nullptr)
    {
      // Generate sequence node name as "<proxyNodeName>-Sequence"
      std::stringstream sequenceNodeName;
      if (proxyNode->GetName())
      {
        sequenceNodeName << proxyNode->GetName();
      }
      else
      {
        sequenceNodeName << vtkMRMLTr("vtkSlicerSequencesLogic", "Unnamed");
      }
      sequenceNodeName << "-" << vtkMRMLTr("vtkSlicerSequencesLogic", "Sequence");
      sequenceNode->SetName(sequenceNodeName.str().c_str());
    }
  }

  // Check if the sequence node to add is compatible with the master
  if (browserNode->GetMasterSequenceNode() != nullptr //
      && !IsNodeCompatibleForBrowsing(browserNode->GetMasterSequenceNode(), sequenceNode))
  {
    vtkWarningMacro("vtkSlicerSequencesLogic::AddSynchronizedNode failed: incompatible index name or unit");
    return nullptr; // Not compatible - exit
  }

  if (!browserNode->IsSynchronizedSequenceNodeID(sequenceNode->GetID(), true))
  {
    browserNode->AddSynchronizedSequenceNodeID(sequenceNode->GetID());
  }
  if (proxyNode != nullptr)
  {
    browserNode->AddProxyNode(proxyNode, sequenceNode, false);
    vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(proxyNode);
    if (displayableNode)
    {
      displayableNode->CreateDefaultSequenceDisplayNodes();
    }
    vtkMRMLStorableNode* storableNode = vtkMRMLStorableNode::SafeDownCast(proxyNode);
    if (storableNode)
    {
      storableNode->AddDefaultStorageNode();
    }
  }
  return sequenceNode;
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData)
{
  vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(caller);
  if (browserNode == nullptr)
  {
    vtkErrorMacro("Expected a vtkMRMLSequenceBrowserNode");
    return;
  }
  if (event == vtkCommand::ModifiedEvent)
  {
    // Browser node modified (e.g., switched to next frame), update proxy nodes
    this->UpdateProxyNodesFromSequences(browserNode);
  }
  else if (event == vtkMRMLSequenceBrowserNode::ProxyNodeModifiedEvent)
  {
    // During import proxy node may change but we don't want to modify the sequence node with it
    // because the saved proxy node might be obsolete (for example, not saved when the scene was saved).
    // It might be useful to update all proxy nodes on SceneEndImport/Restore to make sure the state is consistent.
    if (this->GetMRMLScene() &&                 //
        !this->GetMRMLScene()->IsImporting() && //
        !this->GetMRMLScene()->IsRestoring())
    {
      // One of the proxy nodes changed, update the sequence as needed
      // If we wanted to change behavior of "save changes" to create a new time point when the proxy node changes
      // then we could add a flag to UpdateSequencesFromProxyNodes that would force creation of a new time point.
      this->UpdateSequencesFromProxyNodes(browserNode, vtkMRMLNode::SafeDownCast((vtkObject*)callData));
    }
  }
  else if (event == vtkMRMLSequenceBrowserNode::SequenceNodeModifiedEvent)
  {
    if (this->GetMRMLScene() &&                 //
        !this->GetMRMLScene()->IsImporting() && //
        !this->GetMRMLScene()->IsRestoring())
    {
      // One of the sequence nodes was modified, update the proxy nodes as needed
      // We currently update all proxy nodes, but it would be more efficient to only update the proxy node of the modified sequence node.
      // The modified sequence node is available as vtkMRMLSequenceNode::SafeDownCast((vtkObject*)callData), but it can be nullptr
      // if modifications were done between Start/EndModify.
      this->UpdateProxyNodesFromSequences(browserNode);
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::GetCompatibleNodesFromScene(vtkCollection* compatibleNodes, vtkMRMLSequenceNode* masterSequenceNode)
{
  if (compatibleNodes == nullptr)
  {
    vtkErrorMacro("vtkSlicerSequencesLogic::GetCompatibleNodesFromScene failed: compatibleNodes is invalid");
    return;
  }
  compatibleNodes->RemoveAllItems();
  if (masterSequenceNode == nullptr)
  {
    // if sequence node is invalid then there is no compatible node
    return;
  }
  if (this->GetMRMLScene() == nullptr)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  vtkSmartPointer<vtkCollection> sequenceNodes = vtkSmartPointer<vtkCollection>::Take(this->GetMRMLScene()->GetNodesByClass("vtkMRMLSequenceNode"));
  vtkObject* nextObject = nullptr;
  for (sequenceNodes->InitTraversal(); (nextObject = sequenceNodes->GetNextItemAsObject());)
  {
    vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(nextObject);
    if (sequenceNode == nullptr)
    {
      continue;
    }
    if (sequenceNode == masterSequenceNode)
    {
      // do not add the master node itself to the list of compatible nodes
      continue;
    }
    if (IsNodeCompatibleForBrowsing(masterSequenceNode, sequenceNode))
    {
      compatibleNodes->AddItem(sequenceNode);
    }
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerSequencesLogic::IsNodeCompatibleForBrowsing(vtkMRMLSequenceNode* masterNode, vtkMRMLSequenceNode* testedNode)
{
  bool compatible = (masterNode->GetIndexName() == testedNode->GetIndexName()    //
                     && masterNode->GetIndexUnit() == testedNode->GetIndexUnit() //
                     && masterNode->GetIndexType() == testedNode->GetIndexType());
  return compatible;
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::GetBrowserNodesForSequenceNode(vtkMRMLSequenceNode* sequenceNode, vtkCollection* foundBrowserNodes)
{
  if (this->GetMRMLScene() == nullptr)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  if (foundBrowserNodes == nullptr)
  {
    vtkErrorMacro("foundBrowserNodes is invalid");
    return;
  }
  foundBrowserNodes->RemoveAllItems();
  std::vector<vtkMRMLNode*> browserNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLSequenceBrowserNode", browserNodes);
  for (std::vector<vtkMRMLNode*>::iterator browserNodeIt = browserNodes.begin(); browserNodeIt != browserNodes.end(); ++browserNodeIt)
  {
    vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(*browserNodeIt);
    if (browserNode->IsSynchronizedSequenceNode(sequenceNode, true))
    {
      foundBrowserNodes->AddItem(browserNode);
    }
  }
}

;

//---------------------------------------------------------------------------
vtkMRMLSequenceBrowserNode* vtkSlicerSequencesLogic::GetFirstBrowserNodeForSequenceNode(vtkMRMLSequenceNode* sequenceNode)
{
  if (this->GetMRMLScene() == nullptr)
  {
    vtkErrorMacro("Scene is invalid");
    return nullptr;
  }
  std::vector<vtkMRMLNode*> browserNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLSequenceBrowserNode", browserNodes);
  for (std::vector<vtkMRMLNode*>::iterator browserNodeIt = browserNodes.begin(); browserNodeIt != browserNodes.end(); ++browserNodeIt)
  {
    vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(*browserNodeIt);
    if (browserNode->IsSynchronizedSequenceNode(sequenceNode, true))
    {
      return browserNode;
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::GetBrowserNodesForProxyNode(vtkMRMLNode* proxyNode, vtkCollection* foundBrowserNodes)
{
  if (foundBrowserNodes == nullptr)
  {
    vtkErrorMacro("foundBrowserNodes is invalid");
    return;
  }
  foundBrowserNodes->RemoveAllItems();
  if (this->GetMRMLScene() == nullptr)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  if (proxyNode == nullptr || proxyNode->GetID() == nullptr)
  {
    vtkErrorMacro("proxyNode is invalid");
    return;
  }
  std::vector<vtkMRMLNode*> browserNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLSequenceBrowserNode", browserNodes);
  for (std::vector<vtkMRMLNode*>::iterator browserNodeIt = browserNodes.begin(); browserNodeIt != browserNodes.end(); ++browserNodeIt)
  {
    vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(*browserNodeIt);
    if (browserNode->IsProxyNodeID(proxyNode->GetID()))
    {
      foundBrowserNodes->AddItem(browserNode);
    }
  }
}

//---------------------------------------------------------------------------
vtkMRMLSequenceBrowserNode* vtkSlicerSequencesLogic::GetFirstBrowserNodeForProxyNode(vtkMRMLNode* proxyNode)
{
  if (this->GetMRMLScene() == nullptr)
  {
    vtkErrorMacro("Scene is invalid");
    return nullptr;
  }
  if (proxyNode == nullptr || proxyNode->GetID() == nullptr)
  {
    vtkErrorMacro("proxyNode is invalid");
    return nullptr;
  }
  std::vector<vtkMRMLNode*> browserNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLSequenceBrowserNode", browserNodes);
  for (std::vector<vtkMRMLNode*>::iterator browserNodeIt = browserNodes.begin(); browserNodeIt != browserNodes.end(); ++browserNodeIt)
  {
    vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(*browserNodeIt);
    if (browserNode->IsProxyNodeID(proxyNode->GetID()))
    {
      return browserNode;
    }
  }
  return nullptr;
}
