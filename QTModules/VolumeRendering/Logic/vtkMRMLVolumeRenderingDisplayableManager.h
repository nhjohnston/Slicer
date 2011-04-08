#ifndef __vtkMRMLVolumeRenderingDisplayableManager_h
#define __vtkMRMLVolumeRenderingDisplayableManager_h

#include "vtkSlicerModuleLogic.h"
#include "VolumeRenderingLogicExport.h"
#include "vtkVolumeMapper.h"
#include "vtkVolume.h"

#include <vtkMRMLAbstractThreeDViewDisplayableManager.h>

#include "vtkMRMLVolumeRenderingParametersNode.h"
#include "vtkMRMLVolumeRenderingScenarioNode.h"
#include "vtkSlicerVolumeRenderingLogic.h"

class vtkSlicerVolumeTextureMapper3D;
class vtkSlicerFixedPointVolumeRayCastMapper;
class vtkSlicerGPURayCastVolumeMapper;
class vtkSlicerGPURayCastMultiVolumeMapper;
class vtkGPUVolumeRayCastMapper;

class vtkTimerLog;
class vtkMatrix4x4;
class vtkPlanes;

/// \ingroup Slicer_QtModules_VolumeRendering
class Q_SLICER_QTMODULES_VOLUMERENDERING_LOGIC_EXPORT vtkMRMLVolumeRenderingDisplayableManager  :
  public vtkMRMLAbstractThreeDViewDisplayableManager
{
public:
  static vtkMRMLVolumeRenderingDisplayableManager *New();
  vtkTypeRevisionMacro(vtkMRMLVolumeRenderingDisplayableManager, vtkMRMLAbstractThreeDViewDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent);

  // TODO: do we need to observe MRML here?
  //virtual void ProcessMrmlEvents ( vtkObject *caller, unsigned long event,
    //                               void *callData ){};


  void Reset();

  void SetGUICallbackCommand(vtkCommand* callback);

  virtual void Create();
  
  // Description:
  // Get, Set and Observe VolumeRenderingParametersNode
  vtkGetObjectMacro (VolumeRenderingParametersNode, vtkMRMLVolumeRenderingParametersNode);
  void SetAndObserveVolumeRenderingParametersNode(vtkMRMLVolumeRenderingParametersNode* vspNode);

  // Description:
  // Get, Set and Observe VolumeNode
  vtkGetObjectMacro (VolumeNode, vtkMRMLVolumeNode);
  void SetAndObserveVolumeNode(vtkMRMLVolumeNode* vspNode);

  // Description:
  // Get, Set and Observe FgVolumeNode
  vtkGetObjectMacro (FgVolumeNode, vtkMRMLVolumeNode);
  void SetAndObserveFgVolumeNode(vtkMRMLVolumeNode* vspNode);

  // Description:
  // Get, Set and Observe VolumePropertyNode
  vtkGetObjectMacro (VolumePropertyNode, vtkMRMLVolumePropertyNode);
  void SetAndObserveVolumePropertyNode(vtkMRMLVolumePropertyNode* vspNode);

  // Description:
  // Get, Set and Observe FgVolumePropertyNode
  vtkGetObjectMacro (FgVolumePropertyNode, vtkMRMLVolumePropertyNode);
  void SetAndObserveFgVolumePropertyNode(vtkMRMLVolumePropertyNode* vspNode);

  // Description:
  // Get, Set and Observe ROINode
  vtkGetObjectMacro (ROINode, vtkMRMLROINode);
  void SetAndObserveROINode(vtkMRMLROINode* vspNode);


  // Description:
  // setup mapper based on current parameters node
  // return values:
  // -1: requested mapper not supported
  //  0: invalid input parameter
  //  1: success
  int SetupMapperFromParametersNode(vtkMRMLVolumeRenderingParametersNode* vspNode);


  // Description:
  // Update MRML events
  virtual void ProcessMRMLEvents ( vtkObject * /*caller*/,
                                  unsigned long /*event*/,
                                  void * /*callData*/ );

  // Description:
  // Get Volume Actor
  vtkVolume* GetVolumeActor(){return this->Volume;}

  void SetupHistograms(vtkMRMLVolumeRenderingParametersNode* vspNode);
  //vtkKWHistogramSet* GetHistogramSet(){return this->Histograms;}

  void SetupHistogramsFg(vtkMRMLVolumeRenderingParametersNode* vspNode);
  //vtkKWHistogramSet* GetHistogramSetFg(){return this->HistogramsFg;}

  void SetExpectedFPS(vtkMRMLVolumeRenderingParametersNode* vspNode);
  void SetGPUMemorySize(vtkMRMLVolumeRenderingParametersNode* vspNode);
  void SetCPURaycastParameters(vtkMRMLVolumeRenderingParametersNode* vspNode);
  void SetGPURaycastParameters(vtkMRMLVolumeRenderingParametersNode* vspNode);
  void SetGPURaycastIIParameters(vtkMRMLVolumeRenderingParametersNode* vspNode);
  void SetGPURaycast3Parameters(vtkMRMLVolumeRenderingParametersNode* vspNode);
  void SetROI(vtkMRMLVolumeRenderingParametersNode* vspNode);

  void CreateVolumePropertyGPURaycastII(vtkMRMLVolumeRenderingParametersNode* vspNode);
  void UpdateVolumePropertyGPURaycastII(vtkMRMLVolumeRenderingParametersNode* vspNode);

  //void CreateVolumePropertyGPURaycast3(vtkMRMLVolumeRenderingParametersNode* vspNode);
  //void UpdateVolumePropertyGPURaycast3(vtkMRMLVolumeRenderingParametersNode* vspNode);

  void TransformModified(vtkMRMLVolumeRenderingParametersNode* vspNode);

  void SetVolumeVisibility(int isVisible);

  /* return values:
   * 0: cpu ray cast not used
   * 1: success
   */
  int SetupVolumeRenderingInteractive(vtkMRMLVolumeRenderingParametersNode* vspNode, int buttonDown);

  /* return values:
   * 0: mapper not supported
   * 1: mapper supported
   */
  int IsCurrentMapperSupported(vtkMRMLVolumeRenderingParametersNode* vspNode);

  
protected:
  vtkMRMLVolumeRenderingDisplayableManager();
  ~vtkMRMLVolumeRenderingDisplayableManager();
  vtkMRMLVolumeRenderingDisplayableManager(const vtkMRMLVolumeRenderingDisplayableManager&);
  void operator=(const vtkMRMLVolumeRenderingDisplayableManager&);

  //virtual void OnMRMLSceneNodeAddedEvent(vtkMRMLNode* node);

  //virtual void OnMRMLSceneNodeRemovedEvent(vtkMRMLNode* node);

  void OnCreate();

  static bool First;

  vtkSlicerVolumeRenderingLogic *VolumeRenderingLogic;
  // Description:
  // The hardware accelerated texture mapper.
  vtkSlicerVolumeTextureMapper3D *MapperTexture;

  // Description:
  // The hardware accelerated gpu ray cast mapper.
  vtkSlicerGPURayCastVolumeMapper *MapperGPURaycast;

  // Description:
  // The hardware accelerated multi-volume gpu ray cast mapper.
  vtkSlicerGPURayCastMultiVolumeMapper *MapperGPURaycastII;

  // Description:
  // The software accelerated software mapper
  vtkSlicerFixedPointVolumeRayCastMapper *MapperRaycast;

  // Description:
  // The gpu ray cast mapper.
  vtkGPUVolumeRayCastMapper *MapperGPURaycast3;

  // Description:
  // Actor used for Volume Rendering
  vtkVolume *Volume;

  // Description:
  // internal histogram instance (bg)
  //vtkKWHistogramSet *Histograms;

  // Description:
  // internal histogram instance (fg)
  //vtkKWHistogramSet *HistogramsFg;

  /// Holders for MRML callbacks
  //vtkCallbackCommand *MRMLCallback;


  vtkVolumeProperty *VolumePropertyGPURaycastII;

  //vtkVolumeProperty *VolumePropertyGPURaycast3;

  vtkMRMLVolumeRenderingParametersNode* VolumeRenderingParametersNode;
  vtkMRMLVolumeRenderingScenarioNode*   ScenarioNode;
  vtkMRMLViewNode*                      ViewNode;

  vtkMRMLVolumeNode*          VolumeNode;
  vtkMRMLVolumePropertyNode*  VolumePropertyNode;
  vtkMRMLVolumeNode*          FgVolumeNode;
  vtkMRMLVolumePropertyNode*  FgVolumePropertyNode;
  vtkMRMLROINode*             ROINode;

  int SceneIsLoadingFlag;
  int ProcessingMRMLFlag;

protected:
  void OnScenarioNodeModified();
  void OnViewNodeModified();
  void OnVolumeRenderingParameterNodeModified();

  void ComputeInternalVolumeSize(int index);
  void CalculateMatrix(vtkMRMLVolumeRenderingParametersNode *vspNode, vtkMatrix4x4 *output);
  void EstimateSampleDistance(vtkMRMLVolumeRenderingParametersNode* vspNode);
  vtkMRMLVolumeRenderingParametersNode* GetCurrentParametersNode();
  void RemoveVolumeFromViewers();
  void AddVolumeToViewers();
  void InitializePipelineFromParametersNode();
  int ValidateParametersNode(vtkMRMLVolumeRenderingParametersNode* vspNode);

};

#endif
