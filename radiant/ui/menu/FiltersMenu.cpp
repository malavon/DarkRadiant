#include "FiltersMenu.h"

#include <gtk/gtkwidget.h>

#include "ifilter.h"
#include "iuimanager.h"

namespace ui {
	
	namespace {
		// greebo: These are used for the DarkRadiant main menu
		const std::string MENU_NAME = "main";
		const std::string MENU_INSERT_BEFORE = MENU_NAME + "/map";
		const std::string MENU_FILTERS_NAME = "filters";
		const std::string MENU_PATH = MENU_NAME + "/" + MENU_FILTERS_NAME;
		const std::string MENU_ICON = "iconFilter16.png";

		// These are used for the general-purpose Filter Menu:
		const std::string FILTERS_MENU_BAR = "filters";
		const std::string FILTERS_MENU_FOLDER = "allfilters";
		const std::string FILTERS_MENU_PATH = FILTERS_MENU_BAR + "/" + FILTERS_MENU_FOLDER;
		const std::string FILTERS_MENU_CAPTION = "_Filters";

		// Local visitor class to populate the filters menu
		class MenuPopulatingVisitor : 
			public IFilterVisitor
		{
			// The path under which the items get added.
			std::string _targetPath; 
		public:
			// Pass the target menu path to the constructor
			MenuPopulatingVisitor(const std::string& targetPath) :
				_targetPath(targetPath)
			{}

			// Visitor function
			void visit(const std::string& filterName) {
				// Get the menu manager
				IMenuManager& menuManager = GlobalUIManager().getMenuManager();
		
				std::string eventName = 
					GlobalFilterSystem().getFilterEventName(filterName);

				// Create the toplevel menu item
				menuManager.add(_targetPath, _targetPath + "_" + filterName, 
								menuItem, filterName, 
								MENU_ICON, eventName);
			}	
		};
	}

FiltersMenu::FiltersMenu() {
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();

	_menu = menuManager.get(FILTERS_MENU_BAR);

	if (_menu == NULL)
	{
		// Menu not yet constructed, do it now
		// Create the menu bar first
		_menu = menuManager.add("", FILTERS_MENU_BAR, menuBar, "Filters", "", "");
	
		// Create the folder as child of the bar
		menuManager.add(FILTERS_MENU_BAR, FILTERS_MENU_FOLDER, 
						menuFolder, FILTERS_MENU_CAPTION, "", "");
		
		// Visit the filters in the FilterSystem to populate the menu
		MenuPopulatingVisitor visitor(FILTERS_MENU_PATH);
		GlobalFilterSystem().forEachFilter(visitor);
	}
}

FiltersMenu::operator GtkWidget*() {
	return _menu;
}

// Construct GTK widgets
void FiltersMenu::addItems() {
	// Get the menu manager
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();

	// remove any items first
	removeItems();

	// Create the toplevel menu item
	menuManager.insert(MENU_INSERT_BEFORE, MENU_FILTERS_NAME, 
						ui::menuFolder, "Fi_lter", "", ""); // empty icon, empty event
	
	// Visit the filters in the FilterSystem to populate the menu
	MenuPopulatingVisitor visitor(MENU_PATH);
	GlobalFilterSystem().forEachFilter(visitor);
}

void FiltersMenu::removeItems() {
	// Get the menu manager
	IMenuManager& menuManager = GlobalUIManager().getMenuManager();

	// Remove the filters menu if there exists one
	menuManager.remove(MENU_NAME + "/" + MENU_FILTERS_NAME);
}

} // namespace ui
