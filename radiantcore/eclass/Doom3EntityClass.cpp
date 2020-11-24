#include "Doom3EntityClass.h"

#include "itextstream.h"
#include "os/path.h"
#include "string/convert.h"

#include "string/predicate.h"
#include <fmt/format.h>
#include <functional>

namespace eclass
{

namespace
{

// Constants
const std::string DEF_ATTACH = "def_attach";
const std::string NAME_ATTACH = "name_attach";
const std::string POS_ATTACH = "pos_attach";

const std::string ATTACH_POS_NAME = "attach_pos_name";
const std::string ATTACH_POS_ORIGIN = "attach_pos_origin";
const std::string ATTACH_POS_JOINT = "attach_pos_joint";
const std::string ATTACH_POS_ANGLES = "attach_pos_angles";

// Extract and return the string suffix for a key (which might be the empty
// string if there is no suffix). Returns false if the key did not match
// the prefix.
bool tryGetSuffixedKey(const std::string& key, const std::string& prefix, std::string& suffixedOutput)
{
    if (string::istarts_with(key, prefix))
    {
		suffixedOutput = key.substr(prefix.length());
		return true;
    }
    
	suffixedOutput.clear();
	return false;
}

} // namespace

// Attachment helper object
class Doom3EntityClass::Attachments
{
    // Name of the entity class being parsed (for debug/error purposes)
    std::string _parentClassname;

    // Any def_attached entities. Each attachment has an entity class, a
    // position and optionally a name.
    struct Attachment
    {
        // Class of entity that is attached
        std::string className;

        // Name of the entity that is attached
        std::string name;

        // Name of the position (AttachPos) at which the entity should be
        // attached
        std::string posName;
    };

    // Attached object map initially indexed by key suffix (e.g. "1" for
    // "name_attach1"), then by name.
    typedef std::map<std::string, Attachment> AttachedObjects;
    AttachedObjects _objects;

    // Positions at which def_attached entities can be attached.
    struct AttachPos
    {
        // Name of this attachment position (referred to in the
        // Attachment::posName variable)
        std::string name;

        // 3D offset position from our origin or the model joint, if a joint is
        // specified
        Vector3 origin;

        // Rotation of the attached entity
        Vector3 angles;

        // Optional model joint relative to which the origin should be
        // calculated
        std::string joint;
    };

    // Attach position map initially indexed by key suffix (e.g. "_zhandr" for
    // "attach_pos_name_zhandr"), then by name. It appears that only attachpos
    // keys are using arbitrary strings instead of numeric suffixes, but we
    // might as well treat everything the same way.
    typedef std::map<std::string, AttachPos> AttachPositions;
    AttachPositions _positions;

private:

    template<typename Map> void reindexMapByName(Map& inputMap)
    {
        Map copy(inputMap);
        inputMap.clear();

        // Take each item from the copied map, and insert it into the original
        // map using the name as the key.
        for (typename Map::value_type pair : copy)
        {
            if (!pair.second.name.empty()) // ignore empty names
            {
                inputMap.insert(
                    typename Map::value_type(pair.second.name, pair.second)
                );
            }
        }
    }

public:

    // Initialise and set classname
    Attachments(const std::string& name)
    : _parentClassname(name)
    { }

    // Clear all data
    void clear()
    {
        _objects.clear();
        _positions.clear();
    }

    // Attempt to extract attachment data from the given key/value pair
    void parseDefAttachKeys(const std::string& key, const std::string& value)
    {
        std::string suffix;

        if (tryGetSuffixedKey(key, DEF_ATTACH, suffix))
        {
            _objects[suffix].className = value;
        }
        else if (tryGetSuffixedKey(key, NAME_ATTACH, suffix))
        {
            _objects[suffix].name = value;
        }
        else if (tryGetSuffixedKey(key, POS_ATTACH, suffix))
        {
            _objects[suffix].posName = value;
        }
        else if (tryGetSuffixedKey(key, ATTACH_POS_NAME, suffix))
        {
            _positions[suffix].name = value;
        }
        else if (tryGetSuffixedKey(key, ATTACH_POS_ORIGIN, suffix))
        {
            _positions[suffix].origin = string::convert<Vector3>(value);
        }
        else if (tryGetSuffixedKey(key, ATTACH_POS_ANGLES, suffix))
        {
            _positions[suffix].angles = string::convert<Vector3>(value);
        }
        else if (tryGetSuffixedKey(key, ATTACH_POS_JOINT, suffix))
        {
            _positions[suffix].joint = value;
        }
    }

    // Post-process after attachment parsing
    void validateAttachments()
    {
        // During parsing we indexed spawnargs by string suffix so that matching
        // keys could be found. From now on we are no longer interested in the
        // suffixes so we will re-build the maps indexed by name instead.
        reindexMapByName(_objects);
        reindexMapByName(_positions);

        // Drop any attached objects that specify a non-existent position (I
        // assume new positions cannot be dynamically created in game).
        for (AttachedObjects::iterator i = _objects.begin();
             i != _objects.end();
             /* in-loop increment */)
        {
            if (_positions.find(i->second.posName) == _positions.end())
            {
                rWarning()
                    << "[eclassmgr] Entity class '" << _parentClassname 
                    << "' tries to attach '" << i->first << "' at non-existent "
                    << "position '" << i->second.posName << "'\n";

                _objects.erase(i++);
            }
			else
			{
				++i;
			}
        }
    }
};

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
  _attachments(new Attachments(name)),
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

/** Query whether this entity has a fixed size.
 */
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
    // (Re)set the colour
    const EntityClassAttribute& colourAttr = getAttribute("editor_color");

    if (!colourAttr.getValue().empty())
    {
        setColour(string::convert<Vector3>(colourAttr.getValue()));
    }
    else
    {
        // If no colour is set, assign the default entity colour to this class
        setColour(DefaultEntityColour);
    }
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
    vfs::FileInfo emptyFileInfo{ "def/", "_autogenerated_by_darkradiant_.def" };
    return std::make_shared<Doom3EntityClass>(name, emptyFileInfo, !brushes);
}

// Enumerate entity class attributes
void Doom3EntityClass::forEachClassAttribute(
    std::function<void(const EntityClassAttribute&)> visitor,
    bool editorKeys) const
{
    for (EntityAttributeMap::const_iterator i = _attributes.begin();
         i != _attributes.end();
         ++i)
    {
        // Visit if it is a non-editor key or we are visiting all keys
        if (editorKeys || !string::istarts_with(*i->first, "editor_"))
        {
            visitor(i->second);
        }
    }
}

namespace
{
    void copyInheritedAttribute(Doom3EntityClass* target,
                                const EntityClassAttribute& attr)
    {
        target->addAttribute(EntityClassAttribute(attr, true));
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

        // Copy attributes from the parent to the child, including editor keys
        pIter->second->forEachClassAttribute(
            std::bind(&copyInheritedAttribute, this, std::placeholders::_1), true
        );

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

    resetColour();
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
EntityClassAttribute& Doom3EntityClass::getAttribute(const std::string& name)
{
    StringPtr ref(new std::string(name));

    EntityAttributeMap::iterator f = _attributes.find(ref);

    return (f != _attributes.end()) ? f->second : _emptyAttribute;
}

// Find a single attribute
const EntityClassAttribute& Doom3EntityClass::getAttribute(const std::string& name) const
{
    StringPtr ref(new std::string(name));

    EntityAttributeMap::const_iterator f = _attributes.find(ref);

    return (f != _attributes.end()) ? f->second : _emptyAttribute;
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

    _attachments->clear();
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

        // Try parsing this key/value with the Attachments manager
        _attachments->parseDefAttachKeys(key, value);

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

    _attachments->validateAttachments();

    // Notify the observers
    _changedSignal.emit();
}

} // namespace eclass
