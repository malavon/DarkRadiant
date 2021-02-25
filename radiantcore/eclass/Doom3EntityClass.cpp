#include "Doom3EntityClass.h"

#include "itextstream.h"
#include "os/path.h"
#include "string/convert.h"

#include "string/predicate.h"
#include <fmt/format.h>
#include <functional>

namespace eclass
{

const std::string Doom3EntityClass::DefaultWireShader("<0.3 0.3 1>");
const std::string Doom3EntityClass::DefaultFillShader("(0.3 0.3 1)");
const Vector3 Doom3EntityClass::DefaultEntityColour(0.3, 0.3, 1);

Doom3EntityClass::Doom3EntityClass(const std::string& name, const vfs::FileInfo& fileInfo) :
    Doom3EntityClass(name, fileInfo, false)
{}

Doom3EntityClass::Doom3EntityClass(const std::string& name, const vfs::FileInfo& fileInfo, bool fixedSize)
: _name(name),
  _fileInfo(fileInfo),
  _parent(nullptr),
  _isLight(false),
  _colour(-1, -1, -1),
  _colourTransparent(false),
  _fixedSize(fixedSize),
  _model(""),
  _skin(""),
  _inheritanceResolved(false),
  _modName("base"),
  _emptyAttribute("", "", ""),
  _parseStamp(0)
{}

Doom3EntityClass::~Doom3EntityClass()
{}

std::string Doom3EntityClass::getName() const
{
    return _name;
}

const IEntityClass* Doom3EntityClass::getParent() const
{
    return _parent;
}

sigc::signal<void>& Doom3EntityClass::changedSignal()
{
    return _changedSignal;
}

bool Doom3EntityClass::isFixedSize() const
{
    if (_fixedSize) {
        return true;
    }
    else {
        // Check for the existence of editor_mins/maxs attributes, and that
        // they do not contain only a question mark
        return (getAttribute("editor_mins").getValue().size() > 1
                && getAttribute("editor_maxs").getValue().size() > 1);
    }
}

AABB Doom3EntityClass::getBounds() const
{
    if (isFixedSize())
    {
        return AABB::createFromMinMax(
            string::convert<Vector3>(getAttribute("editor_mins").getValue()),
            string::convert<Vector3>(getAttribute("editor_maxs").getValue())
        );
    }
    else
    {
        return AABB(); // null AABB
    }
}

bool Doom3EntityClass::isLight() const
{
    return _isLight;
}

void Doom3EntityClass::setIsLight(bool val)
{
    _isLight = val;
    if (_isLight)
        _fixedSize = true;
}

void Doom3EntityClass::setColour(const Vector3& colour)
{
    _colour = colour;

    // Set the entity colour to default, if none was specified
    if (_colour == Vector3(-1, -1, -1))
    {
        _colour = DefaultEntityColour;
    }

    // Define fill and wire versions of the entity colour
    _fillShader = _colourTransparent ?
        fmt::format("[{0:f} {1:f} {2:f}]", _colour[0], _colour[1], _colour[2]) :
        fmt::format("({0:f} {1:f} {2:f})", _colour[0], _colour[1], _colour[2]);

    _wireShader = fmt::format("<{0:f} {1:f} {2:f}>", _colour[0], _colour[1], _colour[2]);

    _changedSignal.emit();
}

void Doom3EntityClass::resetColour()
{
    // Look for an editor_color on this class only
    const EntityClassAttribute& attr = getAttribute("editor_color", false);
    if (!attr.getValue().empty())
    {
        return setColour(string::convert<Vector3>(attr.getValue()));
    }

    // If there is a parent, use its getColour() directly, rather than its
    // editor_color attribute, to take into account overrides through the
    // EClassColourManager
    if (_parent)
        return setColour(_parent->getColour());

    // No parent and no attribute, all we can use is the default colour
    setColour(DefaultEntityColour);
}

const Vector3& Doom3EntityClass::getColour() const {
    return _colour;
}

const std::string& Doom3EntityClass::getWireShader() const
{
    // Use a fallback shader colour in case we don't have anything
    return !_wireShader.empty() ? _wireShader : DefaultWireShader;
}

const std::string& Doom3EntityClass::getFillShader() const
{
    // Use a fallback shader colour in case we don't have anything
    return !_fillShader.empty() ? _fillShader : DefaultFillShader;
}

/* ATTRIBUTES */

/**
 * Insert an EntityClassAttribute, without overwriting previous values.
 */
void Doom3EntityClass::addAttribute(const EntityClassAttribute& attribute)
{
    // Try to insert the class attribute
    std::pair<EntityAttributeMap::iterator, bool> result = _attributes.insert(
        EntityAttributeMap::value_type(attribute.getNameRef(), attribute)
    );

    if (!result.second)
    {
        EntityClassAttribute& existing = result.first->second;

        // greebo: Attribute already existed, check if we have some
        // descriptive properties to be added to the existing one.
        if (!attribute.getDescription().empty() && existing.getDescription().empty())
        {
            // Use the shared string reference to save memory
            existing.setDescription(attribute.getDescriptionRef());
        }

        // Check if we have a more descriptive type than "text"
        if (attribute.getType() != "text" && existing.getType() == "text")
        {
            // Use the shared string reference to save memory
            existing.setType(attribute.getTypeRef());
        }
    }
}

Doom3EntityClassPtr Doom3EntityClass::create(const std::string& name, bool brushes)
{
    vfs::FileInfo emptyFileInfo("def/", "_autogenerated_by_darkradiant_.def", vfs::Visibility::HIDDEN);
    return std::make_shared<Doom3EntityClass>(name, emptyFileInfo, !brushes);
}

// Enumerate entity class attributes
void Doom3EntityClass::forEachClassAttribute(
    std::function<void(const EntityClassAttribute&)> visitor,
    bool editorKeys) const
{
    // Visit parent attributes
    if (_parent)
        _parent->forEachClassAttribute(visitor, editorKeys);

    // Visit our own attributes
    for (const auto& pair: _attributes)
    {
        // Visit if it is a non-editor key or we are visiting all keys
        if (editorKeys || !string::istarts_with(*pair.first, "editor_"))
        {
            visitor(pair.second);
        }
    }
}

// Resolve inheritance for this class
void Doom3EntityClass::resolveInheritance(EntityClasses& classmap)
{
    // If we have already resolved inheritance, do nothing
    if (_inheritanceResolved)
        return;

    // Lookup the parent name and return if it is not set. Also return if the
    // parent name is the same as our own classname, to avoid infinite
    // recursion.
    std::string parName = getAttribute("inherit").getValue();
    if (parName.empty() || parName == _name)
        return;

    // Find the parent entity class
    EntityClasses::iterator pIter = classmap.find(parName);
    if (pIter != classmap.end())
    {
        // Recursively resolve inheritance of parent
        pIter->second->resolveInheritance(classmap);

        // Set our parent pointer
        _parent = pIter->second.get();
    }
    else
    {
        rWarning() << "[eclassmgr] Entity class "
                              << _name << " specifies unknown parent class "
                              << parName << std::endl;
    }

    // Set the resolved flag
    _inheritanceResolved = true;

    if (!getAttribute("model").getValue().empty())
    {
        // We have a model path (probably an inherited one)
        setModelPath(getAttribute("model").getValue());
    }

    if (getAttribute("editor_light").getValue() == "1" || getAttribute("spawnclass").getValue() == "idLight")
    {
        // We have a light
        setIsLight(true);
    }

    if (getAttribute("editor_transparent").getValue() == "1")
    {
        _colourTransparent = true;
    }

    // Set up inheritance of entity colours: colours inherit from parent unless
    // there is an explicit editor_color defined at this level
    resetColour();
    if (_parent)
    {
        _parent->changedSignal().connect(
            sigc::mem_fun(this, &Doom3EntityClass::resetColour)
        );
    }
}

bool Doom3EntityClass::isOfType(const std::string& className)
{
	for (const IEntityClass* currentClass = this;
         currentClass != NULL;
         currentClass = currentClass->getParent())
    {
        if (currentClass->getName() == className)
		{
			return true;
		}
    }

	return false;
}

std::string Doom3EntityClass::getDefFileName()
{
    return _fileInfo.fullPath();
}

// Find a single attribute
EntityClassAttribute& Doom3EntityClass::getAttribute(const std::string& name,
                                                     bool includeInherited)
{
    return const_cast<EntityClassAttribute&>(
        std::as_const(*this).getAttribute(name, includeInherited)
    );
}

// Find a single attribute
const EntityClassAttribute&
Doom3EntityClass::getAttribute(const std::string& name,
                               bool includeInherited) const
{
    StringPtr ref(new std::string(name));

    // First look up the attribute on this class; if found, we can simply return it
    auto f = _attributes.find(ref);
    if (f != _attributes.end())
        return f->second;

    // If there is no parent or we have been instructed to ignore inheritance,
    // this is the end of the line: return an empty attribute
    if (!_parent || !includeInherited)
        return _emptyAttribute;

    // Otherwise delegate to the parent (which will recurse until an attribute
    // is found or a null parent ends the process)
    return _parent->getAttribute(name);
}

void Doom3EntityClass::clear()
{
    // Don't clear the name
    _isLight = false;

    _colour = Vector3(-1,-1,-1);
    _colourTransparent = false;

    _fixedSize = false;

    _attributes.clear();
    _model.clear();
    _skin.clear();
    _inheritanceResolved = false;

    _modName = "base";
}

void Doom3EntityClass::parseEditorSpawnarg(const std::string& key,
                                           const std::string& value)
{
    // "editor_yyy" represents an attribute that may be set on this
    // entity. Construct a value-less EntityClassAttribute to add to
    // the class, so that it will show in the entity inspector.

    // Locate the space in "editor_bool myVariable", starting after "editor_"
    std::size_t spacePos = key.find(' ', 7);

    // Only proceed if we have a space (some keys like "editor_displayFolder"
    // don't have spaces)
    if (spacePos != std::string::npos)
    {
        // The part beyond the space is the name of the attribute
        std::string attName = key.substr(spacePos + 1);

        // Get the type by trimming the string left and right
        std::string type = key.substr(7, key.length() - attName.length() - 8);

        if (!attName.empty() && type != "setKeyValue") // Ignore editor_setKeyValue
        {
            // Transform the type into a better format
            if (type == "var" || type == "string")
            {
                type = "text";
            }

            // Construct an attribute with empty value, but with valid
            // description
            addAttribute(EntityClassAttribute(type, attName, "", value));
        }
    }
}

void Doom3EntityClass::parseFromTokens(parser::DefTokeniser& tokeniser)
{
    // Clear this structure first, we might be "refreshing" ourselves from tokens
    clear();

    // Required open brace (the name has already been parsed by the EClassManager)
    tokeniser.assertNextToken("{");

    // Loop over all of the keys in this entitydef
    std::string key;
    while ((key = tokeniser.nextToken()) != "}")
    {
        const std::string value = tokeniser.nextToken();

        // Handle some keys specially
        if (key == "model")
        {
            setModelPath(os::standardPath(value));
        }
        else if (key == "editor_color")
        {
            setColour(string::convert<Vector3>(value));
        }
        else if (key == "editor_light")
        {
            setIsLight(value == "1");
        }
        else if (key == "spawnclass")
        {
            setIsLight(value == "idLight");
        }
        else if (string::istarts_with(key, "editor_"))
        {
            parseEditorSpawnarg(key, value);
        }

        // Add the EntityClassAttribute for this key/val
        if (getAttribute(key).getType().empty())
        {
            // Following key-specific processing, add the keyvalue to the eclass
            EntityClassAttribute attribute("text", key, value, "");

            // Type is empty, attribute does not exist, add it.
            addAttribute(attribute);
        }
        else if (getAttribute(key).getValue().empty())
        {
            // Attribute type is set, but value is empty, set the value.
            getAttribute(key).setValue(value);
        }
        else
        {
            // Both type and value are not empty, emit a warning
            rWarning() << "[eclassmgr] attribute " << key
                << " already set on entityclass " << _name << std::endl;
        }
    } // while true

    // Notify the observers
    _changedSignal.emit();
}

} // namespace eclass
