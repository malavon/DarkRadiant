#ifndef LIGHTCLASS_H_
#define LIGHTCLASS_H_

#include "igl.h"
#include "irender.h"
#include "cullable.h"
#include "scenelib.h"
#include "editable.h"
#include "render.h"
#include "renderable.h"
#include "traverselib.h"
#include "math/frustum.h"

#include "../keyobservers.h"
#include "../origin.h"
#include "../rotation.h"
#include "../colour.h"
#include "../namedentity.h"
#include "../namekeys.h"
#include "../entity.h"

#include "LightTypes.h"
#include "Renderables.h"
#include "LightShader.h"
#include "RenderableVertices.h"
#include "Doom3LightRadius.h"

/* greebo: This is the actual light class. It contains the information about the geometry
 * of the light and the actual render functions.
 * 
 * This class owns all the keyObserver callbacks, that get invoked as soon as the entity key/values get
 * changed by the user.
 * 
 * The subclass Doom3LightRadius contains some variables like the light radius and light center coordinates,
 * and there are some "onChanged" callbacks for the light radius and light center.
 * 
 * Note: All the selection stuff is handled by the LightInstance class. This is just the bare bone light.
 * 
 * Note on Projectd Lights Support: There are some callbacks and the according member variables plus
 * some observers already installed, there is even a render method, but I can't say how functional
 * they really are. 
 */

void light_vertices(const AABB& aabb_light, Vector3 points[6]);
void light_draw(const AABB& aabb_light, RenderStateFlags state);

inline const BasicVector4<double>& plane3_to_vector4(const Plane3& self)
{
  return reinterpret_cast<const BasicVector4<double>&>(self);
}

inline BasicVector4<double>& plane3_to_vector4(Plane3& self)
{
  return reinterpret_cast<BasicVector4<double>&>(self);
}

inline Matrix4 matrix4_from_planes(const Plane3& left, const Plane3& right, const Plane3& bottom, const Plane3& top, const Plane3& front, const Plane3& back)
{
  return Matrix4(
    (right.a - left.a) / 2,
    (top.a - bottom.a) / 2,
    (back.a - front.a) / 2,
    right.a - (right.a - left.a) / 2,
    (right.b - left.b) / 2,
    (top.b - bottom.b) / 2,
    (back.b - front.b) / 2,
    right.b - (right.b - left.b) / 2,
    (right.c - left.c) / 2,
    (top.c - bottom.c) / 2,
    (back.c - front.c) / 2,
    right.c - (right.c - left.c) / 2,
    (right.d - left.d) / 2,
    (top.d - bottom.d) / 2,
    (back.d - front.d) / 2,
    right.d - (right.d - left.d) / 2
  );
}

inline void default_extents(Vector3& extents) {
	extents = Vector3(8,8,8);
}

class Light :
	public OpenGLRenderable,
	public Cullable,
	public Bounded,
	public Editable,
	public Snappable
{
  EntityKeyValues m_entity;
  KeyObserverMap m_keyObservers;
  TraversableNodeSet m_traverse;
  IdentityTransform m_transform;

  OriginKey m_originKey;
  RotationKey m_rotationKey;
  Float9 m_rotation;
  Colour m_colour;

  NamedEntity m_named;
  NameKeys m_nameKeys;
  TraversableObserverPairRelay m_traverseObservers;
  Doom3GroupOrigin m_funcStaticOrigin;

	Doom3LightRadius m_doom3Radius;

	// Renderable components of this light
	entity::RenderLightRadiiBox m_radii_box;
	entity::RenderableLightCentre _rCentre;
	entity::RenderableLightTarget _rTarget;
	
	entity::RenderableLightRelative _rUp;
	entity::RenderableLightRelative _rRight;
	
	entity::RenderableLightTarget _rStart;
	entity::RenderableLightTarget _rEnd;
	RenderableNamedEntity m_renderName;

  Vector3 m_lightOrigin;
  bool m_useLightOrigin;
  Float9 m_lightRotation;
  bool m_useLightRotation;

	// These are the vectors that define a projected light
	Vector3 m_lightTarget;
	bool m_useLightTarget;
	Vector3 _colourLightTarget;
	Vector3 m_lightUp;
	bool m_useLightUp;
	Vector3 _colourLightUp;
	Vector3 m_lightRight;
	bool m_useLightRight;
	Vector3 _colourLightRight;
	Vector3 m_lightStart;
	bool m_useLightStart;
	Vector3 _colourLightStart;
	Vector3 m_lightEnd;
	bool m_useLightEnd;
	Vector3 _colourLightEnd;

  mutable AABB m_doom3AABB;
  mutable AABB _lightAABB;
  mutable Matrix4 m_doom3Rotation;
  mutable Matrix4 m_doom3Projection;
  mutable Frustum m_doom3Frustum;
  mutable bool m_doom3ProjectionChanged;

  entity::RenderLightProjection m_renderProjection;

  LightShader m_shader;

  AABB m_aabb_light;

  Callback m_transformChanged;
  Callback m_boundsChanged;
  Callback m_evaluateTransform;

	void construct();
	void destroy();

	void updateOrigin();
	void originChanged();
	typedef MemberCaller<Light, &Light::originChanged> OriginChangedCaller;

	void lightOriginChanged(const char* value);
	typedef MemberCaller1<Light, const char*, &Light::lightOriginChanged> LightOriginChangedCaller;

	void lightTargetChanged(const char* value);
	typedef MemberCaller1<Light, const char*, &Light::lightTargetChanged> LightTargetChangedCaller;
	void lightUpChanged(const char* value);
	typedef MemberCaller1<Light, const char*, &Light::lightUpChanged> LightUpChangedCaller;
	void lightRightChanged(const char* value);
	typedef MemberCaller1<Light, const char*, &Light::lightRightChanged> LightRightChangedCaller;
	void lightStartChanged(const char* value);
	typedef MemberCaller1<Light, const char*, &Light::lightStartChanged> LightStartChangedCaller;
	void lightEndChanged(const char* value);
	typedef MemberCaller1<Light, const char*, &Light::lightEndChanged> LightEndChangedCaller;

	void writeLightOrigin();

	void updateLightRadiiBox() const;

	void rotationChanged();
	typedef MemberCaller<Light, &Light::rotationChanged> RotationChangedCaller;

	void lightRotationChanged(const char* value);
	typedef MemberCaller1<Light, const char*, &Light::lightRotationChanged> LightRotationChangedCaller;

public:
	// Constructor
	Light(IEntityClass* eclass, scene::Node& node, const Callback& transformChanged, const Callback& boundsChanged, const Callback& evaluateTransform) :
		m_entity(eclass),
		m_originKey(OriginChangedCaller(*this)),
		m_rotationKey(RotationChangedCaller(*this)),
		m_colour(Callback()),
		m_named(m_entity),
		m_nameKeys(m_entity),
		m_funcStaticOrigin(m_traverse, m_originKey.m_origin),
		m_radii_box(m_aabb_light.origin),
		_rCentre(m_doom3Radius.m_centerTransformed, m_aabb_light.origin, m_doom3Radius._centerColour),
		_rTarget(m_lightTarget, m_aabb_light.origin, _colourLightTarget), 
		_rUp(m_lightUp, m_lightTarget, m_aabb_light.origin, _colourLightUp),
		_rRight(m_lightRight, m_lightTarget, m_aabb_light.origin, _colourLightRight),
		_rStart(m_lightStart, m_aabb_light.origin, _colourLightStart),
		_rEnd(m_lightEnd, m_aabb_light.origin, _colourLightEnd),
		m_renderName(m_named, m_aabb_light.origin),
		m_useLightOrigin(false),
		m_useLightRotation(false),
		m_renderProjection(m_doom3Projection, m_aabb_light.origin, m_lightTarget, m_lightRight, m_lightUp),
		m_transformChanged(transformChanged),
		m_boundsChanged(boundsChanged),
		m_evaluateTransform(evaluateTransform)
	{
		construct();
	}
	
	// Copy Constructor
	Light(const Light& other, scene::Node& node, const Callback& transformChanged, const Callback& boundsChanged, const Callback& evaluateTransform) :
		m_entity(other.m_entity),
		m_originKey(OriginChangedCaller(*this)),
		m_rotationKey(RotationChangedCaller(*this)),
		m_colour(Callback()),
		m_named(m_entity),
		m_nameKeys(m_entity),
		m_funcStaticOrigin(m_traverse, m_originKey.m_origin),
		m_radii_box(m_aabb_light.origin),
		_rCentre(m_doom3Radius.m_centerTransformed, m_aabb_light.origin, m_doom3Radius._centerColour),
		_rTarget(m_lightTarget, m_aabb_light.origin, _colourLightTarget),
		_rUp(m_lightUp, m_lightTarget, m_aabb_light.origin, _colourLightUp),
		_rRight(m_lightRight, m_lightTarget, m_aabb_light.origin, _colourLightRight),
		_rStart(m_lightStart, m_aabb_light.origin, _colourLightStart),
		_rEnd(m_lightEnd, m_aabb_light.origin, _colourLightEnd),
		m_renderName(m_named, m_aabb_light.origin),
		m_useLightOrigin(false),
		m_useLightRotation(false),
		m_renderProjection(m_doom3Projection, m_aabb_light.origin, m_lightTarget, m_lightRight, m_lightUp),
		m_transformChanged(transformChanged),
		m_boundsChanged(boundsChanged),
		m_evaluateTransform(evaluateTransform)
	{
		construct();
	}
	
	// Destructor
	~Light() {
		destroy();
	}

	InstanceCounter m_instanceCounter;
	void instanceAttach(const scene::Path& path);
	void instanceDetach(const scene::Path& path);

	EntityKeyValues& getEntity();
	const EntityKeyValues& getEntity() const;

	scene::Traversable& getTraversable();
	Namespaced& getNamespaced();
	Nameable& getNameable();
	TransformNode& getTransformNode();

	void attach(scene::Traversable::Observer* observer);
	void detach(scene::Traversable::Observer* observer);

	void render(RenderStateFlags state) const;

	VolumeIntersectionValue intersectVolume(const VolumeTest& volume, const Matrix4& localToWorld) const;
	const AABB& localAABB() const;
	const AABB& lightAABB() const;

	// Note: move this upwards
	mutable Matrix4 m_projectionOrientation;

	void renderSolid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const;
	void renderWireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const;

	// Adds the light centre renderable to the given renderer
	void renderLightCentre(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void renderProjectionPoints(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected) const;

	// Returns a reference to the member class Doom3LightRadius (used to set colours)
	Doom3LightRadius& getDoom3Radius();

	void testSelect(Selector& selector, SelectionTest& test, const Matrix4& localToWorld);
  
	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);
	void snapto(float snap);
	void setLightRadius(const AABB& aabb);
	void transformLightRadius(const Matrix4& transform);
	void revertTransform();
	void freezeTransform();
	void transformChanged();
	typedef MemberCaller<Light, &Light::transformChanged> TransformChangedCaller;

	// note: move this
	mutable Matrix4 m_localPivot;
	const Matrix4& getLocalPivot() const;

	void setLightChangedCallback(const Callback& callback);

	const AABB& aabb() const;
  	bool testAABB(const AABB& other) const;

	const Matrix4& rotation() const;
	const Vector3& offset() const;
	const Vector3& colour() const;

	bool isProjected() const;
	void projectionChanged();

	const Matrix4& projection() const;

	Shader* getShader() const;
}; // class Light

#endif /*LIGHTCLASS_H_*/
