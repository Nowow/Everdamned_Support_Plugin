#include "form_browser.h"


namespace logger = SKSE::log;


RE::TESForm* LookupSomeFormByEditorID(RE::StaticFunctionTag*, std::string editorID) {
    // logger::debug("EditorID string that arrived: {}", editorID);
    auto someForm = RE::TESForm::LookupByEditorID(editorID);

    if (someForm == nullptr) {
        logger::debug("Looked up form is nullptr");
    } else {
        //    logger::debug("Looked up form is: {}", someForm->GetName());
    }

    return someForm;
}

RE::BSTArray<RE::BGSArtObject*, RE::BSTArrayHeapAllocator> all_art_objects;
RE::BSTArray<RE::BGSExplosion*, RE::BSTArrayHeapAllocator> all_exposions;
RE::BSTArray<RE::BGSProjectile*, RE::BSTArrayHeapAllocator> all_projectiles;
RE::BSTArray<RE::TESObjectACTI*, RE::BSTArrayHeapAllocator> all_activators;
RE::BSTArray<RE::BGSHazard*, RE::BSTArrayHeapAllocator> all_hazards;

std::map<std::string, RE::BSTArray<RE::BGSArtObject*, RE::BSTArrayHeapAllocator>> arts_by_mod;
std::map<std::string, RE::BSTArray<RE::BGSExplosion*, RE::BSTArrayHeapAllocator>> explosions_by_mod;
std::map<std::string, RE::BSTArray<RE::BGSProjectile*, RE::BSTArrayHeapAllocator>> projectiles_by_mod;
std::map<std::string, RE::BSTArray<RE::TESObjectACTI*, RE::BSTArrayHeapAllocator>> activators_by_mod;
std::map<std::string, RE::BSTArray<RE::BGSHazard*, RE::BSTArrayHeapAllocator>> hazards_by_mod;



void ProcessArtObjects(RE::BSTArray<RE::BGSArtObject*, RE::BSTArrayHeapAllocator> form_array) {
    int recordsSkipped = 0;

    for (auto* leform : form_array) {
        const auto someModName = std::string{leform->GetFile(0)->GetFilename()};
        std::string TheModName = someModName;
        std::transform(TheModName.begin(), TheModName.end(), TheModName.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (leform->data.artType != RE::BGSArtObject::ArtType::kMagicHitEffect) {
            recordsSkipped++;
            continue;
        }

        if (!(arts_by_mod.contains(TheModName))) {
            arts_by_mod[TheModName];
        }
        arts_by_mod[TheModName].push_back(leform);
    }

    logger::debug("Forms skipped: {}", recordsSkipped);

    for (const auto& pair : arts_by_mod) {
        logger::debug("Mod {} has this many valid ART objects: {}", pair.first, pair.second.size());
    }
}

void ProcessExplosions(RE::BSTArray<RE::BGSExplosion*, RE::BSTArrayHeapAllocator> form_array) {
    

    for (auto* leform : form_array) {
        const auto someModName = std::string{leform->GetFile(0)->GetFilename()};
        std::string TheModName = someModName;
        std::transform(TheModName.begin(), TheModName.end(), TheModName.begin(),
                       [](unsigned char c) { return std::tolower(c); });


        if (!(explosions_by_mod.contains(TheModName))) {
            explosions_by_mod[TheModName];
        }
        explosions_by_mod[TheModName].push_back(leform);
    }

    for (const auto& pair : explosions_by_mod) {
        logger::debug("Mod {} has this many valid ART objects: {}", pair.first, pair.second.size());
    }
}

void ProcessProjectiles(RE::BSTArray<RE::BGSProjectile*, RE::BSTArrayHeapAllocator> form_array) {
    for (auto* leform : form_array) {
        const auto someModName = std::string{leform->GetFile(0)->GetFilename()};
        std::string TheModName = someModName;
        std::transform(TheModName.begin(), TheModName.end(), TheModName.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (!(projectiles_by_mod.contains(TheModName))) {
            projectiles_by_mod[TheModName];
        }
        projectiles_by_mod[TheModName].push_back(leform);
    }

    for (const auto& pair : projectiles_by_mod) {
        logger::debug("Mod {} has this many valid ART objects: {}", pair.first, pair.second.size());
    }
}

void ProcessActivators(RE::BSTArray<RE::TESObjectACTI*, RE::BSTArrayHeapAllocator> form_array) {
    for (auto* leform : form_array) {
        const auto someModName = std::string{leform->GetFile(0)->GetFilename()};
        std::string TheModName = someModName;
        std::transform(TheModName.begin(), TheModName.end(), TheModName.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (!(activators_by_mod.contains(TheModName))) {
            activators_by_mod[TheModName];
        }
        activators_by_mod[TheModName].push_back(leform);
    }

    for (const auto& pair : activators_by_mod) {
        logger::debug("Mod {} has this many valid ART objects: {}", pair.first, pair.second.size());
    }
}

void ProcessHazards(RE::BSTArray<RE::BGSHazard*, RE::BSTArrayHeapAllocator> form_array) {
    for (auto* leform : form_array) {
        const auto someModName = std::string{leform->GetFile(0)->GetFilename()};
        std::string TheModName = someModName;
        std::transform(TheModName.begin(), TheModName.end(), TheModName.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (!(hazards_by_mod.contains(TheModName))) {
            hazards_by_mod[TheModName];
        }
        hazards_by_mod[TheModName].push_back(leform);
    }

    for (const auto& pair : hazards_by_mod) {
        logger::debug("Mod {} has this many valid ART objects: {}", pair.first, pair.second.size());
    }
}


void SetupFormMaps(RE::StaticFunctionTag*) {
    all_art_objects = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSArtObject>();
    all_exposions = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSExplosion>();
    all_projectiles = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSProjectile>();
    all_activators = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESObjectACTI>();
    all_hazards = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSHazard>();
    ProcessArtObjects(all_art_objects);
    ProcessExplosions(all_exposions);
    ProcessProjectiles(all_projectiles);
    ProcessActivators(all_activators);
    ProcessHazards(all_hazards);
}


RE::BGSArtObject* GetArtObjectByIndex(RE::StaticFunctionTag*, std::string ModName, int i) {
    if (i < 0) {
        logger::debug("Art object index is < 0, returning nullptr!");
        return nullptr;
    }

    std::transform(ModName.begin(), ModName.end(), ModName.begin(), [](unsigned char c) { return std::tolower(c); });

    if (!(arts_by_mod.contains(ModName))) {
        logger::debug("Mod {} is not contained in the map!", ModName);
        return nullptr;
    }

    if (i >= arts_by_mod[ModName].size()) {
        logger::debug("Form array size exseeded, returning nullptr");
        return nullptr;
    }

    RE::BGSArtObject* theArt = arts_by_mod[ModName][i];

    logger::debug("Retrieving art object number: {}, model path: {}", i, theArt->model);
    return theArt;
}

RE::BGSExplosion* GetExplosionByIndex(RE::StaticFunctionTag*, std::string ModName, int i) {
    if (i < 0) {
        logger::debug("Explosion index is < 0, returning nullptr!");
        return nullptr;
    }

    std::transform(ModName.begin(), ModName.end(), ModName.begin(), [](unsigned char c) { return std::tolower(c); });

    if (!explosions_by_mod.contains(ModName)) {
        logger::debug("Mod {} is not contained in the explosion map!", ModName);
        return nullptr;
    }

    if (i >= explosions_by_mod[ModName].size()) {
        logger::debug("Form array size exseeded, returning nullptr");
        return nullptr;
    }

    RE::BGSExplosion* theExplosion = explosions_by_mod[ModName][i];
    logger::debug("Retrieving explosion number: {}, editor ID: {}", i, theExplosion->GetFormEditorID());

    return theExplosion;
}

RE::BGSProjectile* GetProjectileByIndex(RE::StaticFunctionTag*, std::string ModName, int i) {
    if (i < 0) {
        logger::debug("Projectile index is < 0, returning nullptr!");
        return nullptr;
    }

    std::transform(ModName.begin(), ModName.end(), ModName.begin(), [](unsigned char c) { return std::tolower(c); });

    if (!projectiles_by_mod.contains(ModName)) {
        logger::debug("Mod {} is not contained in the projectile map!", ModName);
        return nullptr;
    }

    if (i >= projectiles_by_mod[ModName].size()) {
        logger::debug("Form array size exseeded, returning nullptr");
        return nullptr;
    }

    RE::BGSProjectile* theProjectile = projectiles_by_mod[ModName][i];
    logger::debug("Retrieving projectile number: {}, editor ID: {}", i, theProjectile->GetFormEditorID());

    return theProjectile;
}

RE::TESObjectACTI* GetActivatorByIndex(RE::StaticFunctionTag*, std::string ModName, int i) {
    if (i < 0) {
        logger::debug("Activator index is < 0, returning nullptr!");
        return nullptr;
    }

    std::transform(ModName.begin(), ModName.end(), ModName.begin(), [](unsigned char c) { return std::tolower(c); });

    if (!activators_by_mod.contains(ModName)) {
        logger::debug("Mod {} is not contained in the activator map!", ModName);
        return nullptr;
    }

    if (i >= activators_by_mod[ModName].size()) {
        logger::debug("Form array size exseeded, returning nullptr");
        return nullptr;
    }

    RE::TESObjectACTI* theActivator = activators_by_mod[ModName][i];
    logger::debug("Retrieving activator number: {}, name: {}", i, theActivator->GetName());

    return theActivator;
}

RE::BGSHazard* GetHazardByIndex(RE::StaticFunctionTag*, std::string ModName, int i) {
    if (i < 0) {
        logger::debug("Hazard index is < 0, returning nullptr!");
        return nullptr;
    }

    std::transform(ModName.begin(), ModName.end(), ModName.begin(), [](unsigned char c) { return std::tolower(c); });

    if (!hazards_by_mod.contains(ModName)) {
        logger::debug("Mod {} is not contained in the hazard map!", ModName);
        return nullptr;
    }

    if (i >= hazards_by_mod[ModName].size()) {
        logger::debug("Form array size exseeded, returning nullptr");
        return nullptr;
    }

    RE::BGSHazard* theHazard = hazards_by_mod[ModName][i];
    logger::debug("Retrieving hazard number: {}, editor ID: {}", i, theHazard->GetFormEditorID());

    return theHazard;
}