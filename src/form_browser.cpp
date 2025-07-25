#include "form_browser.h"


namespace logger = SKSE::log;

//class BGSExplosion;
//class BGSProjectile;
//class TESObjectACTI;
//class BGSHazard;



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

RE::BSTArray<RE::BGSArtObject*, RE::BSTArrayHeapAllocator> art_objects;
RE::BSTArray<RE::BGSArtObject*, RE::BSTArrayHeapAllocator> art_objects_filtered;
std::map<std::string, RE::BSTArray<RE::BGSArtObject*, RE::BSTArrayHeapAllocator>> art_objects_by_mod;

void SetupArtObjectArray(RE::StaticFunctionTag*) {
    logger::debug("Setup Art object array was called");

    art_objects.clear();
    // art_objects_filtered.clear();

    art_objects = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSArtObject>();

    logger::debug("There are {} many art objects total", art_objects.size());

    int recordsSkipped = 0;

    for (auto* artItem : art_objects) {
        const auto someModName = std::string{artItem->GetFile(0)->GetFilename()};
        std::string TheModName = someModName;
        std::transform(TheModName.begin(), TheModName.end(), TheModName.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        /*auto found = (std::find(uniqueModNames.begin(), uniqueModNames.end(), someModName)) != uniqueModNames.end();

        if (!found) {
            logger::debug("Unique mod name: {}", someModName);
            uniqueModNames.push_back(someModName);
        }*/

        if (artItem->data.artType != RE::BGSArtObject::ArtType::kMagicHitEffect) {
            recordsSkipped++;
            continue;
        }

        /*if (TheModName != ModName) {
            recordsSkipped++;
            continue;
        }*/

        if (!(art_objects_by_mod.contains(TheModName))) {
            art_objects_by_mod[TheModName];
        }
        art_objects_by_mod[TheModName].push_back(artItem);

        // art_objects_filtered.push_back(artItem);
    }

    // logger::debug("Art objects valid: {}", recordsNotSkipped);
    // logger::debug("Art objects filtered array size: {}", art_objects_filtered.size());

    logger::debug("Art objects skipped: {}", recordsSkipped);

    for (const auto& pair : art_objects_by_mod) {
        logger::debug("Mod {} has this many valid art objects: {}", pair.first, pair.second.size());
    }
}

RE::BGSArtObject* GetArtObjectByIndex(RE::StaticFunctionTag*, std::string ModName, int artIndex) {
    if (artIndex < 0) {
        logger::debug("Art object index is < 0, returning nullptr!");
        return nullptr;
    }

    std::transform(ModName.begin(), ModName.end(), ModName.begin(), [](unsigned char c) { return std::tolower(c); });

    if (!(art_objects_by_mod.contains(ModName))) {
        logger::debug("Mod {} is not contained in the map!", ModName);
        return nullptr;
    }

    // RE::BGSArtObject* theArt = art_objects_filtered[artIndex];
    RE::BGSArtObject* theArt = art_objects_by_mod[ModName][artIndex];

    logger::debug("Retrieving art object number: {}, model path: {}", artIndex, theArt->model);
    return theArt;
}

