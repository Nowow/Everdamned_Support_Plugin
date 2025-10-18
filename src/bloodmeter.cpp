#include "bloodmeter.h"
#include "include/API_ActorValueGenerator.h"
#include <thread>

namespace logger = SKSE::log;

RE::BSString current_widget_root = "_root.WidgetContainer.7.widget";
RE::BSString root_setpercent = "_root.WidgetContainer.7.widget.setPercent";
RE::BSString root_fadeto = "_root.WidgetContainer.7.widget.fadeTo";
RE::BSString root_setalpha = "_root.WidgetContainer.7.widget.setAlpha";

constexpr std::string_view bloodPoolAVname = "ED_BloodPool";
RE::ActorValue bloodPoolAV;
RE::TESGlobal* totalBloodPoolGlobal;
float lastRatio;

    //std::atomic<bool> bloodMeterIsUpdating{false};

bool bloodMeterIsUpdating = false;
std::thread bloodMeterUpdateThread;


void sendModEvent(RE::TESForm* sender, std::string eventName, std::string strArg, float numArg) {
    SKSE::ModCallbackEvent modEvent{eventName, strArg, numArg, sender};

    auto modCallbackSource = SKSE::GetModCallbackEventSource();
    modCallbackSource->SendEvent(&modEvent);
}


void CommunicateCurrentWidgetRoot(RE::StaticFunctionTag*, RE::BSString widget_root) {
    
    if (widget_root.empty()) {
        logger::debug("Widget root communicated was empty");
    }
     
    current_widget_root = widget_root;

    std::string temp = widget_root.c_str();
    temp += ".setPercent";
    logger::debug("Widget .setPercent root: {}", temp);
    root_setpercent = temp.c_str();

    temp = widget_root.c_str();
    temp += ".fadeTo";
    logger::debug("Widget .fadeTo root: {}", temp);
    root_fadeto = temp.c_str();

    temp = widget_root.c_str();
    temp += ".setAlpha";
    logger::debug("Widget .setAlpha root: {}", temp);
    root_setalpha = temp.c_str();
    
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


void SetBloodMeterPercent(bool bForce) {

    if (root_setpercent.empty()) {
        logger::debug("Current meter root not defined, doing nothing");
        return;
    }

    auto player = RE::PlayerCharacter::GetSingleton();
    auto currentBloodPool = player->AsActorValueOwner()->GetActorValue(bloodPoolAV);
    auto totalBloodPool = totalBloodPoolGlobal->value;

    float currentRatio = currentBloodPool / totalBloodPool;
    if (!bForce && lastRatio == currentRatio) return;

    lastRatio = currentRatio;

	auto ui = RE::UI::GetSingleton();
    if (!ui || !ui->IsMenuOpen(RE::HUDMenu::MENU_NAME)) return;

    auto hud = ui->GetMenu<RE::HUDMenu>();
    if (!hud || !hud->uiMovie) return;

    RE::GPtr<RE::GFxMovieView> view = hud->uiMovie;

    RE::GFxValue args[2];
    args[0].SetNumber(currentRatio);  //percent to display
    args[1].SetNumber(0.0f);  //bool if fade or instant
    
    view->Invoke(root_setpercent.c_str(), nullptr, args, 1);

    RE::GFxValue args2[2];
    args2[0].SetNumber(100.f);  // full alpha
    args2[1].SetNumber(0.1f);  // duration over which to fade

    view->Invoke(root_fadeto.c_str(), nullptr, args2, 2);

}


void BloodMeterUpdateLoop() {

    while (bloodMeterIsUpdating) {
         //logger::debug("UPDATE LOOP THREAD HERE");
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
         SKSE::GetTaskInterface()->AddUITask([]() { SetBloodMeterPercent(false); });
    }

}


void ToggleBloodPoolUpdateLoop(RE::StaticFunctionTag*, bool toggleOn) {

    if (toggleOn) {
        if (bloodMeterIsUpdating) return;

        if (current_widget_root.empty()) {
            logger::debug("Blood Meter Updater: Current meter root not defined, exiting");
            return;
        }

        bloodPoolAV = GetBloodPoolAV();
        totalBloodPoolGlobal = RE::TESForm::LookupByEditorID<RE::TESGlobal>("ED_Mechanics_BloodPool_Total");

        if (totalBloodPoolGlobal == nullptr) {
            logger::debug("Blood Meter Updater: could not find max pool global, exiting");
            return;
        }

        SetBloodMeterPercent(true);
        bloodMeterIsUpdating = true;
        logger::debug("Blood Meter Update toggled ON");

        std::thread(BloodMeterUpdateLoop).detach();  // run in background
        
        //SKSE::GetTaskInterface()->AddUITask([]() { BloodMeterUpdateLoop(); });
    } else {
        if (!bloodMeterIsUpdating) return;
        bloodMeterIsUpdating = false;
        /*if (bloodMeterUpdateThread.joinable()) {
            bloodMeterUpdateThread.join();
        }*/
        logger::debug("Blood Meter Update toggled OFF");
    }

}
