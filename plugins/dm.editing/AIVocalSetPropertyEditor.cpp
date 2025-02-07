#include "AIVocalSetPropertyEditor.h"

#include "i18n.h"
#include "ieclass.h"
#include "ientity.h"

#include <wx/panel.h>
#include <wx/button.h>
#include "wxutil/Bitmap.h"
#include <wx/sizer.h>

#include "AIVocalSetChooserDialog.h"

namespace ui
{

AIVocalSetPropertyEditor::AIVocalSetPropertyEditor(wxWindow* parent, IEntitySelection& entities, const std::string& key, const std::string& options) :
	_entities(entities)
{
	// Construct the main widget (will be managed by the base class)
	_widget = new wxPanel(parent, wxID_ANY);
	_widget->SetSizer(new wxBoxSizer(wxHORIZONTAL));

	// Create the browse button
	wxButton* browseButton = new wxButton(_widget, wxID_ANY, _("Select Vocal Set..."));
	browseButton->SetBitmap(wxutil::GetLocalBitmap("icon_sound.png"));
	browseButton->Bind(wxEVT_BUTTON, &AIVocalSetPropertyEditor::onChooseButton, this);

	_widget->GetSizer()->Add(browseButton, 0, wxALIGN_CENTER_VERTICAL);
}

AIVocalSetPropertyEditor::~AIVocalSetPropertyEditor()
{
	if (_widget != nullptr)
	{
		_widget->Destroy();
	}
}

wxPanel* AIVocalSetPropertyEditor::getWidget()
{
	return _widget;
}

void AIVocalSetPropertyEditor::updateFromEntities()
{
	// Nothing to do
}

IPropertyEditor::Ptr AIVocalSetPropertyEditor::CreateNew(wxWindow* parent, IEntitySelection& entities,
	const std::string& key, const std::string& options)
{
	return std::make_shared<AIVocalSetPropertyEditor>(parent, entities, key, options);
}

void AIVocalSetPropertyEditor::onChooseButton(wxCommandEvent& ev)
{
	// Construct a new vocal set chooser dialog
	AIVocalSetChooserDialog* dialog = new AIVocalSetChooserDialog;

	dialog->setSelectedVocalSet(_entities.getSharedKeyValue(DEF_VOCAL_SET_KEY, true));

	// Show and block
	if (dialog->ShowModal() == wxID_OK)
	{
        _entities.foreachEntity([&](Entity* entity)
        {
            entity->setKeyValue(DEF_VOCAL_SET_KEY, dialog->getSelectedVocalSet());
        });
	}

	dialog->Destroy();
}

std::string AIVocalSetEditorDialogWrapper::runDialog(Entity* entity, const std::string& key)
{
    // Construct a new vocal set chooser dialog
    AIVocalSetChooserDialog* dialog = new AIVocalSetChooserDialog;

    std::string oldValue = entity->getKeyValue(DEF_VOCAL_SET_KEY);
    dialog->setSelectedVocalSet(oldValue);

    // Show and block
    std::string rv = oldValue;

    if (dialog->ShowModal() == wxID_OK)
    {
        rv = dialog->getSelectedVocalSet();
    }

    dialog->Destroy();

    return rv;
}

} // namespace ui
