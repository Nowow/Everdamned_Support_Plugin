#pragma once

RE::TESForm* LookupSomeFormByEditorID(RE::StaticFunctionTag*, std::string editorID);
void SetupArtObjectArray(RE::StaticFunctionTag*);
RE::BGSArtObject* GetArtObjectByIndex(RE::StaticFunctionTag*, std::string ModName, int artIndex);
