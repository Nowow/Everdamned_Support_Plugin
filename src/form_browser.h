#pragma once

RE::TESForm* LookupSomeFormByEditorID(RE::StaticFunctionTag*, std::string editorID);
void SetupFormMaps(RE::StaticFunctionTag*);

RE::BGSArtObject* GetArtObjectByIndex(RE::StaticFunctionTag*, std::string ModName, int i);
RE::BGSExplosion* GetExplosionByIndex(RE::StaticFunctionTag*, std::string ModName, int i);
RE::BGSProjectile* GetProjectileByIndex(RE::StaticFunctionTag*, std::string ModName, int i);
RE::TESObjectACTI* GetActivatorByIndex(RE::StaticFunctionTag*, std::string ModName, int i);
RE::BGSHazard* GetHazardByIndex(RE::StaticFunctionTag*, std::string ModName, int i);

