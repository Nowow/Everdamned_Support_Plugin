#include "bloodmeter.h"
#include "include/API_ActorValueGenerator.h"
#include <thread>

namespace logger = SKSE::log;

RE::BSString current_widget_root = "_root.WidgetContainer.7.widget";
constexpr std::string_view bloodPoolAVname = "ED_BloodPool";
//std::atomic<bool> bloodMeterIsUpdating{false};
bool bloodMeterIsUpdating = false;
std::thread bloodMeterUpdateThread;


void CommunicateCurrentWidgetRoot(RE::StaticFunctionTag*, RE::BSString widget_root) {
    
    logger::debug("Im here");

    if (widget_root.empty()) {
        logger::debug("Widget root communicated was empty");
    }
    
    
    current_widget_root = widget_root;
    logger::debug("And then here");
}

RE::ActorValue GetBloodPoolAV() {
    auto avg = GetModuleHandle(AVG_API_SOURCE);
    if (avg == nullptr) {
        avg = GetModuleHandle(L"ActorValueExtension.dll");
        if (avg == nullptr) {
            logger::debug("There is no AVG dll present! Move this check to registring");
            return RE::ActorValue::kVariable08;
        }
    }

    RE::ActorValue av = AVG::ExtraValue(bloodPoolAVname);
    if (av != RE::ActorValue::kNone) {
        logger::debug("Found the AVG ED_BloodPool");
        return av;
    }
    logger::debug("DID NOT FIND the AVG ED_BloodPool");
    return RE::ActorValue::kVariable08;
}


void SetBloodMeterPercent() {

    if (current_widget_root.empty()) {
        logger::debug("Current meter root not defined, doing nothing");
        return;
    }
	auto ui = RE::UI::GetSingleton();
    if (!ui || !ui->IsMenuOpen(RE::HUDMenu::MENU_NAME)) return;

    auto hud = ui->GetMenu<RE::HUDMenu>();
    if (!hud || !hud->uiMovie) return;

    RE::GPtr<RE::GFxMovieView> view = hud->uiMovie;

    auto player = RE::PlayerCharacter::GetSingleton();
    RE::ActorValue bloodPoolAV = GetBloodPoolAV();

    auto currentBloodPool = player->AsActorValueOwner()->GetActorValue(bloodPoolAV);

    RE::TESGlobal* totalBloodPoolGlobal = RE::TESForm::LookupByEditorID<RE::TESGlobal>("ED_Mechanics_BloodPool_Total");
    auto totalBloodPool = totalBloodPoolGlobal->value;

    RE::GFxValue args[2];
    args[0].SetNumber(currentBloodPool/totalBloodPool);
    args[1].SetNumber(0.0f);
    
    view->Invoke(current_widget_root.c_str(), nullptr, args, 1);

}

void DummyUpdateFunc() { logger::debug("UPDATE ACTION HERE"); }


void BloodMeterUpdateLoop() {

    while (bloodMeterIsUpdating) {
         logger::debug("UPDATE LOOP THREAD HERE");
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
         SKSE::GetTaskInterface()->AddUITask([]() { SetBloodMeterPercent(); });
    }

    //SetBloodMeterPercent();
}


void ToggleBloodPoolUpdateLoop(RE::StaticFunctionTag*, bool toggleOn) {

    if (toggleOn) {
        bloodMeterIsUpdating = true;
        logger::debug("Blood Meter Update toggled ON");

        std::thread(BloodMeterUpdateLoop).detach();  // run in background
        
        //SKSE::GetTaskInterface()->AddUITask([]() { BloodMeterUpdateLoop(); });
    } else {
        bloodMeterIsUpdating = false;
        /*if (bloodMeterUpdateThread.joinable()) {
            bloodMeterUpdateThread.join();
        }*/
        logger::debug("Blood Meter Update toggled OFF");
    }

}
