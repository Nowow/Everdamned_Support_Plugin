#include "bloodmeter.h"

namespace logger = SKSE::log;

RE::BSString current_widget_root;

void CommunicateCurrentWidgetRoot(RE::StaticFunctionTag*, RE::BSString widget_root) {
    if (widget_root == nullptr) {
        logger::debug("Blood Meter Widget root communicated is null");
        return;
    }
    current_widget_root = widget_root;
}


void SetBloodMeterPercent() {

    if (current_widget_root == nullptr) {
        logger::debug("Current meter root not defined, doing nothing");
        return;
    }
	auto ui = RE::UI::GetSingleton();
    if (!ui || !ui->IsMenuOpen(RE::HUDMenu::MENU_NAME)) return;

    auto hud = ui->GetMenu<RE::HUDMenu>();
    if (!hud || !hud->uiMovie) return;

    RE::GPtr<RE::GFxMovieView> view = hud->uiMovie;
 
    RE::GFxValue args[2];
    args[0].SetNumber(0.35f);
    args[1].SetNumber(0.0f);
    
    view->Invoke(current_widget_root.c_str(), nullptr, args, 1);

}