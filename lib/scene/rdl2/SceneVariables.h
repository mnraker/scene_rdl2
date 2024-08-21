// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0


#pragma once

// Include this before any other includes!
#include <scene_rdl2/common/platform/Platform.h>

#include "AttributeKey.h"
#include "SceneClass.h"
#include "SceneObject.h"
#include "Types.h"
#include <scene_rdl2/common/math/Viewport.h>

#include <string>

namespace scene_rdl2 {
namespace rdl2 {

enum class PixelFilterType
{
    box = 0,
    cubicBSpline = 1,
    quadraticBSpline = 2
};

enum class TaskDistributionType : int
{
    NON_OVERLAPPED_TILE = 0,
    MULTIPLEX_PIXEL = 1
};

enum class VolumeOverlapMode
{
    SUM = 0,
    MAX = 1,
    RND = 2
};

enum class ShadowTerminatorFix
{
    OFF = 0,
    CUSTOM = 1,
    SINE_COMPENSATION = 2,
    GGX = 3,
    COSINE_COMPENSATION = 4
};

/**
 * The SceneVariables are a SceneObject which contain render globals. This
 * object is created by the SceneContext when it is constructed, and the context
 * enforces that no additional SceneVariables objects are created (it's a
 * singleton within the SceneContext). Its name is "__SceneVariables__", but
 * you don't need to remember that because you can access the object directly
 * from the SceneContext.
 *
 * Thread Safety:
 *  - The guarantees are exactly the same as any other SceneObject. There is
 *      no sychronization from RDL on accessing or modifying SceneObjects once
 *      you get the pointer back from the SceneContext.
 *  - During rendering, accessing the SceneVariables from multiple threads is
 *      safe because the whole context is const and nobody is updating it.
 *      In areas where the context is not const (outside the render loop),
 *      synchronization is up to you.
 */
class SceneVariables : public SceneObject
{
public:
    typedef SceneObject Parent;

    SceneVariables(const SceneClass& sceneClass, const std::string& name);
    static SceneObjectInterface declare(SceneClass& sceneClass);

    /// Retrieves the region window width (AFTER applying the resolution divisor)
    /// in pixels. The higher level render buffers are this width.
    uint32_t getRezedWidth() const;

    /// Retrieves the region window height (AFTER applying the resolution divisor)
    /// in pixels. The higher level render buffers are this height.
    uint32_t getRezedHeight() const;

    /// The camera is mapped to this window. It is defined in pixel space
    math::HalfOpenViewport getRezedApertureWindow() const;

    /// A pixel is rendered for every point in this window. It is defined in pixel space
    math::HalfOpenViewport getRezedRegionWindow() const;

    // Defined relative to the region window and clipped to the region window.
    math::HalfOpenViewport getRezedSubViewport() const;

    /// Get the machine ID. Machine IDs must be >= 0 and < numMachines.
    int getMachineId() const;

    /// Get the number of machines in the cluster. If not rendering in a
    /// cluster, this is 1.
    int getNumMachines() const;

    /// Retrieves the active RDL layer object we're rendering from. Returns NULL
    /// if no layer could be found.
    SceneObject* getLayer() const;

    /// Retrieves the active RDL camera object we're rendering from. Returns NULL
    /// if no camera could be found.
    SceneObject* getCamera() const;

    /// Retrieves metadata for image output. Returns NULL if there is no metadata.
    const SceneObject* getExrHeaderAttributes() const;

    /// Get the pixel to debug. The get call also returns whether or not
    /// the debug pixel was set (false by default).
    /// The debug pixel is expressed in rezed / region window coordinates
    /// The debug pixel is initialized to an invalid value. If it has not been
    /// set to something else, the getter will return false. Therefore, the
    /// return boolean should be checked by the caller.
    bool getDebugPixel(math::Vec2i &pixel) const;
    /// Get start and end ray to debug, inclusive.
    /// The debug rays primary range is initialized to an invalid value.
    /// If it has not been set to something else, the getter will
    /// return false. Therefore, the return boolean should be checked by the caller.
    bool getDebugRaysPrimaryRange(int &start, int &end) const;
    /// Get start and end ray depth debug, inclusive.
    /// The debug rays depth range is initialized to an invalid value.
    /// If it has not been set to something else, the getter will
    /// return false. Therefore, the return boolean should be checked by the caller.
    bool getDebugRaysDepthRange(int &start, int &end) const;

    /// Get sub-viewport. We don't render pixels outside of this viewport.
    /// Max x and y coordinates are inclusive, i.e. we render them.
    /// The sub-viewport is expressed in rezed / frame-viewport coordinates.
    /// The subviewport is initialized to an invalid value. If it has not been
    /// set to something else, the getter will return false. Therefore, the
    /// return boolean should be checked by the caller.
    bool getSubViewport(math::HalfOpenViewport& viewport) const;

    void disableSubViewport();

    /// Return temporary directory path name
    std::string getTmpDir() const;

    //
    // Frame
    //

    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sMinFrameKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sMaxFrameKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sFrameKey;

    //
    // Camera and Layer
    //

    // The primary RDL camera object we're rendering from.
    RDL2_DSO_BUILTIN_API static AttributeKey<SceneObject*> sCamera;
    // Dicing RDL camera
    RDL2_DSO_BUILTIN_API static AttributeKey<SceneObject*> sDicingCamera;
    // The active RDL layer object we're rendering from.
    RDL2_DSO_BUILTIN_API static AttributeKey<SceneObject*> sLayer;

    //
    // Exr Header Attributes
    //

    RDL2_DSO_BUILTIN_API static AttributeKey<SceneObject*> sAttrExrHeaderAttributes;

    //
    // Image Size
    //

    // Canonical frame width (BEFORE applying the resolution divisor or viewport), in pixels.
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sImageWidth;
    // Canonical frame height (BEFORE applying the resolution divisor or viewport), in pixels.
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sImageHeight;
    // The resolution divisor.
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sResKey;

    // See http://jira.anim.dreamworks.com/browse/MOONRAY-1999 for a detailed description
    // of the exact definition of aperture and region window.
    RDL2_DSO_BUILTIN_API static AttributeKey<IntVector> sApertureWindow;
    RDL2_DSO_BUILTIN_API static AttributeKey<IntVector> sRegionWindow;

    // The sub-viewport. We don't render pixels outside of this viewport.
    RDL2_DSO_BUILTIN_API static AttributeKey<IntVector> sSubViewport;

    //
    // Motion and Scale
    //

    RDL2_DSO_BUILTIN_API static AttributeKey<FloatVector> sMotionSteps;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sSlerpXforms;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sFpsKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sSceneScaleKey;

    //
    // Sampling
    //

    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sSamplingMode;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMinAdaptiveSamples;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMaxAdaptiveSamples;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sTargetAdaptiveError;

    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sLightSamplingMode;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sLightSamplingQuality;

    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sPixelSamplesSqrt;     // Traditional non-adaptive sampling sample count.
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sLightSamplesSqrt;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sBsdfSamplesSqrt;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sBssrdfSamplesSqrt;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMaxDepth;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMaxDiffuseDepth;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMaxGlossyDepth;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMaxMirrorDepth;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMaxVolumeDepth;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMaxPresenceDepth;
    // Note: hair material has glossy lobes. So the max depth
    // for hair materials is actually max(sMaxGlossyDepth, sMaxHairDepth)
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMaxHairDepth;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sDisableOptimizedHairSampling;

    // The following is a control for max subsurface evaluations
    // after which it switches to a diffuse approximation
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMaxSubsurfacePerPath;

    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sTransparencyThreshold;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sPresenceThreshold;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sPresenceQuality;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sRussianRouletteThreshold;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sLockFrameNoise;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sVolumeQuality;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sVolumeShadowQuality;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sVolumeIlluminationSamples;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sVolumeOpacityThreshold;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sVolumeOverlapMode;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int>  sVolumeIndirectSamples;

    //
    // Volume Multiple Scattering coefficient
    //
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sVolumeAttenuationFactor;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sVolumeContributionFactor;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sVolumePhaseAttenuationFactor;

    //
    // Fireflies removal
    //

    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sSampleClampingValue;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sSampleClampingDepth;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sRoughnessClampingFactor;

    //
    // Filtering
    //

    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sTextureBlur;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sPixelFilterWidth;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sPixelFilterType;

    //
    // Deep file output
    //

    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sDeepFormat;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sDeepCurvatureTolerance;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float> sDeepZTolerance;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sDeepVolCompressionRes;
    RDL2_DSO_BUILTIN_API static AttributeKey<StringVector> sDeepIDAttributeNames;

    RDL2_DSO_BUILTIN_API static AttributeKey<String> sCryptoUVAttributeName;

    //
    // Caching
    //

    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sTextureCacheSizeMb;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sTextureFileHandleCount;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sFastGeomUpdate;

    //
    // Checkpoint render
    //
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool>   sCheckpointActive; // The toggle for checkpoint render
    RDL2_DSO_BUILTIN_API static AttributeKey<Float>  sCheckpointInterval; // Unit is minute
    RDL2_DSO_BUILTIN_API static AttributeKey<Int>    sCheckpointQualitySteps;
    RDL2_DSO_BUILTIN_API static AttributeKey<Float>  sCheckpointTimeCap; // Unit is minute
    RDL2_DSO_BUILTIN_API static AttributeKey<Int>    sCheckpointSampleCap;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool>   sCheckpointOverwrite;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int>    sCheckpointMode;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int>    sCheckpointStartSPP; // Samples per pixel
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool>   sCheckpointBgWrite;
    RDL2_DSO_BUILTIN_API static AttributeKey<String> sCheckpointPostScript; // Post checkpoint lua script name
    RDL2_DSO_BUILTIN_API static AttributeKey<Int>    sCheckpointTotalFiles; // for quality based checkpoint mode
    RDL2_DSO_BUILTIN_API static AttributeKey<Int>    sCheckpointMaxBgCache; // for sCheckpointBgWrite = true
    RDL2_DSO_BUILTIN_API static AttributeKey<Float>  sCheckpointMaxSnapshotOverhead; // max threshold fraction of snapshot overhead
    RDL2_DSO_BUILTIN_API static AttributeKey<Float>  sCheckpointSnapshotInterval; // Unit is minute

    //
    // Resume render
    //
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool>   sResumableOutput;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool>   sResumeRender;
    RDL2_DSO_BUILTIN_API static AttributeKey<String> sOnResumeScript; // on resume lua script name

    //
    // Global overriding toggles
    //

    // The toggle for camera motion blur
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sEnableMotionBlur;
    // The toggle for camera depth of field
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sEnableDof;
    // The toggle for limiting the max subdivision
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sEnableMaxGeomResolution;
    // Max subdivision limit
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMaxGeomResolution;
    // The toggle for displacement map
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sEnableDisplacement;
    // The toggle for subsurface scattering
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sEnableSSS;
    // The toggle for shadow
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sEnableShadowing;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sEnablePresenceShadows;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sLightsVisibleInCameraKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sPropagateVisibilityBounceType;
    RDL2_DSO_BUILTIN_API static AttributeKey<Int>  sShadowTerminatorFix;

    //
    // Driver
    //

    // The machine ID. Machine IDs must be >= 0 and < numMachines.
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sMachineId;

    // The number of machines in the cluster. If not rendering in a cluster, this is 1.
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sNumMachines;

    // Task distribution type for multi-machine context.
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sTaskDistributionType;

    // Batch/Realime mode tile scheduling pattern.
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sBatchTileOrder;

    // Progressive mode tile scheduling pattern.
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sProgressiveTileOrder;

    // Checkpoint mode tile scheduling pattern.
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sCheckpointTileOrder;

    // The output image file path.
    RDL2_DSO_BUILTIN_API static AttributeKey<String> sOutputFile;

    // Temporary directory
    RDL2_DSO_BUILTIN_API static AttributeKey<String> sTemporaryDirectory;

    // The aov to use as the primary output
    static AttributeKey<SceneObject*> sPrimaryAov;

    // File output logic
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sTwoStageOutput;

    //
    // Logging
    //

    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sDebugKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sInfoKey;
    RDL2_DSO_BUILTIN_API static AttributeKey<Rgb> sFatalColor;
    RDL2_DSO_BUILTIN_API static AttributeKey<Vec3f> sFatalNormal;
    // The statsfile file path.
    RDL2_DSO_BUILTIN_API static AttributeKey<String> sStatsFile;

    // Athena Data Collection
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sAthenaDebug;

    //
    // Debug
    //

    // The pixel to debug, expressed in rezed / frame-viewport coordinates.
    RDL2_DSO_BUILTIN_API static AttributeKey<IntVector> sDebugPixel;
    // The debug rays output file path.
    RDL2_DSO_BUILTIN_API static AttributeKey<String> sDebugRaysFile;
    // Start and end ray to debug, inclusive.
    RDL2_DSO_BUILTIN_API static AttributeKey<IntVector> sDebugRaysPrimaryRange;
    // Start and end ray depth to debug, inclusive.
    RDL2_DSO_BUILTIN_API static AttributeKey<IntVector> sDebugRaysDepthRange;

    // Debug console
    RDL2_DSO_BUILTIN_API static AttributeKey<Int> sDebugConsole;

    // Geometry validation
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sValidateGeometry;

    // capture multiple layers of presence for cryptomatte
    RDL2_DSO_BUILTIN_API static AttributeKey<Bool> sCryptomatteMultiPresence;
};

} // namespace rdl2
} // namespace scene_rdl2

