#include "MessageBox.h"

#include "imainframe.h"
#include "itextstream.h"
#include "i18n.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/frame.h>

namespace wxutil
{

Messagebox::Messagebox(const std::string& title, const std::string& text,
					   ui::IDialog::MessageType type, wxWindow* parent) :
	_dialog(new wxMessageDialog(parent != NULL ? parent : GlobalMainFrame().getWxTopLevelWindow(), 
			text, title, getDialogStyle(type)))
{
	if (type == ui::IDialog::MESSAGE_SAVECONFIRMATION)
	{
		_dialog->SetYesNoLabels(_("Save"), _("Close without saving"));
	}
}

Messagebox::~Messagebox()
{
	_dialog->Destroy();
}

void Messagebox::setTitle(const std::string& title)
{
	_dialog->SetTitle(title);
}

ui::IDialog::Result Messagebox::run()
{
	int returnCode = _dialog->ShowModal();

	// Map the wx result codes to ui::IDialog namespace
	switch (returnCode)
	{
		case wxID_OK:		return ui::IDialog::RESULT_OK;
		case wxID_CANCEL:	return ui::IDialog::RESULT_CANCELLED;
		case wxID_YES:		return ui::IDialog::RESULT_YES;
		case wxID_NO:		return ui::IDialog::RESULT_NO;
		default:			return ui::IDialog::RESULT_CANCELLED;
	};
}

long Messagebox::getDialogStyle(ui::IDialog::MessageType type)
{
	long style = wxCENTER;

	switch (type)
	{
		case ui::IDialog::MESSAGE_CONFIRM:
			style |= wxOK | wxICON_INFORMATION | wxOK_DEFAULT;
			break;
		case ui::IDialog::MESSAGE_ASK:
			style |= wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT;
			break;
		case ui::IDialog::MESSAGE_WARNING:
			style |= wxOK | wxICON_WARNING | wxOK_DEFAULT;
			break;
		case ui::IDialog::MESSAGE_ERROR:
			style |= wxOK | wxICON_ERROR | wxOK_DEFAULT;
			break;
		case ui::IDialog::MESSAGE_YESNOCANCEL:
			style |= wxYES_NO | wxCANCEL | wxICON_QUESTION | wxYES_DEFAULT;
			break;
        case ui::IDialog::MESSAGE_SAVECONFIRMATION:
			style |= wxYES_NO | wxCANCEL | wxICON_WARNING | wxYES_DEFAULT;
			break;
	};

	return style;
}

ui::IDialog::Result Messagebox::Show(const std::string& title,
	const std::string& text, ui::IDialog::MessageType type, wxWindow* parent)
{
	Messagebox msg(title, text, type, parent);
	
	return msg.run();
}

void Messagebox::ShowError(const std::string& errorText, wxWindow* parent)
{
	Messagebox msg("Error", errorText, ui::IDialog::MESSAGE_ERROR, parent);
	msg.run();
}

void Messagebox::ShowFatalError(const std::string& errorText, wxWindow* parent)
{
	ShowError(errorText, parent);
	abort();
}

} // namespace

#if 0
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/table.h>
#include <gdk/gdkkeysyms.h>

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignment.h"

namespace gtkutil
{

Messagebox::Messagebox(const std::string& title, const std::string& text,
					   IDialog::MessageType type, const Glib::RefPtr<Gtk::Window>& parent) :
	Dialog(title, parent),
	_text(text),
	_type(type)
{}

// Constructs the dialog (adds buttons, text and icons)
void Messagebox::construct()
{
	// Base class is adding the buttons
	Dialog::construct();

	// Add an hbox for the icon and the content
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 12));
	_vbox->pack_start(*hbox, false, false, 0);

	// Add the icon
	Gtk::Widget* icon = createIcon();

	if (icon != NULL)
	{
		hbox->pack_start(*icon, false, false, 0);
	}

	// Add the text
	hbox->pack_start(
        *Gtk::manage(new gtkutil::LeftAlignedLabel(_text)), true, true, 0
    );
}

Gtk::Widget* Messagebox::createIcon()
{
	Gtk::BuiltinStockID stockId;

	switch (_type)
	{
	case MESSAGE_CONFIRM:
		stockId = Gtk::Stock::DIALOG_INFO;
		break;
	case MESSAGE_ASK:
		stockId = Gtk::Stock::DIALOG_QUESTION;
		break;
	case MESSAGE_WARNING:
		stockId = Gtk::Stock::DIALOG_WARNING;
		break;
	case MESSAGE_ERROR:
		stockId = Gtk::Stock::DIALOG_ERROR;
		break;
	default:
		stockId = Gtk::Stock::DIALOG_INFO;
	};

	return Gtk::manage(new Gtk::Image(stockId, Gtk::ICON_SIZE_DIALOG));
}

// Override Dialog::createButtons() to add the custom ones
Gtk::Widget& Messagebox::createButtons()
{
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(true, 6));

	if (   _type == MESSAGE_CONFIRM
        || _type == MESSAGE_WARNING
        || _type == MESSAGE_ERROR)
	{
		// Add an OK button
		Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
		okButton->signal_clicked().connect(sigc::mem_fun(*this, &Messagebox::onOK));
		buttonHBox->pack_end(*okButton, true, true, 0);

		mapKeyToButton(GDK_O, *okButton);
		mapKeyToButton(GDK_Return, *okButton);
		mapKeyToButton(GDK_Escape, *okButton);
	}
	else if (_type == MESSAGE_ASK)
	{
		// YES button
		Gtk::Button* yesButton = Gtk::manage(new Gtk::Button(Gtk::Stock::YES));
		yesButton->signal_clicked().connect(sigc::mem_fun(*this, &Messagebox::onYes));
		buttonHBox->pack_end(*yesButton, true, true, 0);

		mapKeyToButton(GDK_Y, *yesButton);
		mapKeyToButton(GDK_Return, *yesButton);

		// NO button
		Gtk::Button* noButton = Gtk::manage(new Gtk::Button(Gtk::Stock::NO));
		noButton->signal_clicked().connect(sigc::mem_fun(*this, &Messagebox::onNo));
		buttonHBox->pack_end(*noButton, true, true, 0);

		mapKeyToButton(GDK_N, *noButton);
		mapKeyToButton(GDK_Escape, *noButton);
	}
	else if (   _type == MESSAGE_YESNOCANCEL
             || _type == MESSAGE_SAVECONFIRMATION)
	{
        bool isYesNo = (_type == MESSAGE_YESNOCANCEL);

		// Cancel button
		Gtk::Button* cancelButton = Gtk::manage(
            new Gtk::Button(Gtk::Stock::CANCEL)
        );
		cancelButton->signal_clicked().connect(
            sigc::mem_fun(*this, &Messagebox::onCancel)
        );

		mapKeyToButton(GDK_Escape, *cancelButton);
		mapKeyToButton(GDK_C, *cancelButton);

		// NO button / "Close without saving" button
		Gtk::Button* noButton = Gtk::manage(
            isYesNo
            ? new Gtk::Button(Gtk::Stock::NO)
            : new Gtk::Button(_("Close without saving"))
        );
		noButton->signal_clicked().connect(
            sigc::mem_fun(*this, &Messagebox::onNo)
        );

		mapKeyToButton(isYesNo ? GDK_N : GDK_W, *noButton);

		// YES button / Save button
		Gtk::Button* yesButton = Gtk::manage(
            new Gtk::Button(isYesNo ? Gtk::Stock::YES : Gtk::Stock::SAVE)
        );
		yesButton->signal_clicked().connect(
            sigc::mem_fun(*this, &Messagebox::onYes)
        );

		mapKeyToButton(isYesNo ? GDK_Y : GDK_S, *yesButton);
		mapKeyToButton(GDK_Return, *yesButton);

        // Pack buttons
        buttonHBox->pack_start(
            isYesNo ? *cancelButton : *noButton, true, true, 0
        );
		buttonHBox->pack_start(
            isYesNo ? *noButton : *cancelButton, true, true, 0
        );
		buttonHBox->pack_start(*yesButton, true, true, 0);

	}
	else
	{
		rError() << "Invalid message type encountered: " << _type << std::endl;
	}

	return *Gtk::manage(new RightAlignment(*buttonHBox));
}

void Messagebox::onYes()
{
	_result = ui::IDialog::RESULT_YES;
	hide(); // breaks gtk_main()
}

void Messagebox::onNo()
{
	_result = ui::IDialog::RESULT_NO;
	hide(); // breaks gtk_main()
}

#if 0
void Messagebox::ShowError(const std::string& errorText, const Glib::RefPtr<Gtk::Window>& mainFrame)
{
	Messagebox msg("Error", errorText, Messagebox::MESSAGE_ERROR, mainFrame);
	msg.run();
}

void Messagebox::ShowFatalError(const std::string& errorText, const Glib::RefPtr<Gtk::Window>& mainFrame)
{
	ShowError(errorText, mainFrame);
	abort();
}
#endif

} // namespace gtkutil
#endif