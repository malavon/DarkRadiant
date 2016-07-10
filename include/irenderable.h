#pragma once

#include <memory>

class Shader;
typedef std::shared_ptr<Shader> ShaderPtr;

class RenderSystem;
typedef std::shared_ptr<RenderSystem> RenderSystemPtr;

class OpenGLRenderable;
class LightList;
class Matrix4;
class IRenderEntity;

/**
 * \brief
 * Class which accepts OpenGLRenderable objects during the first pass of
 * rendering.
 *
 * Each Renderable in the scenegraph is passed a reference to a
 * RenderableCollector, on which the Renderable sets the necessary state
 * variables and then submits its OpenGLRenderable(s) for later rendering. A
 * single Renderable may submit more than one OpenGLRenderable, with a different
 * state each time -- for instance a Renderable model class may submit each of
 * its material surfaces separately with the respective shaders set beforehand.
 *
 * \todo
 * This class probably doesn't need to be a state machine, convert it to a
 * single submit method with necessary parameters.
 */

class RenderableCollector
{
public:
	virtual ~RenderableCollector() {}

	/**
	 * Enumeration containing render styles.
	 */
	enum EStyle {
		eWireframeOnly,
		eFullMaterials
	};

	/**
	 * Push a Shader onto the internal shader stack. This is an OpenGL-style
	 * push, which does not accept an argument but duplicates the topmost
	 * stack value. The new value should be set with SetState().
	 */
	virtual void PushState() = 0;

	/**
	 * Pop the topmost Shader off the internal stack. This discards the value
	 * without returning it.
	 */
	virtual void PopState() = 0;

	/**
	 * Set the Shader to be used when rendering any subsequently-submitted
	 * OpenGLRenderable object. This shader remains in effect until it is
	 * changed with a subsequent call to SetState().
	 *
	 * @param state
	 * The Shader to be used from now on.
	 *
	 * @param mode
	 * The type of rendering (wireframe or shaded) that this shader should be
    * used for. Individual RenderableCollector subclasses may ignore this method
    * call if it does not use the render mode they are interested in.
	 */
	virtual void SetState(const ShaderPtr& state, EStyle mode) = 0;

	/**
	 * Submit an OpenGLRenderable object for rendering when the backend render
	 * pass is conducted. The object will be rendered using the Shader previous-
	 * ly set with SetState().
	 *
	 * @param renderable
	 * The renderable object to submit.
	 *
	 * @param world
	 * The local to world transform that should be applied to this object when
	 * it is rendered.
	 */
	virtual void addRenderable(const OpenGLRenderable& renderable,
							   const Matrix4& world) = 0;

	/**
	 * Like addRenderable() above but providing an additional IRenderEntity argument
	 * needed to evaluate the shader expressions right before rendering.
	 */
	virtual void addRenderable(const OpenGLRenderable& renderable,
							   const Matrix4& world,
							   const IRenderEntity& entity) = 0;


    /**
     * \brief
     * Determine if this RenderableCollector can accept renderables for full
     * materials rendering, or just wireframe rendering.
     *
     * \return
     * true if full materials are supported, false if only wireframe rendering
     * is supported.
     */
	virtual bool supportsFullMaterials() const = 0;

	struct Highlight
	{
		enum Flags
		{
			None		= 0,
			Faces		= 1 << 0, /// Highlight faces of subsequently-submitted objects, if supported
			Primitives	= 1 << 1, /// Highlight primitives of subsequently-submitted objects, if supported
			GroupMember	= 1 << 2, /// Highlight as member of group, if supported
		};
	};

	virtual void setHighlightFlag(Highlight::Flags flags, bool enabled) = 0;

  	/**
  	 * Set the list of lights to be used for lighting-mode rendering. This
    * method only makes sense for RenderableCollectors that support this
    * rendering mode.
  	 *
  	 * TODO: Use std::shared_ptr<> here.
  	 */
    virtual void setLights(const LightList& lights) { }
};

class VolumeTest;

/** Interface class for Renderable objects. All objects which wish to be
 * rendered need to implement this interface. During the scenegraph traversal
 * for rendering, each Renderable object is passed a RenderableCollector object
 * which it can use to submit its geometry and state parameters.
 */

class Renderable
{
public:
	/**
	 * Destructor
	 */
	virtual ~Renderable() {}

	/**
	 * Sets the rendersystem this renderable is attached to. This is necessary
	 * for this object to request Materials/Shaders for rendering.
	 */
	virtual void setRenderSystem(const RenderSystemPtr& renderSystem) = 0;

	/** Submit renderable geometry when rendering takes place in Solid mode.
	 */
	virtual void renderSolid(RenderableCollector& collector,
  							 const VolumeTest& volume) const = 0;

	/** Submit renderable geometry when rendering takes place in Wireframe
	 * mode.
	 */
	virtual void renderWireframe(RenderableCollector& collector,
								 const VolumeTest& volume) const = 0;

	virtual void renderComponents(RenderableCollector&, const VolumeTest&) const
	{ }

	virtual void viewChanged() const
	{ }

	struct Highlight
	{
		enum Flags
		{
			None				= 0,
			Selected			= 1 << 0,
			SelectedGroupMember = 1 << 1,
		};
	};

	/**
	 * Returns information about whether the renderer should highlight this node and how.
	 */
	virtual std::size_t getHighlightFlags() = 0;
};
typedef std::shared_ptr<Renderable> RenderablePtr;
