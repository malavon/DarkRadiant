#include "Texturing.h"

#include "selection/textool/FaceNode.h"
#include "selection/textool/PatchNode.h"

namespace selection
{

namespace algorithm
{

namespace
{

inline void applyTransform(const textool::INode::Ptr& node, const Matrix3& transform)
{
    node->beginTransformation();
    node->transform(transform);
    node->commitTransformation();
}

}

TextureNodeManipulator::operator std::function<bool(const textool::INode::Ptr& node)>()
{
    return [this](const textool::INode::Ptr& node)
    {
        return processNode(node);
    };
}

bool TextureBoundsAccumulator::processNode(const textool::INode::Ptr& node)
{
    _bounds.includeAABB(node->localAABB());
    return true;
}

TextureFlipper::TextureFlipper(const Vector2& flipCenter, int axis)
{
    auto flipMatrix = Matrix3::getIdentity();

    if (axis == 0)
    {
        flipMatrix.xx() = -1;
    }
    else // axis == 1
    {
        flipMatrix.yy() = -1;
    }

    _transform = Matrix3::getTranslation(-flipCenter);
    _transform.premultiplyBy(flipMatrix);
    _transform.premultiplyBy(Matrix3::getTranslation(+flipCenter));
}

bool TextureFlipper::processNode(const textool::INode::Ptr& node)
{
    applyTransform(node, _transform);
    return true;
}

void TextureFlipper::FlipNode(const textool::INode::Ptr& node, int flipAxis)
{
    const auto& bounds = node->localAABB();
    TextureFlipper flipper({ bounds.origin.x(), bounds.origin.y() }, flipAxis);

    flipper.processNode(node);
}

void TextureFlipper::FlipPatch(IPatch& patch, int flipAxis)
{
    FlipNode(std::make_shared<textool::PatchNode>(patch), flipAxis);
}

void TextureFlipper::FlipFace(IFace& face, int flipAxis)
{
    FlipNode(std::make_shared<textool::FaceNode>(face), flipAxis);
}

// Rotation

TextureRotator::TextureRotator(const Vector2& pivot, double angle)
{
    _transform = Matrix3::getTranslation(-pivot);
    _transform.premultiplyBy(Matrix3::getRotation(angle));
    _transform.premultiplyBy(Matrix3::getTranslation(pivot));
}

bool TextureRotator::processNode(const textool::INode::Ptr& node)
{
    applyTransform(node, _transform);
    return true;
}

void TextureRotator::RotatePatch(IPatch& patch, double angle)
{
    RotateNode(std::make_shared<textool::PatchNode>(patch), angle);
}

void TextureRotator::RotateFace(IFace& face, double angle)
{
    RotateNode(std::make_shared<textool::FaceNode>(face), angle);
}

void TextureRotator::RotateNode(const textool::INode::Ptr& node, double angle)
{
    const auto& bounds = node->localAABB();
    TextureRotator rotator({ bounds.origin.x(), bounds.origin.y() }, angle);

    rotator.processNode(node);
}

}

}
