#include "FreezePointer.h"

#include "Cursor.h"
#include "debugging/debugging.h"
#include "MouseButton.h"

namespace gtkutil
{

void FreezePointer::freeze(const Glib::RefPtr<Gtk::Window>& window, const MotionDeltaFunction& function)
{
	ASSERT_MESSAGE(!_function, "can't freeze pointer");
	
	const Gdk::EventMask mask = 
		Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK | Gdk::BUTTON_MOTION_MASK | 
		Gdk::BUTTON1_MOTION_MASK | Gdk::BUTTON2_MOTION_MASK | Gdk::BUTTON3_MOTION_MASK |
		Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::VISIBILITY_NOTIFY_MASK;

    // Hide cursor and grab the pointer
    Gdk::Cursor blank(Gdk::BLANK_CURSOR);
    window->get_window()->pointer_grab(true, mask, blank, GDK_CURRENT_TIME);

	Cursor::ReadPosition(window, _freezePosX, _freezePosY);
	Cursor::SetPosition(window, _freezePosX,_freezePosY);
		
	_function = function;

	_motionHandler = window->signal_motion_notify_event().connect(
		sigc::bind(sigc::mem_fun(*this, &FreezePointer::_onMouseMotion), window));
}

void FreezePointer::unfreeze(const Glib::RefPtr<Gtk::Window>& window)
{
	_motionHandler.disconnect();
	_function = MotionDeltaFunction();

	Cursor::SetPosition(window, _freezePosX,_freezePosY);
	
	Gdk::Window::pointer_ungrab(GDK_CURRENT_TIME);
}

bool FreezePointer::_onMouseMotion(GdkEventMotion* ev, const Glib::RefPtr<Gtk::Window>& window)
{
	int current_x, current_y;
	Cursor::ReadPosition(window, current_x, current_y);
		
	int dx = current_x - _freezePosX;
	int dy = current_y - _freezePosY;

	if (dx != 0 || dy != 0)
	{
		Cursor::SetPosition(window, _freezePosX, _freezePosY);

		if (_function)
		{
			_function(dx, dy, ev->state);
		}
	}

	return FALSE;
}

} // namespace

namespace wxutil
{

void FreezePointer::freeze(wxWindow& window, const MotionDeltaFunction& motionDelta, const CaptureLostFunction& captureLost)
{
	ASSERT_MESSAGE(motionDelta, "can't freeze pointer");
	ASSERT_MESSAGE(captureLost, "can't freeze pointer");
	
    // Hide cursor and grab the pointer
	window.SetCursor(wxCursor(wxCURSOR_BLANK)); 

	window.CaptureMouse();

	wxPoint windowMousePos = window.ScreenToClient(wxGetMousePosition());

	_freezePosX = windowMousePos.x;
	_freezePosY = windowMousePos.y;

	window.WarpPointer(_freezePosX, _freezePosY);

	_motionDelta = motionDelta;
	_captureLost = captureLost;

	window.Connect(wxEVT_MOTION, wxMouseEventHandler(FreezePointer::onMouseMotion), &window, this);
	window.Connect(wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(FreezePointer::onMouseCaptureLost), &window, this);
}

void FreezePointer::unfreeze(wxWindow& window)
{
	window.Disconnect(wxEVT_MOTION, wxMouseEventHandler(FreezePointer::onMouseMotion), &window, this);
	window.Disconnect(wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(FreezePointer::onMouseCaptureLost), &window, this);

	_motionDelta = MotionDeltaFunction();
	_captureLost = CaptureLostFunction();

	window.WarpPointer(_freezePosX, _freezePosY);
	
	window.ReleaseMouse();
}

void FreezePointer::onMouseMotion(wxMouseEvent& ev)
{
	wxWindow* window = static_cast<wxWindow*>(ev.GetEventUserData());

	wxPoint windowMousePos = window->ScreenToClient(wxGetMousePosition());
		
	int dx = windowMousePos.x - _freezePosX;
	int dy = windowMousePos.y - _freezePosY;

	if (dx != 0 || dy != 0)
	{
		window->WarpPointer(_freezePosX, _freezePosY);

		if (_motionDelta)
		{
			_motionDelta(dx, dy, MouseButton::GetStateForMouseEvent(ev));
		}
	}
}

void FreezePointer::onMouseCaptureLost(wxMouseCaptureLostEvent& ev)
{
	wxWindow* window = static_cast<wxWindow*>(ev.GetEventUserData());

	window->ReleaseMouse();

	if (_captureLost)
	{
		_captureLost();
	}
}

} // namespace
