#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "irender.h"
#include "irenderable.h"
#include "render/frontend/RenderHighlighted.h"

/**
 * Scene render function. Uses the visibility walkers to traverse the scene
 * graph and submit all visible objects to the provided RenderableCollector.
 */
inline void Scene_Render(RenderableCollector& collector, const VolumeTest& volume) {

	// Instantiate a new walker class
	RenderHighlighted renderHighlightWalker(collector, volume);

	// Submit renderables from scene graph
	GlobalSceneGraph().foreachNodeInVolume(volume, renderHighlightWalker);
	
	// Submit renderables directly attached to the ShaderCache
	GlobalRenderSystem().forEachRenderable(
		RenderHighlighted::RenderCaller(RenderHighlighted(collector, volume)));
}

#endif /* _RENDERER_H_ */
