/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_ENGINE_NODE_H
#define NATRON_ENGINE_NODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <string>
#include <map>
#include <list>
#include <bitset>

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMetaType>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QMutex>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif
#include "Engine/AppManager.h"
#include "Global/KeySymbols.h"
#include "Engine/ChoiceOption.h"
#include "Engine/DimensionIdx.h"
#include "Engine/ImagePlaneDesc.h"
#include "Serialization/SerializationBase.h"
#include "Engine/ViewIdx.h"
#include "Engine/Markdown.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

#define kNodePageParamName "nodePage"
#define kNodePageParamLabel "Node"
#define kInfoPageParamName "infoPage"
#define kInfoPageParamLabel "Info"
#define kPyPlugPageParamName "pyPlugPage"
#define kPyPlugPageParamLabel "PyPlug"

#define kDisableNodeKnobName "disableNode"
#define kLifeTimeNodeKnobName "nodeLifeTime"
#define kEnableLifeTimeNodeKnobName "enableNodeLifeTime"
#define kUserLabelKnobName "userTextArea"
#define kEnableMaskKnobName "enableMask"
#define kEnableInputKnobName "enableInput"
#define kMaskChannelKnobName "maskChannel"
#define kInputChannelKnobName "inputChannel"
#define kEnablePreviewKnobName "enablePreview"
#define kOutputChannelsKnobName "channels"

#define kHostMixingKnobName "hostMix"
#define kHostMixingKnobLabel "Mix"
#define kHostMixingKnobHint "Mix between the source image at 0 and the full effect at 1"

#define kOfxMaskInvertParamName "maskInvert"
#define kOfxMixParamName "mix"

#define kReadOIIOAvailableViewsKnobName "availableViews"
#define kWriteOIIOParamViewsSelector "viewsSelector"

#define kNatronNodeKnobExportPyPlugGroup "exportPyPlugDialog"
#define kNatronNodeKnobExportPyPlugGroupLabel "Export"

#define kNatronNodeKnobExportPyPlugButton "exportPyPlug"
#define kNatronNodeKnobExportPyPlugButtonLabel "Export"

#define kNatronNodeKnobConvertToGroupButton "convertToGroup"
#define kNatronNodeKnobConvertToGroupButtonLabel "Convert to Group"

#define kNatronNodeKnobPyPlugPluginID "pyPlugPluginID"
#define kNatronNodeKnobPyPlugPluginIDLabel "PyPlug ID"
#define kNatronNodeKnobPyPlugPluginIDHint "When exporting a group to PyPlug, this will be the plug-in ID of the PyPlug.\n" \
"Generally, this contains domain and sub-domains names " \
"such as fr.inria.group.XXX.\n" \
"If two plug-ins or more happen to have the same ID, they will be " \
"gathered by version.\n" \
"If two plug-ins or more have the same ID and version, the first loaded in the" \
" search-paths will take precedence over the other."

#define kNatronNodeKnobPyPlugPluginLabel "pyPlugPluginLabel"
#define kNatronNodeKnobPyPlugPluginLabelLabel "PyPlug Label"
#define kNatronNodeKnobPyPlugPluginLabelHint "When exporting a group to PyPlug, this will be the plug-in label as visible in the GUI of the PyPlug"

#define kNatronNodeKnobPyPlugPluginGrouping "pyPlugPluginGrouping"
#define kNatronNodeKnobPyPlugPluginGroupingLabel "PyPlug Grouping"
#define kNatronNodeKnobPyPlugPluginGroupingHint "When exporting a group to PyPlug, this will be the grouping of the PyPlug, that is the menu under which it should be located. For example: \"Color/MyPyPlugs\". Each sub-level must be separated by a '/' character"

#define kNatronNodeKnobPyPlugPluginIconFile "pyPlugPluginIcon"
#define kNatronNodeKnobPyPlugPluginIconFileLabel "PyPlug Icon"
#define kNatronNodeKnobPyPlugPluginIconFileHint "When exporting a group to PyPlug, this parameter indicates the filename of a PNG file of the icon to be used for this plug-in. The filename should be relative to the directory containing the PyPlug script"

#define kNatronNodeKnobPyPlugPluginDescription "pyPlugPluginDescription"
#define kNatronNodeKnobPyPlugPluginDescriptionLabel "PyPlug Description"
#define kNatronNodeKnobPyPlugPluginDescriptionHint "When exporting a group to PyPlug, this will be the documentation of the PyPlug"

#define kNatronNodeKnobPyPlugPluginDescriptionIsMarkdown "pyPlugPluginDescriptionIsMarkdown"
#define kNatronNodeKnobPyPlugPluginDescriptionIsMarkdownLabel "Markdown"
#define kNatronNodeKnobPyPlugPluginDescriptionIsMarkdownHint "Indicates whether the PyPlug description is encoded in Markdown or not. This is helpful to use rich text capabilities for the documentation"

#define kNatronNodeKnobPyPlugPluginVersion "pyPlugPluginVersion"
#define kNatronNodeKnobPyPlugPluginVersionLabel "PyPlug Version"
#define kNatronNodeKnobPyPlugPluginVersionHint "When exporting a group to PyPlug, this will be the version of the PyPlug. This is useful to indicate users it has changed"

#define kNatronNodeKnobPyPlugPluginCallbacksPythonScript "pyPlugCallbacksPythonScript"
#define kNatronNodeKnobPyPlugPluginCallbacksPythonScriptLabel "Callback(s) Python script"
#define kNatronNodeKnobPyPlugPluginCallbacksPythonScriptHint "When exporting a group to PyPlug, this parameter indicates the filename of a Python script where callbacks used by this PyPlug are defined. The filename should be relative to the directory containing the PyPlug script"

#define kNatronNodeKnobPyPlugPluginShortcut "pyPlugPluginShortcut"
#define kNatronNodeKnobPyPlugPluginShortcutLabel "PyPlug Shortcut"
#define kNatronNodeKnobPyPlugPluginShortcutHint "When exporting a group to PyPlug, this will be the keyboard shortcut by default the user can use to create the PyPlug. The user can always change it later on in the Preferences/Shortcut Editor"

#define kNatronNodeKnobExportDialogFilePath "exportFilePath"
#define kNatronNodeKnobExportDialogFilePathLabel "File"
#define kNatronNodeKnobExportDialogFilePathHint "The file where to write"

#define kNatronNodeKnobExportDialogOkButton "exportDialogOkButton"
#define kNatronNodeKnobExportDialogOkButtonLabel "Ok"

#define kNatronNodeKnobExportDialogCancelButton "exportDialogCancelButton"
#define kNatronNodeKnobExportDialogCancelButtonLabel "Cancel"

#define kNatronNodeKnobKeepInAnimationModuleButton "keepInAnimationModuleButton"
#define kNatronNodeKnobKeepInAnimationModuleButtonLabel "Keep In Animation Module"
#define kNatronNodeKnobKeepInAnimationModuleButtonHint "When checked, this node will always be visible in the Animation Module regardless of whether its settings panel is opened or not"

struct NodePrivate;
class Node
    : public QObject
    , public boost::enable_shared_from_this<Node>
    , public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON


protected: 
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    Node(const AppInstancePtr& app,
         const NodeCollectionPtr& group,
         const PluginPtr& plugin);

public:
    static NodePtr create(const AppInstancePtr& app,
                          const NodeCollectionPtr& group,
                          const PluginPtr& plugin)
    {
        return NodePtr( new Node(app, group, plugin) );
    }

    virtual ~Node();

    NodeCollectionPtr getGroup() const;

    /**
     * @brief Returns true if this node is a "user" node. For internal invisible node, this would return false.
     * If this function returns false, the node will not be serialized.
     **/
    bool isPersistent() const;

    // returns the plug-in currently instanciated in the node
    PluginPtr getPlugin() const;

    // For pyplugs, returns the handle to the pyplug
    PluginPtr getPyPlugPlugin() const;

    // For groups which may be pyplugs, return the handle of the group plugin
    PluginPtr getOriginalPlugin() const;


    void setPrecompNode(const PrecompNodePtr& precomp);

    //
    PrecompNodePtr isPartOfPrecomp() const;

    /**
     * @brief Creates the EffectInstance that will be embedded into this node and set it up.
     * This function also loads all parameters. Node connections will not be setup in this method.
     **/
    void load(const CreateNodeArgsPtr& args);


    void initNodeScriptName(const SERIALIZATION_NAMESPACE::NodeSerialization* serialization, const QString& fixedName);


    void loadKnob(const KnobIPtr & knob, const std::list<SERIALIZATION_NAMESPACE::KnobSerializationPtr> & serialization);

    /**
     * @brief Links all the evaluateOnChange knobs to the other one except
     * trigger buttons. The other node must be the same plug-in
     **/
    bool linkToNode(const NodePtr& other);

    /**
     * @brief Unlink all knobs of the node.
     **/
    void unlinkAllKnobs();

    /**
     * @brief Get a list of all nodes that are linked to this one.
     * For each node in return a boolean indicates whether the link is a clone link
     * or a just a regular link.
     * A node is considered clone if all its evaluate on change knobs are linked.
     **/
    void getLinkedNodes(std::list<std::pair<NodePtr, bool> >* nodes) const;

    /**
     * @brief Wrapper around getLinkedNodes() that returns cloned nodes
     **/
    void getCloneLinkedNodes(std::list<NodePtr>* clones) const;

    /**
     * @brief Move this node to the given group
     **/
    void moveToGroup(const NodeCollectionPtr& group);

private:

    void initNodeNameFallbackOnPluginDefault();

    void createNodeGuiInternal(const CreateNodeArgsPtr& args);

  

    void restoreUserKnob(const KnobGroupPtr& group,
                         const KnobPagePtr& page,
                         const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase,
                         unsigned int recursionLevel);

public:


    /**
     * @brief Implement to save the content of the object to the serialization object
     **/
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE FINAL;

    /**
     * @brief Implement to load the content of the serialization object onto this object
     **/
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE FINAL;

    void loadInternalNodeGraph(bool initialSetupAllowed,
                               const SERIALIZATION_NAMESPACE::NodeSerialization* projectSerialization,
                               const SERIALIZATION_NAMESPACE::NodeSerialization* pyPlugSerialization);

    void loadKnobsFromSerialization(const SERIALIZATION_NAMESPACE::NodeSerialization& serialization);

    void getNodeSerializationFromPresetFile(const std::string& presetFile, SERIALIZATION_NAMESPACE::NodeSerialization* serialization);

    void getNodeSerializationFromPresetName(const std::string& presetName, SERIALIZATION_NAMESPACE::NodeSerialization* serialization);


private:


    void loadPresetsInternal(const SERIALIZATION_NAMESPACE::NodeSerializationPtr& serialization, bool setKnobsDefault);

public:

    void refreshDefaultPagesOrder();

    /**
     * @brief Setup the node state according to the presets file.
     * This function throws exception in case of error.
     **/
    void loadPresets(const std::string& presetsLabel);

    /**
     * @brief Clear any preset flag on the node, but does'nt change the configuration
     **/
    void clearPresetFlag();

    void loadPresetsFromFile(const std::string& presetsFile);

    void exportNodeToPyPlug(const std::string& filePath);
    void exportNodeToPresets(const std::string& filePath,
                             const std::string& presetsLabel,
                             const std::string& iconFilePath,
                             int shortcutSymbol,
                             int shortcutModifiers);

public:




    std::string getCurrentNodePresets() const;

    /**
     * @brief Restores the node to its default state. If this node has a preset active or it is a PyPlug
     * it will be restored according to the preset/PyPlug.
     **/
    void restoreNodeToDefaultState(const CreateNodeArgsPtr& args);

    ///Set values for Knobs given their serialization
    void setValuesFromSerialization(const CreateNodeArgs& args);


    ///function called by EffectInstance to create a knob
    template <class K>
    boost::shared_ptr<K> createKnob(const std::string &description,
                                    int dimension = 1,
                                    bool declaredByPlugin = true)
    {
        return appPTR->getKnobFactory().createKnob<K>(getEffectInstance(), description, dimension, declaredByPlugin);
    }

    ///This cannot be done in loadKnobs as to call this all the nodes in the project must have
    ///been loaded first.
    void restoreKnobsLinks(const SERIALIZATION_NAMESPACE::NodeSerialization & serialization,
                           const std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >& allCreatedNodesInGroup);

    void setPagesOrder(const std::list<std::string>& pages);

    std::list<std::string> getPagesOrder() const;

    bool hasPageOrderChangedSinceDefault() const;

    bool isNodeCreated() const;

    bool isGLFinishRequiredBeforeRender() const;

    void refreshAcceptedBitDepths();

    /**
     * @brief Quits any processing on going on this node, this call is non blocking
     **/
    void quitAnyProcessing_non_blocking();
    void quitAnyProcessing_blocking(bool allowThreadsToRestart);

    /**
     * @brief Returns true if all processing threads controlled by this node have quit
     **/
    bool areAllProcessingThreadsQuit() const;

    /* @brief Similar to quitAnyProcessing except that the threads aren't destroyed
     * This is called when a node is deleted by the user
     */
    void abortAnyProcessing_non_blocking();
    void abortAnyProcessing_blocking();


    EffectInstancePtr getEffectInstance() const;
    /**
     * @brief Forwarded to the live effect instance
     **/
    void beginEditKnobs();

    /**
     * @brief Forwarded to the live effect instance
     **/
    const std::vector< KnobIPtr > & getKnobs() const;


    /**
     * @brief If this node is an output node, return the render engine
     **/
    RenderEnginePtr getRenderEngine() const;

    /**
     * @brief Is this node render engine currently doing playback ?
     **/
    bool isDoingSequentialRender() const;

  

    /**
     * @brief Forwarded to the live effect instance
     **/
    int getMajorVersion() const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    int getMinorVersion() const;


    /**
     * @brief Forwarded to the live effect instance
     **/
    bool isInputNode() const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    bool isOutputNode() const;


    /**
     * @brief Returns true if this node is a backdrop
     **/
    bool isBackdropNode() const;


    ViewerNodePtr isEffectViewerNode() const;

    ViewerInstancePtr isEffectViewerInstance() const;

    NodeGroupPtr isEffectNodeGroup() const;

    StubNodePtr isEffectStubNode() const;

    PrecompNodePtr isEffectPrecompNode() const;

    GroupInputPtr isEffectGroupInput() const;

    GroupOutputPtr isEffectGroupOutput() const;

    ReadNodePtr isEffectReadNode() const;

    WriteNodePtr isEffectWriteNode() const;

    BackdropPtr isEffectBackdrop() const;

    OneViewNodePtr isEffectOneViewNode() const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    int getMaxInputCount() const;

    /**
     * @brief Hint indicating to the UI that this node has numerous optional inputs and should not display them all.
     * See Switch/Viewer node for examples
     **/
    bool isEntitledForInspectorInputsStyle() const;

    /**
     * @brief Returns true if the given input supports the given components. If inputNb equals -1
     * then this function will check whether the effect can produce the given components.
     **/
    bool isSupportedComponent(int inputNb, const ImagePlaneDesc& comp) const;

    /**
     * @brief Returns the most appropriate number of components that can be supported by the inputNb.
     * If inputNb equals -1 then this function will check the output components.
     **/
    int findClosestSupportedNumberOfComponents(int inputNb, int nComps) const;

    ImageBitDepthEnum getBestSupportedBitDepth() const;
    bool isSupportedBitDepth(ImageBitDepthEnum depth) const;
    ImageBitDepthEnum getClosestSupportedBitDepth(ImageBitDepthEnum depth);

    /**
     * @brief Returns the components and index of the channel to use to produce the mask.
     * None = -1
     * R = 0
     * G = 1
     * B = 2
     * A = 3
     **/
    int getMaskChannel(int inputNb, const std::list<ImagePlaneDesc>& availableLayers, ImagePlaneDesc* comps) const;

    int isMaskChannelKnob(const KnobIConstPtr& knob) const;

    /**
     * @brief Returns whether masking is enabled or not
     **/
    bool isMaskEnabled(int inputNb) const;

    /**
     * @brief Returns a pointer to the input Node at index 'index'
     * or NULL if it couldn't find such node.
     * MT-safe
     * This function uses thread-local storage because several render thread can be rendering concurrently
     * and we want the rendering of a frame to have a "snap-shot" of the tree throughout the rendering of the
     * frame.
     **/
    NodePtr getInput(int index) const;

    /**
     * @brief Same as getInput except that it doesn't do group redirections for Inputs/Outputs
     **/
    NodePtr getRealInput(int index) const;

private:

    NodePtr getInputInternal(bool useGroupRedirections, int index) const;

public:

    /**
     * @brief Returns the input index of the node if it is an input of this node, -1 otherwise.
     **/
    int getInputIndex(const NodeConstPtr& node) const;

    /**
     * @brief Returns true if the node is currently executing the onInputChanged handler.
     **/
    bool duringInputChangedAction() const;

    /**
     *@brief Returns the inputs of the node as the Gui just set them.
     * The vector might be different from what getInputs_other_thread() could return.
     * This can only be called by the main thread.
     **/
    const std::vector<NodeWPtr > & getInputs() const WARN_UNUSED_RETURN;
    std::vector<NodeWPtr > getInputs_copy() const WARN_UNUSED_RETURN;

    /**
     * @brief Returns the input index of the node n if it exists,
     * -1 otherwise.
     **/
    int inputIndex(const NodePtr& n) const;

    const std::vector<std::string> & getInputLabels() const;
    std::string getInputLabel(int inputNb) const;

    std::string getInputHint(int inputNb) const;

    void setInputLabel(int inputNb, const std::string& label);

    void setInputHint(int inputNb, const std::string& hint);

    bool isInputVisible(int inputNb) const;

    void setInputVisible(int inputNb, bool visible);

    int getInputNumberFromLabel(const std::string& inputLabel) const;

    bool isInputConnected(int inputNb) const;

    bool hasOutputConnected() const;

    bool hasInputConnected() const;

    bool hasOverlay() const;

    bool hasMandatoryInputDisconnected() const;

    bool hasAllInputsConnected() const;

    /**
     * @brief This is used by the auto-connection algorithm.
     * When connecting nodes together this function helps determine
     * on which input it should connect a new node.
     * It returns the first non optional empty input or the first optional
     * empty input if they are all optionals, or -1 if nothing matches the 2 first conditions..
     * if all inputs are connected.
     **/
    virtual int getPreferredInputForConnection()  const;
    virtual int getPreferredInput() const;

    NodePtr getPreferredInputNode() const;


    void setRenderThreadSafety(RenderSafetyEnum safety);
    RenderSafetyEnum getCurrentRenderThreadSafety() const;
    RenderSafetyEnum getPluginRenderThreadSafety() const;
    void revertToPluginThreadSafety();

    void setCurrentOpenGLRenderSupport(PluginOpenGLRenderSupport support);
    PluginOpenGLRenderSupport getCurrentOpenGLRenderSupport() const;

    void setCurrentSequentialRenderSupport(SequentialPreferenceEnum support);
    SequentialPreferenceEnum getCurrentSequentialRenderSupport() const;

    void setCurrentCanDistort(bool support);
    bool getCurrentCanDistort() const;

    void setCurrentCanTransform(bool support);
    bool getCurrentCanTransform() const;

    void setCurrentSupportTiles(bool support);
    bool getCurrentSupportTiles() const;

    void setCurrentSupportRenderScale(bool support);
    bool getCurrentSupportRenderScale() const;

    void refreshDynamicProperties();

    bool isRenderScaleSupportEnabledForPlugin() const;

    bool isMultiThreadingSupportEnabledForPlugin() const;

    /////////////////////ROTO-PAINT related functionnalities//////////////////////
    //////////////////////////////////////////////////////////////////////////////

    bool isDuringPaintStrokeCreation() const;

    ////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////

    void setProcessChannelsValues(bool doR, bool doG, bool doB, bool doA);

    /**
     * @brief Used in the implementation of EffectInstance::onMetadataChanged_recursive so we know if the metadata changed or not.
     * Can only be called on the main thread.
     **/
    U64 getLastTimeInvariantMetadataHash() const;
    void setLastTimeInvariantMetadataHash(U64 lastTimeInvariantMetadataHashRefreshed);

private:

    int getPreferredInputInternal(bool connected) const;

public:


    /**
     * @brief Returns in 'outputs' a map of all nodes connected to this node
     * where the value of the map is the input index from which these outputs
     * are connected to this node.
     **/
    void getOutputsConnectedToThisNode(std::map<NodePtr, int>* outputs);

    const NodesWList & getOutputs() const;
    void getOutputs_mt_safe(NodesWList& outputs) const;

    /**
     * @brief Same as above but enters into subgroups
     **/
    void getOutputsWithGroupRedirection(NodesList& outputs) const;

    /**
     * @brief Each input label is mapped against the script-name of the input
     * node, or an empty string if disconnected. Masks are placed in a separate map;
     **/
    void getInputNames(std::map<std::string, std::string> & inputNames,
                       std::map<std::string, std::string> & maskNames) const;


    enum CanConnectInputReturnValue
    {
        eCanConnectInput_ok = 0,
        eCanConnectInput_indexOutOfRange,
        eCanConnectInput_inputAlreadyConnected,
        eCanConnectInput_givenNodeNotConnectable,
        eCanConnectInput_groupHasNoOutput,
        eCanConnectInput_graphCycles,
        eCanConnectInput_differentPars,
        eCanConnectInput_differentFPS,
        eCanConnectInput_multiResNotSupported,
    };

    /**
     * @brief Returns true if a connection is possible for the given input number of the current node
     * to the given input.
     **/
    Node::CanConnectInputReturnValue canConnectInput(const NodePtr& input, int inputNumber) const;


    /** @brief Adds the node parent to the input inputNumber of the
     * node. Returns true if it succeeded, false otherwise.
     * When returning false, this means an input was already
     * connected for this inputNumber. It should be removed
     * beforehand.
     */
    virtual bool connectInput(const NodePtr& input, int inputNumber);

    bool connectInputBase(const NodePtr& input,
                          int inputNumber)
    {
        return Node::connectInput(input, inputNumber);
    }

    /** @brief Removes the node connected to the input inputNumber of the
     * node. Returns true upon sucess, false otherwise.
     */
    bool disconnectInput(int inputNumber);

    /** @brief Removes the node input of the
     * node inputs. Returns true upon success, false otherwise.
     */
    bool disconnectInput(const NodePtr& input);

    /**
     * @brief Atomically swaps the given inputNumber to the given input node.
     * If the input is NULL, the input is disconnected. This is the same as calling
     * atomically disconnectInput() then connectInput().
     **/
    bool swapInput(const NodePtr& input, int inputNumber);

    void setNodeGuiPointer(const NodeGuiIPtr& gui);

    NodeGuiIPtr getNodeGui() const;

    bool isSettingsPanelVisible() const;

    bool isSettingsPanelMinimized() const;

    void onOpenGLEnabledKnobChangedOnProject(bool activated);

    KnobChoicePtr getOrCreateOpenGLEnabledKnob() const;

private:

    bool replaceInputInternal(const NodePtr& input, int inputNumber, bool failIfExisting);


    bool isSettingsPanelVisibleInternal(std::set<NodeConstPtr>& recursionList) const;

public:

    bool isUserSelected() const;


    /**
     * @brief If the session is a GUI session, then this function sets the position of the node on the nodegraph.
     **/
    void setPosition(double x, double y);
    void getPosition(double *x, double *y) const;
    void onNodeUIPositionChanged(double x, double y);

    void setSize(double w, double h);
    void getSize(double* w, double* h) const;
    void onNodeUISizeChanged(double x, double y);

    /**
     * @brief Get the colour of the node as it appears on the nodegraph.
     **/
    bool getColor(double* r, double *g, double* b) const;
    void setColor(double r, double g, double b);
    void onNodeUIColorChanged(double r, double g, double b);
    bool hasColorChangedSinceDefault() const;
    void getDefaultColor(double* r, double *g, double* b) const;

    void setOverlayColor(double r, double g, double b);
    bool getOverlayColor(double* r, double* g, double* b) const;
    void onNodeUIOverlayColorChanged(double r, double g, double b);

    void onNodeUISelectionChanged(bool isSelected);
    bool getNodeIsSelected() const;


    std::string getKnobChangedCallback() const;
    std::string getInputChangedCallback() const;

    void runChangedParamCallback(const KnobIPtr& k, bool userEdited);

    /**
     * @brief This is used exclusively by nodes in the underlying graph of the implementation of the RotoPaint.
     * Do not use that anywhere else.
     **/
    void attachRotoItem(const RotoDrawableItemPtr& stroke);

    /**
     * @brief Returns the attached roto item. If called from a render thread, this will
     * return a pointer to the shallow render copy.
     **/
    RotoDrawableItemPtr getAttachedRotoItem() const;

    /**
     * @brief Return the item set with attachRotoItem
     **/
    RotoDrawableItemPtr getOriginalAttachedItem() const;

protected:

    void runInputChangedCallback(int index);

private:

    /**
     * @brief Adds an output to this node.
     **/
    void connectOutput(const NodePtr& output);

    /** @brief Removes the node output of the
     * node outputs. Returns the outputNumber if it could remove it,
       otherwise returns -1.*/
    int disconnectOutput(const Node* output);

public:

    /**
     * @brief Switches the 2 first inputs that are not a mask, if and only if they have compatible components/bitdepths
     **/
    void switchInput0And1();

    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string getPluginID() const;


    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string getPluginLabel() const;

    /**
     * @brief Returns the path to where the resources are stored for this plug-in
     **/
    std::string getPluginResourcesPath() const;

    void getPluginGrouping(std::vector<std::string>* grouping) const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string getPluginDescription() const;

    /**
     * @brief Returns the absolute file-path to the plug-in icon.
     **/
    std::string getPluginIconFilePath() const;

    /**
     * @brief Returns true if this node is a PyPlug
     **/
    bool isPyPlug() const;


    AppInstancePtr getApp() const;


    /* @brief Make this node inactive. It will appear
       as if it was removed from the graph editor
       but the object still lives to allow
       undo/redo operations.
       @param outputsToDisconnect A list of the outputs whose inputs must be disconnected
       @param disconnectAll If set to true the parameter outputsToDisconnect is ignored and all outputs' inputs are disconnected
       @param reconnect If set to true Natron will attempt to re-connect disconnected output to an input of this node
       @param hideGui When true, the node gui will be notified so it gets hidden
     */
    void deactivate(const std::list< NodePtr > & outputsToDisconnect = std::list< NodePtr >(),
                    bool disconnectAll = true,
                    bool reconnect = true,
                    bool hideGui = true,
                    bool triggerRender = true,
                    bool unslaveKnobs = true);


    /* @brief Make this node active. It will appear
       again on the node graph.
       WARNING: this function can only be called
       after a call to deactivate() has been made.
     *
     * @param outputsToRestore Only the outputs specified that were previously connected to the node prior to the call to
     * deactivate() will be reconnected as output to this node.
     * @param restoreAll If true, the parameter outputsToRestore will be ignored.
     */
    void activate(const std::list< NodePtr > & outputsToRestore = std::list< NodePtr >(),
                  bool restoreAll = true,
                  bool triggerRender = true);

    /**
     * @brief Calls deactivate() and then remove the node from the project. The object will be destroyed
     * when the caller releases the reference to this Node
     * @param blockingDestroy This function needs to ensure no processing occurs when the node is destroyed. If this parameter
     * is set to true, the function will sit and wait until all processing is done, otherwise it will return immediatelely even if the node
     * is not yet detroyed
     * @param autoReconnect If set to true, outputs connected to this node will try to connect to the input of this node automatically.
     **/
    void destroyNode(bool blockingDestroy, bool autoReconnect);



private:
    
    
    void doDestroyNodeInternalEnd(bool autoReconnect);

public:


    /**
     * @brief Forwarded to the live effect instance
     **/
    KnobIPtr getKnobByName(const std::string & name) const;


    bool makePreviewByDefault() const;

    void togglePreview();

    bool isPreviewEnabled() const;

    /**
     * @brief Makes a small 8-bit preview image of size width x height with a ARGB32 format.
     * Pre-condition:
     *  - buf has been allocated for the correct amount of memory needed to fill the buffer.
     * Post-condition:
     *  - buf must not be freed or overflown.
     * It will serve as a preview on the node's graphical user interface.
     * This function is called directly by the PreviewThread.
     *
     * In order to notify the GUI that you want to refresh the preview, just
     * call refreshPreviewImage(time).
     *
     * The width and height might be modified by the function, so their value can
     * be queried at the end of the function
     **/
    bool makePreviewImage(TimeValue time, int *width, int *height, unsigned int* buf);

    /**
     * @brief Returns true if the node is currently rendering a preview image.
     **/
    bool isRenderingPreview() const;


    /**
     * @brief Returns true if the node is active for use in the graph editor. MT-safe
     **/
    bool isActivated() const;


    /**
     * @brief Use this function to post a transient message to the user. It will be displayed using
     * a dialog. The message can be of 4 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * eMessageTypeWarning : you want to inform the user that something important happened.
     * eMessageTypeError : you want to inform the user an error occured.
     * eMessageTypeQuestion : you want to ask the user about something.
     * The function will return true always except for a message of type eMessageTypeQuestion, in which
     * case the function may return false if the user pressed the 'No' button.
     * @param content The message you want to pass.
     **/
    bool message(MessageTypeEnum type, const std::string & content) const;

    /**
     * @brief Use this function to post a persistent message to the user. It will be displayed on the
     * node's graphical interface and on any connected viewer. The message can be of 3 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * eMessageTypeWarning : you want to inform the user that something important happened.
     * eMessageTypeError : you want to inform the user an error occured.
     * @param content The message you want to pass.
     **/
    void setPersistentMessage(MessageTypeEnum type, const std::string & content);

    /**
     * @brief Clears any message posted previously by setPersistentMessage.
     * This function will also be called on all inputs
     **/
    void clearPersistentMessage(bool recurse);

private:

    void clearPersistentMessageRecursive(std::list<NodePtr>& markedNodes);

    void clearPersistentMessageInternal();

public:


    bool notifyInputNIsRendering(int inputNb);

    void notifyInputNIsFinishedRendering(int inputNb);

    bool notifyRenderingStarted();

    void notifyRenderingEnded();

    int getIsInputNRenderingCounter(int inputNb) const;

    int getIsNodeRenderingCounter() const;


    //see eRenderSafetyInstanceSafe in EffectInstance::renderRoI
    //only 1 clone can render at any time
    QMutex & getRenderInstancesSharedMutex();

    void refreshPreviewsRecursivelyDownstream(TimeValue time);

    void refreshPreviewsRecursivelyUpstream(TimeValue time);

public:

    static void choiceParamAddLayerCallback(const KnobChoicePtr& knob);

    //When creating a Reader or Writer node, this is a pointer to the "bundle" node that the user actually see.
    NodePtr getIOContainer() const;

    KnobStringPtr getExtraLabelKnob() const;

    KnobStringPtr getOFXSubLabelKnob() const;

    void beginInputEdition();

    void endInputEdition(bool triggerRender);

    void onInputChanged(int inputNb);

    bool onEffectKnobValueChanged(const KnobIPtr& what, ValueChangedReasonEnum reason);

    bool getDisabledKnobValue() const;

    bool isNodeDisabledForFrame(TimeValue time, ViewIdx view) const;

    void setNodeDisabled(bool disabled);

    KnobBoolPtr getDisabledKnob() const;

    bool isLifetimeActivated(int *firstFrame, int *lastFrame) const;

    KnobBoolPtr getLifeTimeEnabledKnob() const;

    KnobIntPtr getLifeTimeKnob() const;

    std::string getNodeExtraLabel() const;

    bool isKeepInAnimationModuleButtonDown() const;

private:

    void restoreSublabel();

public:

    /**
     * @brief Returns whether this node or one of its inputs (recursively) is marked as
     * eSequentialPreferenceOnlySequential
     *
     * @param nodeName If the return value is true, this will be set to the name of the node
     * which is sequential.
     *
     **/
    bool hasSequentialOnlyNodeUpstream(std::string & nodeName) const;


    /**
     * @brief Updates the sub label knob: e.g for the Merge node it corresponds to the
     * operation name currently used and visible on the node
     **/
    void updateEffectSubLabelKnob(const QString & name);

    /**
     * @brief Returns true if an effect should be able to connect this node.
     **/
    bool canOthersConnectToThisNode() const;

    /**
     * @brief Clears any pointer refering to the last rendered image
     **/
    void clearLastRenderedImage();

    /**
     * @brief For plug-ins that accumulate (for now just RotoShapeRenderNode), this is a pointer
     * to the last rendered image.
     **/
    void setLastRenderedImage(const ImagePtr& lastRenderedImage);
    ImagePtr getLastRenderedImage() const;

    struct KnobLink
    {
        KnobIWPtr masterKnob;
        KnobIWPtr slaveKnob;

        // The master node to which the knob is slaved to
        NodeWPtr masterNode;
    };

    /*Initialises inputs*/
    void initializeInputs();

    /**
     * @brief Forwarded to the live effect instance
     **/
    void initializeKnobs(bool loadingSerialization, bool hasGUI);

    void checkForPremultWarningAndCheckboxes();

    void findPluginFormatKnobs();

    KnobDoublePtr getOrCreateHostMixKnob(const KnobPagePtr& mainPage);
    
    KnobPagePtr getOrCreateMainPage();

private:

    void initializeDefaultKnobs(bool loadingSerialization, bool hasGUI);

    void findPluginFormatKnobs(const KnobsVec & knobs, bool loadingSerialization);

    void findRightClickMenuKnob(const KnobsVec& knobs);

    void createNodePage(const KnobPagePtr& settingsPage);

    void createInfoPage();

    void createPyPlugPage();

    void createPyPlugExportGroup();

    void createMaskSelectors(const std::vector<std::pair<bool, bool> >& hasMaskChannelSelector,
                             const std::vector<std::string>& inputLabels,
                             const KnobPagePtr& mainPage,
                             bool addNewLineOnLastMask,
                             KnobIPtr* lastKnobCreated);


    void createLabelKnob(const KnobPagePtr& settingsPage, const std::string& label);

    void findOrCreateChannelEnabled();

    void createChannelSelectors(const std::vector<std::pair<bool, bool> >& hasMaskChannelSelector,
                                const std::vector<std::string>& inputLabels,
                                const KnobPagePtr& mainPage,
                                KnobIPtr* lastKnobBeforeAdvancedOption);

public:


    /**
     * @brief Returns true if the parallel render args thread-storage is set
     **/
    bool isNodeRendering() const;

    bool hasPersistentMessage() const;

    void getPersistentMessage(QString* message, int* type, bool prefixLabelAndType = true) const;


    /**
     * @brief Attempts to detect cycles considering input being an input of this node.
     * Returns true if it couldn't detect any cycle, false otherwise.
     **/
    bool checkIfConnectingInputIsOk(const NodePtr& input) const;

    bool isForceCachingEnabled() const;

    void setForceCachingEnabled(bool b);

    /**
     * @brief Declares to Python all parameters as attribute of the variable representing this node.
     **/
    void declarePythonKnobs();

private:


    /**
     * @brief Declares to Python all parameters, roto, tracking attributes
     * This is called in activate() whenever the node was deleted
     **/
    void declareAllPythonAttributes();

public:
    /**
     * @brief Set the node name.
     * Throws a run-time error with the message in case of error
     **/
    void setScriptName(const std::string & name);

    void setScriptName_no_error_check(const std::string & name);


    /**
     * @brief The node unique name.
     **/
    const std::string & getScriptName() const;
    std::string getScriptName_mt_safe() const;

    /**
       @brief Returns the name of the node, prepended by the name of all the group containing it, e.g:
     * - a node in the "root" project would be: Blur1
     * - a node within the group 1 of the project would be : <g>group1</g>Blur1
     * - a node within the group 1 of the group 1 of the project would be : <g>group1</g><g>group1</g>Blur1
     **/
    std::string getFullyQualifiedName() const;

    std::string getContainerGroupFullyQualifiedName() const;

    void setLabel(const std::string& label);

    const std::string& getLabel() const;
    std::string getLabel_mt_safe() const;
    std::string getBeforeRenderCallback() const;
    std::string getBeforeFrameRenderCallback() const;
    std::string getAfterRenderCallback() const;
    std::string getAfterFrameRenderCallback() const;
    std::string getAfterNodeCreatedCallback() const;
    std::string getBeforeNodeRemovalCallback() const;
    
    void runAfterTableItemsSelectionChangedCallback(const std::list<KnobTableItemPtr>& deselected, const std::list<KnobTableItemPtr>& selected, TableChangeReasonEnum reason);

    void onFileNameParameterChanged(const KnobIPtr& fileKnob);

    static void getOriginalFrameRangeForReader(const std::string& pluginID, const std::string& canonicalFileName, int* firstFrame, int* lastFrame);

    void computeFrameRangeForReader(const KnobIPtr& fileKnob, bool setFrameRange);


    bool canHandleRenderScaleForOverlays() const;

    /**
     * @brief Push a new undo command to the undo/redo stack associated to this node.
     * The stack takes ownership of the shared pointer, so you should not hold a strong reference to the passed pointer.
     * If no undo/redo stack is present, the command will just be redone once then destroyed.
     **/
    void pushUndoCommand(const UndoCommandPtr& command);


    /**
     * @brief Set the cursor to be one of the default cursor.
     * @returns True if it successfully set the cursor, false otherwise.
     * Note: this can only be called during an overlay interact action.
     **/
    void setCurrentCursor(CursorEnum defaultCursor);
    bool setCurrentCursor(const QString& customCursorFilePath);

private:

    friend class DuringInteractActionSetter_RAII;
    void setDuringInteractAction(bool b);

public:


    void setCurrentViewportForOverlays_public(OverlaySupport* viewport);

    OverlaySupport* getCurrentViewportForOverlays() const;

    RenderScale getOverlayInteractRenderScale() const;

    bool isDoingInteractAction() const;

    bool shouldDrawOverlay(TimeValue time, ViewIdx view) const;


    void drawHostOverlay(TimeValue time,
                         const RenderScale& renderScale,
                         ViewIdx view);

    bool onOverlayPenDownDefault(TimeValue time,
                                 const RenderScale& renderScale,
                                 ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure) WARN_UNUSED_RETURN;

    bool onOverlayPenDoubleClickedDefault(TimeValue time,
                                          const RenderScale& renderScale,
                                          ViewIdx view, const QPointF & viewportPos, const QPointF & pos) WARN_UNUSED_RETURN;


    bool onOverlayPenMotionDefault(TimeValue time,
                                   const RenderScale& renderScale,
                                   ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure) WARN_UNUSED_RETURN;

    bool onOverlayPenUpDefault(TimeValue time,
                               const RenderScale& renderScale,
                               ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure) WARN_UNUSED_RETURN;

    bool onOverlayKeyDownDefault(TimeValue time,
                                 const RenderScale& renderScale,
                                 ViewIdx view, Key key, KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayKeyUpDefault(TimeValue time,
                               const RenderScale& renderScale,
                               ViewIdx view, Key key, KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayKeyRepeatDefault(TimeValue time,
                                   const RenderScale& renderScale,
                                   ViewIdx view, Key key, KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayFocusGainedDefault(TimeValue time,
                                     const RenderScale& renderScale,
                                     ViewIdx view) WARN_UNUSED_RETURN;

    bool onOverlayFocusLostDefault(TimeValue time,
                                   const RenderScale& renderScale,
                                   ViewIdx view) WARN_UNUSED_RETURN;

    void addPositionInteract(const KnobDoublePtr& position,
                             const KnobBoolPtr& interactive);

    void addTransformInteract(const KnobDoublePtr& translate,
                              const KnobDoublePtr& scale,
                              const KnobBoolPtr& scaleUniform,
                              const KnobDoublePtr& rotate,
                              const KnobDoublePtr& skewX,
                              const KnobDoublePtr& skewY,
                              const KnobChoicePtr& skewOrder,
                              const KnobDoublePtr& center,
                              const KnobBoolPtr& invert,
                              const KnobBoolPtr& interactive);

    void addCornerPinInteract(const KnobDoublePtr& from1,
                              const KnobDoublePtr& from2,
                              const KnobDoublePtr& from3,
                              const KnobDoublePtr& from4,
                              const KnobDoublePtr& to1,
                              const KnobDoublePtr& to2,
                              const KnobDoublePtr& to3,
                              const KnobDoublePtr& to4,
                              const KnobBoolPtr& enable1,
                              const KnobBoolPtr& enable2,
                              const KnobBoolPtr& enable3,
                              const KnobBoolPtr& enable4,
                              const KnobChoicePtr& overlayPoints,
                              const KnobBoolPtr& invert,
                              const KnobBoolPtr& interactive);

    void removePositionHostOverlay(const KnobIPtr& knob);

    void initializeHostOverlays();

    bool hasHostOverlay() const;

    void setCurrentViewportForHostOverlays(OverlaySupport* viewPort);

    bool hasHostOverlayForParam(const KnobIConstPtr& knob) const;

    bool isSubGraphEditedByUser() const;

    //Returns true if changed
    bool refreshChannelSelectors();

    void refreshLayersSelectorsVisibility();

    bool isPluginUsingHostChannelSelectors() const;

    bool getProcessChannel(int channelIndex) const;

    KnobBoolPtr getProcessChannelKnob(int channelIndex) const;

    KnobChoicePtr getChannelSelectorKnob(int inputNb) const;

    KnobBoolPtr getProcessAllLayersKnob() const;

    bool getSelectedLayer(int inputNb,
                          const std::list<ImagePlaneDesc>& availableLayers,
                          std::bitset<4> *processChannels,
                          bool* isAll,
                          ImagePlaneDesc *layer) const;

    bool addUserComponents(const ImagePlaneDesc& comps);

    void getUserCreatedComponents(std::list<ImagePlaneDesc>* comps);

    bool hasAtLeastOneChannelToProcess() const;

    void removeParameterFromPython(const std::string& parameterName);

    double getHostMixingValue(TimeValue time, ViewIdx view) const;

public:


    KnobChoicePtr getLayerChoiceKnob(int inputNb) const;

    const std::vector<std::string>& getCreatedViews() const;

    void refreshCreatedViews();

    void refreshIdentityState();

    bool getHideInputsKnobValue() const;
    void setHideInputsKnobValue(bool hidden);

    int getFrameStepKnobValue() const;

    void refreshFormatParamChoice(const std::vector<ChoiceOption>& entries, int defValue, bool loadingProject);

    bool handleFormatKnob(const KnobIPtr& knob);

    QString makeDocumentation(bool genHTML) const;

    void refreshPreviewsAfterProjectLoad();

    enum StreamWarningEnum
    {
        //A bitdepth conversion occurs and converts to a lower bitdepth the stream,
        //inducing a quality loss
        eStreamWarningBitdepth,

        //The node has inputs with different aspect ratios but does not handle it
        eStreamWarningPixelAspectRatio,

        //The node has inputs with different frame rates which may produce unwanted results
        eStreamWarningFrameRate,
    };

    void setStreamWarning(StreamWarningEnum warning, const QString& message);
    void setStreamWarnings(const std::map<StreamWarningEnum, QString>& warnings);
    void getStreamWarnings(std::map<StreamWarningEnum, QString>* warnings) const;

    void refreshEnabledKnobsLabel(const ImagePlaneDesc& mainInputComps, const ImagePlaneDesc& outputComps);

    bool isNodeUpstream(const NodeConstPtr& input) const;

    void onNodeMetadatasRefreshedOnMainThread(const NodeMetadata& meta);

private:



    /**
     * @brief If the node is an input of this node, set ok to true, otherwise
     * calls this function recursively on all inputs.
     **/
    bool isNodeUpstreamInternal(const NodeConstPtr& input, std::list<const Node*>& markedNodes) const;

    bool setStreamWarningInternal(StreamWarningEnum warning, const QString& message);

    void refreshCreatedViews(const KnobIPtr& knob);

    void setNameInternal(const std::string& name, bool throwErrors);

    std::string getFullyQualifiedNameInternal(const std::string& scriptName) const;

    void s_outputLayerChanged() { Q_EMIT outputLayerChanged(); }

public Q_SLOTS:


    void onProcessingQuitInDestroyNodeInternal(int taskID, const WatcherCallerArgsPtr& args);

    void onRefreshIdentityStateRequestReceived();


    void doRefreshEdgesGUI()
    {
        Q_EMIT refreshEdgesGUI();
    }

    /*will force a preview re-computation not matter of the project's preview mode*/
    void computePreviewImage(TimeValue time)
    {
        Q_EMIT previewRefreshRequested(time);
    }

    /*will refresh the preview only if the project is in auto-preview mode*/
    void refreshPreviewImage(TimeValue time)
    {
        Q_EMIT previewImageChanged(time);
    }

    void onInputLabelChanged(const QString& oldName, const QString& newName);

    void notifySettingsPanelClosed(bool closed )
    {
        Q_EMIT settingsPanelClosed(closed);
    }

Q_SIGNALS:

    void keepInAnimationModuleKnobChanged();

    void rightClickMenuKnobPopulated();

    void s_refreshPreviewsAfterProjectLoadRequested();

    void hideInputsKnobChanged(bool hidden);

    void refreshIdentityStateRequested();

    void availableViewsChanged();

    void outputLayerChanged();

    void settingsPanelClosed(bool);

    void persistentMessageChanged();

    void inputsInitialized();

    void inputLabelChanged(int, QString);

    void knobsInitialized();

    /*
     * @brief Emitted whenever an input changed on the GUI.
     */
    void inputChanged(int);

    void outputsChanged();

    void activated(bool triggerRender);

    void deactivated(bool triggerRender);

    void canUndoChanged(bool);

    void canRedoChanged(bool);

    void labelChanged(QString,QString);
    void scriptNameChanged(QString);
    void inputEdgeLabelChanged(int, QString);

    void inputVisibilityChanged(int);

    void refreshEdgesGUI();

    void previewImageChanged(TimeValue);

    void previewRefreshRequested(TimeValue);

    void inputNIsRendering(int inputNb);

    void inputNIsFinishedRendering(int inputNb);

    void renderingStarted();

    void renderingEnded();

    ///how much has just changed, this not the new value but the difference between the new value
    ///and the old value
    void pluginMemoryUsageChanged(qint64 mem);

    void knobSlaved();

    void previewKnobToggled();

    void disabledKnobToggled(bool disabled);

    void streamWarningsChanged();
    void nodeExtraLabelChanged();

    void nodePresetsChanged();

    void enabledChannelCheckboxChanged();

private:


    void declareTablePythonFields();

    std::string makeInfoForInput(int inputNumber) const;

    void refreshInfos();


    void declareNodeVariableToPython(const std::string& nodeName);
    void setNodeVariableToPython(const std::string& oldName, const std::string& newName);
    void deleteNodeVariableToPython(const std::string& nodeName);

    friend struct NodePrivate;
    boost::scoped_ptr<NodePrivate> _imp;
};


class DuringInteractActionSetter_RAII
{
    NodePtr _node;
public:

    DuringInteractActionSetter_RAII(const NodePtr& node)
    : _node(node)
    {
        node->setDuringInteractAction(true);
    }

    ~DuringInteractActionSetter_RAII()
    {
        _node->setDuringInteractAction(false);
    }
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_NODE_H
