

//#include "include\PerkEntryPointExtenderAPI.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <random>

namespace logger = SKSE::log;

void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    //RE::ConsoleLog::GetSingleton()->Print(logFilePath.string().c_str());
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::trace);
}


struct Hooks {
    struct CommandedActorLimitHook {
        static void thunk(RE::BGSPerkEntry::EntryPoint entry_point, RE::Actor* target, RE::MagicItem* a_spell, void* out) {
            //logger::info("We in CommandedActorLimitHook func body");
            float* floatPtr = static_cast<float*>(out);
            *floatPtr = 999.0f;  // If you need more than 999 summons, I think you've got a problem

        }
        static inline REL::Relocation<decltype(thunk)> func;
    };

    struct CommandedActorHook {
        static void thunk(RE::AIProcess* test, RE::ActiveEffectReferenceEffectController* target2, void* target3) {
            func(test, target2, target3);
            auto a_AE = target2->effect;

            logger::info("Commanded Actor effect was cast!");

            bool casterIsPlayer = test->GetUserData()->IsPlayerRef();
            logger::info("Caster was player: {}", casterIsPlayer ? "true" : "false");
            
            if (casterIsPlayer && a_AE->effect->baseEffect->HasKeywordString("MagicSummon_ED_Uncapped")) {
                logger::info("Summon spell was cast by player and it had effect with MagicSummon_ED_Uncapped keyword, so doing nothing further");
                return;
            }

            std::vector<int> a_effectsToDeleteIndex;
            std::vector<int> a_activeSummonEffectsIndex;
            std::vector<int> a_activeSummonEffectsIndexSorted;
            std::vector<float> a_activeSummonEffectsDuration;

            //logger::info("We in CommandedActorHook func body");

            float perkfactor = 0.0f;
            
            int j = 0;

            if (a_AE && test->middleHigh->perkData) {  

                auto akCastedMagic = a_AE->spell;
                auto commandedActorsEffectsArray = test->middleHigh->commandedActors;
                auto summoner = test->GetUserData();
                
                // getting current command limit with respect to the relevant entry point
                perkfactor = 1.0f;
                RE::BGSEntryPoint::HandleEntryPoint(RE::BGSPerkEntry::EntryPoint::kModCommandedActorLimit, summoner,
                                                    akCastedMagic, &perkfactor);

                logger::info("Current command limit: {}", perkfactor);

                // sorting active summon effects from newest to oldest
                for (auto& elements : commandedActorsEffectsArray) {
                    if (commandedActorsEffectsArray[j].activeEffect) {
                        a_activeSummonEffectsIndex.push_back(j);
                        a_activeSummonEffectsDuration.push_back(
                            commandedActorsEffectsArray[j].activeEffect->elapsedSeconds);
                    }
                    j += 1;
                }
                for (std::uint32_t widx = 0; widx < a_activeSummonEffectsIndex.size(); ++widx) {
                    auto maxtime =
                        std::max_element(a_activeSummonEffectsDuration.begin(), a_activeSummonEffectsDuration.end());
                    float maxvalue = *maxtime;
                    auto iter = (std::find(a_activeSummonEffectsDuration.begin(), a_activeSummonEffectsDuration.end(),
                                           maxvalue));
                    auto index = std::distance(a_activeSummonEffectsDuration.begin(), iter);
                    if (a_activeSummonEffectsIndexSorted.empty()) {
                        a_activeSummonEffectsIndexSorted.push_back(a_activeSummonEffectsIndex[index]);
                    } else {
                        a_activeSummonEffectsIndexSorted.insert(a_activeSummonEffectsIndexSorted.begin(),
                                                                a_activeSummonEffectsIndex[index]);
                    }
                    a_activeSummonEffectsDuration[index] = 0.0f;
                }

                // iterate over sorted active summon effects to determine which to dispel
                for (std::uint32_t widx = 0; widx < a_activeSummonEffectsIndexSorted.size(); ++widx) {
                    auto element = commandedActorsEffectsArray[a_activeSummonEffectsIndexSorted[widx]];

                    if (!casterIsPlayer  || !element.activeEffect->effect->baseEffect->HasKeywordString(
                            "MagicSummon_ED_Uncapped")) {
                        if (perkfactor >= 1.0f) {
                            // summon limit has space for this effect

                            logger::info("Checking capped command spell, not reached limit yet");

                            perkfactor -= 1.0f;
                        } else {
                            // summon limit is full, this spell and other

                            logger::info("Checking capped command spell, limit has already been reached, will be dispelled");
                            a_effectsToDeleteIndex.push_back(a_activeSummonEffectsIndexSorted[widx]);
                        }
                    }   
                }

                // dispel effects liable for dispel
                if (a_effectsToDeleteIndex.size() > 0) {
                    for (std::uint32_t widx = 0; widx < a_effectsToDeleteIndex.size(); ++widx) {
                        commandedActorsEffectsArray[a_effectsToDeleteIndex[widx]].activeEffect->Dispel(true);
                    }
                }
            }
        }
        static inline REL::Relocation<decltype(thunk)> func;
    };
    static void Install() {
        REL::Relocation<std::uintptr_t> functionCommandedActorLimitHook{RELOCATION_ID(38993, 40056),
                                                                        REL::Relocate(0xA1, 0xEC)};
        stl::write_thunk_call<CommandedActorLimitHook>(functionCommandedActorLimitHook.address());

        REL::Relocation<std::uintptr_t> functionCommandedActorHook{RELOCATION_ID(38904, 39950),
                                                                   REL::Relocate(0x14B, 0x12B)};
        stl::write_thunk_call<CommandedActorHook>(functionCommandedActorHook.address());
    };
};


void MessageListener(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kPostLoad) {

        logger::info("All plugins have loaded, checking if SummonActorLimitOverhaul is present");
        if (GetModuleHandle(L"SummonActorLimitOverhaul.dll") == nullptr) {
            logger::info("No SummonActorLimitOverhaul detected, installing hooks...");
            Hooks::Install();
            logger::info("EverdamnedSupportPlugin hooks were installed!");
        } else {
            logger::info("SummonActorLimitOverhaul detected, plugin hooks were not installed");
        }

    }
}

//std::string MyNativeFunction(RE::StaticFunctionTag*, int numba) {
//    logger::info("MyNativeFunction in C++ got called!, numba was: {}", numba);
//    return "Hello from C++!";
//}
//
//std::string GetSpellAndReturnItsName(RE::StaticFunctionTag*, RE::SpellItem* theSpell) {
//    logger::info("The spell name: {}", theSpell->GetFullName());
//    return theSpell->GetFullName();
//}
//
//RE::Actor* GetEffectAndReturnActor(RE::StaticFunctionTag*, RE::ActiveEffect* theEffect) {
//    logger::info("The effect name: {}", theEffect->effect->baseEffect->GetFullName());
//    logger::info("The effect caster name: {}", theEffect->caster.get().get()->GetName());
//    return theEffect->caster.get().get();
//}

RE::Actor* GetActiveEffectCommandedActor(RE::StaticFunctionTag*, RE::ActiveEffect* theEffect) {
    //logger::info("The effect name: {}", theEffect->effect->baseEffect->GetFullName());
    //logger::info("The effect caster name: {}", theEffect->caster.get().get()->GetName());

    if (theEffect) {
        logger::debug("GetActiveEffectCommandedActor got called and effect is not null!");
        RE::SummonCreatureEffect* summonedeffect;
        RE::ReanimateEffect* reanimatedeffect;
        RE::CommandEffect* commandedeffect;
        RE::Actor* summonedactor;

        if (theEffect->effect->baseEffect->HasArchetype(RE::EffectArchetypes::ArchetypeID::kSummonCreature)) {
            logger::debug("Control effect is of Summon type");
            summonedeffect = reinterpret_cast<RE::SummonCreatureEffect*>(theEffect);
            if (summonedeffect) {
                summonedactor = summonedeffect->commandedActor.get().get();
                return summonedactor;
            }
        } else if (theEffect->effect->baseEffect->HasArchetype(RE::EffectArchetypes::ArchetypeID::kReanimate)) {
            logger::debug("Control effect is of reanimate type");
            reanimatedeffect = reinterpret_cast<RE::ReanimateEffect*>(theEffect);
            if (reanimatedeffect) {
                summonedactor = reanimatedeffect->commandedActor.get().get();
                return summonedactor;
            }
        } else if (theEffect->effect->baseEffect->HasArchetype(RE::EffectArchetypes::ArchetypeID::kCommandSummoned)) {
            logger::debug("Control effect is of command type");
            commandedeffect = reinterpret_cast<RE::CommandEffect*>(theEffect);
            if (commandedeffect) {
                summonedactor = commandedeffect->commandedActor.get().get();
                return summonedactor;
            }
        }
    }
    return nullptr;
}

void IncreaseActiveEffectDuration(RE::StaticFunctionTag*, RE::ActiveEffect* theEffect, float delta) {
    if (theEffect) {
        logger::debug("IncreaseActiveEffectDuration got called!");
        logger::debug("Delta to increase duration by (only positive values): {}", abs(delta));
        logger::debug("This effect's duration before increase: {}", theEffect->duration);
        theEffect->duration += abs(delta);
        logger::debug("This effect's duration after increase: {}", theEffect->duration);
    }
}

RE::BSTArray<int> GetAdjustedAvForComparison(RE::StaticFunctionTag*, RE::Actor* thisActor, int playerLevel, int skillsPerLevelSetting, int skillBaseSetting) {


    RE::TESNPC* someNPC = thisActor->GetActorBase();
    RE::CLASS_DATA thisNpcClass = someNPC->npcClass->data;

    //thisActor->AsActorValueOwner()->GetBaseActorValue();

    RE::BSTArray<int> resultArr;

    bool lvlsWithPC = someNPC->HasPCLevelMult();

    float currentSkillLevel;
    int maxSkillLevel = 0;
    int maxSkillLevelNumber = 0;


    if (!lvlsWithPC) {
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kOneHanded);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 0;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kTwoHanded);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 1;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kArchery);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 2;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kBlock);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 3;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kSmithing);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 4;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHeavyArmor);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 5;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kLightArmor);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 6;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kPickpocket);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 7;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kLockpicking);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 8;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kSneak);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 9;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kAlchemy);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 10;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kSpeech);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 11;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kAlteration);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 12;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kConjuration);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 13;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kDestruction);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 14;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kIllusion);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 15;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kRestoration);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 16;
        }
        currentSkillLevel = thisActor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kEnchanting);
        if (currentSkillLevel > maxSkillLevel) {
            maxSkillLevel = currentSkillLevel;
            maxSkillLevelNumber = 17;
        }
        logger::debug("This actor does not lvl with PC, returning current max skill level of {}", maxSkillLevel);

        resultArr.push_back(maxSkillLevel);
        resultArr.push_back(maxSkillLevelNumber);
        return resultArr;
    }

    int currentWeight;
    int maxWeight = 0;
    int maxWeightSkillNumber = 0;
    int totalWeights = 0;

    currentWeight = thisNpcClass.skillWeights.oneHanded;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 0;
    }
    currentWeight = thisNpcClass.skillWeights.twoHanded;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 1;
    }
    currentWeight = thisNpcClass.skillWeights.archery;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 2;
    }
    currentWeight = thisNpcClass.skillWeights.block;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 3;
    }
    currentWeight = thisNpcClass.skillWeights.smithing;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 4;
    }
    currentWeight = thisNpcClass.skillWeights.heavyArmor;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 5;
    }
    currentWeight = thisNpcClass.skillWeights.lightArmor;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 6;
    }
    currentWeight = thisNpcClass.skillWeights.pickpocket;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 7;
    }
    currentWeight = thisNpcClass.skillWeights.lockpicking;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 8;
    }
    currentWeight = thisNpcClass.skillWeights.sneak;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 9;
    }
    currentWeight = thisNpcClass.skillWeights.alchemy;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 10;
    }
    currentWeight = thisNpcClass.skillWeights.speech;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 11;
    }
    currentWeight = thisNpcClass.skillWeights.alteration;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 12;
    }
    currentWeight = thisNpcClass.skillWeights.conjuration;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 13;
    }
    currentWeight = thisNpcClass.skillWeights.destruction;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 14;
    }
    currentWeight = thisNpcClass.skillWeights.illusion;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 15;
    }
    currentWeight = thisNpcClass.skillWeights.restoration;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 16;
    }
    currentWeight = thisNpcClass.skillWeights.enchanting;
    totalWeights += currentWeight;
    if (currentWeight > maxWeight) {
        maxWeight = currentWeight;
        maxWeightSkillNumber = 17;
    }

    logger::debug("maxWeight: {}, maxWeightSkillNumber: {}, totalWeights: {}", maxWeight, maxWeightSkillNumber, totalWeights);

    RE::ACTOR_BASE_DATA thisNpcBaseData = someNPC->actorData;

    int adjustLevel;
    int maxLvl = thisNpcBaseData.calcLevelMax;
    bool isUnique = someNPC->IsUnique();

    if (isUnique) {
        adjustLevel = maxLvl;
        logger::debug(
            "This actor does lvl with PC and is unique, skill will be adjusted to its max lvl of: {}", adjustLevel);
    } else {
        // move this shet out of func
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(playerLevel, maxLvl);

        int adjustLevel = distrib(gen);
        logger::debug(
            "This actor does lvl with PC and is NOt unique,  skill will be adjusted to random between current "
            "player lvl and its max lvl, res: {}", adjustLevel);
    }

    int totalSkillPoints = skillsPerLevelSetting * adjustLevel;
    logger::debug("totalSkillPoints: {}", totalSkillPoints);

    float weightShare = static_cast<float>(maxWeight) / totalWeights;
    logger::debug("weightShare: {}", weightShare);

    int calculatedSkillLevel = skillBaseSetting + (totalSkillPoints * weightShare);

    logger::debug("Adjusted skill level: {}", calculatedSkillLevel);

    resultArr.push_back(calculatedSkillLevel);
    resultArr.push_back(maxWeightSkillNumber);

    return resultArr;
    
}

bool BindPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    //vm->RegisterFunction("PapyrusNativeFunctionBinding", "ED_SKSEnativebindings", MyNativeFunction);
    //vm->RegisterFunction("GetProvidedSpellName", "ED_SKSEnativebindings", GetSpellAndReturnItsName);
    //vm->RegisterFunction("GetEffectCaster", "ED_SKSEnativebindings", GetEffectAndReturnActor);
    vm->RegisterFunction("GetActiveEffectCommandedActor", "ED_SKSEnativebindings", GetActiveEffectCommandedActor);
    vm->RegisterFunction("IncreaseActiveEffectDuration", "ED_SKSEnativebindings", IncreaseActiveEffectDuration);
    vm->RegisterFunction("GetAdjustedAvForComparison", "ED_SKSEnativebindings", GetAdjustedAvForComparison);
    logger::info("Papyrus functions bound!");
    return true;
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SetupLog();
    //skse->GetPluginInfo("");
    SKSE::GetMessagingInterface()->RegisterListener(MessageListener);
    SKSE::GetPapyrusInterface()->Register(BindPapyrusFunctions);
    

    //SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
    //    if (message->type == SKSE::MessagingInterface::kDataLoaded)
    //        
    //});
    return true;
}

