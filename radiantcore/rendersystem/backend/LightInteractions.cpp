#include "LightInteractions.h"

#include "OpenGLShader.h"
#include "ObjectRenderer.h"
#include "glprogram/GLSLDepthFillAlphaProgram.h"

namespace render
{

void LightInteractions::addObject(IRenderableObject& object, IRenderEntity& entity, OpenGLShader* shader)
{
    auto& objectsByMaterial = _objectsByEntity.emplace(
        &entity, ObjectsByMaterial{}).first->second;

    auto& surfaces = objectsByMaterial.emplace(
        shader, ObjectList{}).first->second;

    surfaces.emplace_back(std::ref(object));

    ++_objectCount;
}

bool LightInteractions::isInView(const IRenderView& view)
{
    return view.TestAABB(_lightBounds) != VOLUME_OUTSIDE;
}

void LightInteractions::collectSurfaces(const std::set<IRenderEntityPtr>& entities)
{
    // Now check all the entities intersecting with this light
    for (const auto& entity : entities)
    {
        entity->foreachRenderableTouchingBounds(_lightBounds,
            [&](const IRenderableObject::Ptr& object, Shader* shader)
        {
            // Skip empty objects
            if (!object->isVisible()) return;

            // Don't collect invisible shaders
            if (!shader->isVisible()) return;

            auto glShader = static_cast<OpenGLShader*>(shader);

            // We only consider materials designated for camera rendering
            if (!glShader->isApplicableTo(RenderViewType::Camera))
            {
                return;
            }

            if (!glShader->getInteractionPass())
            {
                return; // This material doesn't interact with lighting
            }

            addObject(*object, *entity, glShader);
        });
    }
}

void LightInteractions::fillDepthBuffer(OpenGLState& state, GLSLDepthFillAlphaProgram& program, const IRenderView& view, std::size_t renderTime)
{
    std::vector<IGeometryStore::Slot> untransformedObjects;
    untransformedObjects.reserve(10000);

    // Set the modelview and projection matrix
    program.setModelViewProjection(view.GetViewProjection());

    for (const auto& [entity, objectsByShader] : _objectsByEntity)
    {
        for (const auto& [shader, objects] : objectsByShader)
        {
            auto depthFillPass = shader->getDepthFillPass();

            if (!depthFillPass) continue;

            // Skip translucent materials
            if (shader->getMaterial() && shader->getMaterial()->getCoverage() == Material::MC_TRANSLUCENT)
            {
                continue;
            }

            // Evaluate the shader stages of this material
            depthFillPass->evaluateShaderStages(renderTime, entity);

            // Apply the alpha test value, it might be affected by time and entity parms
            program.setAlphaTest(depthFillPass->getAlphaTestValue());

            // If there's a diffuse stage, apply the correct texture
            OpenGLState::SetTextureState(state.texture0, depthFillPass->state().texture0, GL_TEXTURE0, GL_TEXTURE_2D);

            // Set evaluated stage texture transformation matrix to the GLSL uniform
            program.setDiffuseTextureTransform(depthFillPass->getDiffuseTextureTransform());

            for (const auto& object : objects)
            {
                // We submit all objects with an identity matrix in a single multi draw call
                if (!object.get().isOriented())
                {
                    untransformedObjects.push_back(object.get().getStorageLocation());
                    continue;
                }

                program.setObjectTransform(object.get().getObjectTransform());

                ObjectRenderer::SubmitGeometry(object.get().getStorageLocation(), GL_TRIANGLES, _store);
                ++_drawCalls;
            }

            if (!untransformedObjects.empty())
            {
                program.setObjectTransform(Matrix4::getIdentity());

                ObjectRenderer::SubmitGeometry(untransformedObjects, GL_TRIANGLES, _store);
                ++_drawCalls;

                untransformedObjects.clear();
            }
        }
    }
}

void LightInteractions::drawInteractions(OpenGLState& state, GLSLBumpProgram& program, 
    RenderStateFlags globalFlagsMask, const IRenderView& view, std::size_t renderTime)
{
    auto worldToLight = _light.getLightTextureTransformation();

    std::vector<IGeometryStore::Slot> untransformedObjects;
    untransformedObjects.reserve(10000);

    program.setModelViewProjection(view.GetViewProjection());

    for (const auto& [entity, objectsByShader] : _objectsByEntity)
    {
        for (const auto& [shader, objects] : objectsByShader)
        {
            const auto pass = shader->getInteractionPass();

            if (!pass || !pass->stateIsActive()) continue;

            // Apply our state to the current state object
            pass->evaluateStagesAndApplyState(state, globalFlagsMask, renderTime, entity);

            // Load stage texture matrices
            program.setDiffuseTextureTransform(pass->getDiffuseTextureTransform());
            program.setBumpTextureTransform(pass->getBumpTextureTransform());
            program.setSpecularTextureTransform(pass->getSpecularTextureTransform());

            for (const auto& object : objects)
            {
                // We submit all objects with an identity matrix in a single multi draw call
                if (!object.get().isOriented())
                {
                    untransformedObjects.push_back(object.get().getStorageLocation());
                    continue;
                }

                OpenGLShaderPass::SetUpLightingCalculation(state, &_light, worldToLight,
                    view.getViewer(), object.get().getObjectTransform(), renderTime);

                pass->getProgram().setObjectTransform(object.get().getObjectTransform());

                ObjectRenderer::SubmitGeometry(object.get().getStorageLocation(), GL_TRIANGLES, _store);
                ++_drawCalls;
            }

            if (!untransformedObjects.empty())
            {
                OpenGLShaderPass::SetUpLightingCalculation(state, &_light, worldToLight,
                    view.getViewer(), Matrix4::getIdentity(), renderTime);

                pass->getProgram().setObjectTransform(Matrix4::getIdentity());

                ObjectRenderer::SubmitGeometry(untransformedObjects, GL_TRIANGLES, _store);
                ++_drawCalls;

                untransformedObjects.clear();
            }
        }
    }
}

}
