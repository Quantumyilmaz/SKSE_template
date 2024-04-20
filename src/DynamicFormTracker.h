#include "Utils.h"

class DynamicFormTracker : public Utilities::DFSaveLoadData {
    std::map<std::pair<FormID, std::string>, std::set<FormID>> forms;  // Create populates this
    std::map<FormID, uint32_t> customIDforms;                          // Fetch populates this

    std::set<FormID> active_forms;  // _yield populates this
    std::set<FormID> deleted_forms;

    std::mutex mutex;
    const unsigned int form_limit = 10000;
    bool block_create = false;

    std::map<FormID, float> act_effs;

    [[nodiscard]] const bool IsTracked(const FormID dynamic_formid) {
        for (const auto& [base_pair, dyn_formset] : forms) {
            if (dyn_formset.contains(dynamic_formid)) {
                return true;
            }
        }
        return false;
    }

    [[maybe_unused]] RE::TESForm* GetOGFormOfDynamic(const FormID dynamic_formid) {
        for (const auto& [base_pair, dyn_formset] : forms) {
            if (dyn_formset.contains(dynamic_formid)) {
                return Utilities::FunctionsSkyrim::GetFormByID(base_pair.first, base_pair.second);
            }
        }
        return nullptr;
    }

    void ReviveDynamicForm(RE::TESForm* fake, RE::TESForm* base, const FormID setFormID) {
        using namespace Utilities::FunctionsSkyrim::DynamicForm;
        fake->Copy(base);
        auto weaponBaseForm = base->As<RE::TESObjectWEAP>();

        auto weaponNewForm = fake->As<RE::TESObjectWEAP>();

        auto bookBaseForm = base->As<RE::TESObjectBOOK>();

        auto bookNewForm = fake->As<RE::TESObjectBOOK>();

        auto ammoBaseForm = base->As<RE::TESAmmo>();

        auto ammoNewForm = fake->As<RE::TESAmmo>();

        if (weaponNewForm && weaponBaseForm) {
            weaponNewForm->firstPersonModelObject = weaponBaseForm->firstPersonModelObject;

            weaponNewForm->weaponData = weaponBaseForm->weaponData;

            weaponNewForm->criticalData = weaponBaseForm->criticalData;

            weaponNewForm->attackSound = weaponBaseForm->attackSound;

            weaponNewForm->attackSound2D = weaponBaseForm->attackSound2D;

            weaponNewForm->attackSound = weaponBaseForm->attackSound;

            weaponNewForm->attackFailSound = weaponBaseForm->attackFailSound;

            weaponNewForm->idleSound = weaponBaseForm->idleSound;

            weaponNewForm->equipSound = weaponBaseForm->equipSound;

            weaponNewForm->unequipSound = weaponBaseForm->unequipSound;

            weaponNewForm->soundLevel = weaponBaseForm->soundLevel;

            weaponNewForm->impactDataSet = weaponBaseForm->impactDataSet;

            weaponNewForm->templateWeapon = weaponBaseForm->templateWeapon;

            weaponNewForm->embeddedNode = weaponBaseForm->embeddedNode;

        } else if (bookBaseForm && bookNewForm) {
            bookNewForm->data.flags = bookBaseForm->data.flags;

            bookNewForm->data.teaches.spell = bookBaseForm->data.teaches.spell;

            bookNewForm->data.teaches.actorValueToAdvance = bookBaseForm->data.teaches.actorValueToAdvance;

            bookNewForm->data.type = bookBaseForm->data.type;

            bookNewForm->inventoryModel = bookBaseForm->inventoryModel;

            bookNewForm->itemCardDescription = bookBaseForm->itemCardDescription;

        } else if (ammoBaseForm && ammoNewForm) {
            ammoNewForm->GetRuntimeData().data.damage = ammoBaseForm->GetRuntimeData().data.damage;

            ammoNewForm->GetRuntimeData().data.flags = ammoBaseForm->GetRuntimeData().data.flags;

            ammoNewForm->GetRuntimeData().data.projectile = ammoBaseForm->GetRuntimeData().data.projectile;
        }
        /*else {
            new_form->Copy(baseForm);
        }*/

        copyComponent<RE::TESDescription>(base, fake);

        copyComponent<RE::BGSKeywordForm>(base, fake);

        copyComponent<RE::BGSPickupPutdownSounds>(base, fake);

        copyComponent<RE::TESModelTextureSwap>(base, fake);

        copyComponent<RE::TESModel>(base, fake);

        copyComponent<RE::BGSMessageIcon>(base, fake);

        copyComponent<RE::TESIcon>(base, fake);

        copyComponent<RE::TESFullName>(base, fake);

        copyComponent<RE::TESValueForm>(base, fake);

        copyComponent<RE::TESWeightForm>(base, fake);

        copyComponent<RE::BGSDestructibleObjectForm>(base, fake);

        copyComponent<RE::TESEnchantableForm>(base, fake);

        copyComponent<RE::BGSBlockBashData>(base, fake);

        copyComponent<RE::BGSEquipType>(base, fake);

        copyComponent<RE::TESAttackDamageForm>(base, fake);

        copyComponent<RE::TESBipedModelForm>(base, fake);

        if (setFormID != 0) fake->SetFormID(setFormID, false);
    }

    template <typename T>
    const FormID Create(T* baseForm, const RE::FormID setFormID = 0) {
        if (block_create) return 0;

        // std::lock_guard<std::mutex> lock(mutex);

        if (!baseForm) {
            logger::error("Real form is null for baseForm.");
            return 0;
        }

        const auto base_formid = baseForm->GetFormID();
        const auto base_editorid = clib_util::editorID::get_editorID(baseForm);

        if (base_editorid.empty()) {
            logger::error("Failed to get editorID for baseForm.");
            return 0;
        }

        RE::TESForm* new_form = nullptr;

        auto factory = RE::IFormFactory::GetFormFactoryByType(baseForm->GetFormType());

        new_form = factory->Create();

        // new_form = baseForm->CreateDuplicateForm(true, (void*)new_form)->As<T>();

        if (!new_form) {
            logger::error("Failed to create new form.");
            return 0;
        }
        logger::trace("Original form id: {:x}", new_form->GetFormID());

        if (forms[{base_formid, base_editorid}].contains(setFormID)) {
            logger::warn("Form with ID {:x} already exist for baseid {} and editorid {}.", setFormID, base_formid,
                         base_editorid);
            ReviveDynamicForm(new_form, baseForm, 0);
        } else
            ReviveDynamicForm(new_form, baseForm, setFormID);

        const auto new_formid = new_form->GetFormID();

        logger::trace("Created form with type: {}, Base ID: {:x}, Name: {}",
                      RE::FormTypeToString(new_form->GetFormType()), new_form->GetFormID(), new_form->GetName());

        if (!forms[{base_formid, base_editorid}].insert(new_formid).second) {
            logger::error("Failed to insert new form into forms.");
            _delete({base_formid, base_editorid}, new_formid);
            return 0;
        };

        if (new_formid >= 0xFF3DFFFF) {
            logger::critical("Dynamic FormID limit reached!!!!!!");
            block_create = true;
            _delete({base_formid, base_editorid}, new_formid);
            return 0;
        }

        return new_formid;
    }

    const FormID GetByCustomID(const uint32_t custom_id, const FormID base_formid, std::string base_editorid) {
        const auto formset = GetFormSet(base_formid, base_editorid);
        if (formset.empty()) return 0;
        for (const auto _formid : formset) {
            if (customIDforms.contains(_formid) && customIDforms[_formid] == custom_id) return _formid;
        }
        return 0;
    }

    const bool IsActive(const FormID a_formid) { return active_forms.contains(a_formid); }

    const RE::TESForm* _yield(const FormID dynamic_formid, RE::TESForm* base_form) {
        if (auto newForm = RE::TESForm::LookupByID(dynamic_formid)) {
            if (std::strlen(newForm->GetName()) == 0) {
                ReviveDynamicForm(newForm, base_form, 0);
            }
            if (active_forms.insert(dynamic_formid).second) {
                if (active_forms.size() > form_limit) {
                    logger::warn("Active dynamic forms limit reached!!!");
                    block_create = true;
                }
            };

            return newForm;
        }
        return nullptr;
    }

    void _delete(const std::pair<FormID, std::string> base, const FormID dynamic_formid) {
        if (!forms.contains(base)) return;

        if (auto newForm = RE::TESForm::LookupByID(dynamic_formid)) {
            if (auto bound_temp = newForm->As<RE::TESBoundObject>(); bound_temp) {
                auto player = RE::PlayerCharacter::GetSingleton();
                auto player_inventory = player->GetInventory();
                if (auto it = player_inventory.find(bound_temp); it != player_inventory.end()) {
                    player->RemoveItem(bound_temp, it->second.first, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
                }
            }

            // if (auto* virtualMachine = RE::BSScript::Internal::VirtualMachine::GetSingleton()) {
            //     auto* handlePolicy = virtualMachine->GetObjectHandlePolicy();
            //     auto* bindPolicy = virtualMachine->GetObjectBindPolicy();

            //    if (handlePolicy && bindPolicy) {
            //        auto newHandler = handlePolicy->GetHandleForObject(newForm->GetFormType(), newForm);

            //        if (newHandler != handlePolicy->EmptyHandle()) {
            //            auto* vm_scripts_hashmap = &virtualMachine->attachedScripts;
            //            auto newHandlerScripts_it = vm_scripts_hashmap->find(newHandler);

            //            if (newHandlerScripts_it != vm_scripts_hashmap->end()) {
            //                vm_scripts_hashmap[newHandler].clear();
            //            }
            //        }
            //    }
            //}
            delete newForm;
        }

        forms[base].erase(dynamic_formid);
        customIDforms.erase(dynamic_formid);
        active_forms.erase(dynamic_formid);
        deleted_forms.insert(dynamic_formid);
    }

	[[nodiscard]] const bool _underlying_check(const RE::TESForm* underlying, const RE::TESForm* derivative) const {
		
        if (underlying->GetFormType() != derivative->GetFormType()) {
            logger::trace("Form types do not match.");
            return false;
        }

        // alchemy
        const auto alch_underlying = underlying->As<RE::AlchemyItem>();
        const auto alch_derivative = derivative->As<RE::AlchemyItem>();
        bool asd1 = alch_underlying!=nullptr;
        bool asd2 = alch_derivative != nullptr;
        if (const int asd = asd1 + asd2; asd % 2 != 0){
            logger::trace("Alchemy status does not match.");
			return false;
        }

        if (alch_underlying && alch_derivative) {
			if (alch_underlying->IsPoison() != alch_derivative->IsPoison()) {
				logger::trace("Poison status does not match.");
				return false;
			}

            if (alch_underlying->IsFood() != alch_derivative->IsFood()) {
				logger::trace("Food status does not match.");
				return false;
			}

            if (alch_underlying->IsMedicine() != alch_derivative->IsMedicine()) {
                logger::trace("Medicine status does not match.");
                return false;
            }
		}

        // ingredient
        const auto ingr_underlying = underlying->As<RE::IngredientItem>();
        const auto ingr_derivative = derivative->As<RE::IngredientItem>();
        asd1 = ingr_underlying != nullptr;
        asd2 = ingr_derivative != nullptr;
        if (const int asd = asd1 + asd2; asd % 2 != 0){
			logger::trace("Ingredient status does not match.");
            return false;
		}

        if (ingr_underlying && ingr_derivative) {
            if (ingr_underlying->IsFood() != ingr_derivative->IsFood()) {
                logger::trace("Food status does not match.");
                return false;
            }

            if (ingr_underlying->IsPoison() != ingr_derivative->IsPoison()) {
                logger::trace("Poison status does not match.");
                return false;
            }

            if (ingr_underlying->IsMedicine() != ingr_derivative->IsMedicine()) {
                logger::trace("Medicine status does not match.");
                return false;
            }
        }

        // TODO ergänzen as you enable other modules


		return true;
	}

public:
    static DynamicFormTracker* GetSingleton() {
        static DynamicFormTracker singleton;
        return &singleton;
    }

    const char* GetType() override { return "DynamicFormTracker"; }

    void Delete(const FormID dynamic_formid) {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& [base, formset] : forms) {
            if (formset.contains(dynamic_formid)) {
                _delete(base, dynamic_formid);
            }
        }
    }

    void DeleteInactives() {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& [base, formset] : forms) {
            size_t index = 0;
            while (index < formset.size()) {
                auto it = formset.begin();
                std::advance(it, index);
                if (!IsActive(*it))
                    _delete(base, *it);
                else
                    index++;
            }
        }
    }

    std::vector<std::pair<FormID, std::string>> GetSourceForms() {
        std::vector<std::pair<FormID, std::string>> source_forms;
        for (const auto& [base, formset] : forms) {
            source_forms.push_back(base);
        }
        return source_forms;
    }

    void EditCustomID(const FormID dynamic_formid, const uint32_t custom_id) {
        if (customIDforms.contains(dynamic_formid))
            customIDforms[dynamic_formid] = custom_id;
        else if (IsTracked(dynamic_formid))
            customIDforms.insert({dynamic_formid, custom_id});
    }

    const FormID Fetch(const FormID baseFormID, const std::string baseEditorID,
                       const std::optional<uint32_t> customID) {
        auto* base_form = Utilities::FunctionsSkyrim::GetFormByID(baseFormID, baseEditorID);

        if (!base_form) {
            logger::error("Failed to get base form.");
            return 0;
        }

        if (customID.has_value()) {
            const auto new_formid = GetByCustomID(customID.value(), baseFormID, baseEditorID);
            if (const auto dyn_form = _yield(new_formid, base_form)) return dyn_form->GetFormID();
        } else if (const auto formset = GetFormSet(baseFormID, baseEditorID); !formset.empty()) {
            for (const auto _formid : formset) {
                if (IsActive(_formid)) continue;
                if (const auto dyn_form = _yield(_formid, base_form)) return dyn_form->GetFormID();
            }
        }

        return 0;
    }

    template <typename T>
    const FormID FetchCreate(const FormID baseFormID, const std::string baseEditorID,
                             const std::optional<uint32_t> customID) {
        // TODO merge with Fetch
        auto* base_form = Utilities::FunctionsSkyrim::GetFormByID<T>(baseFormID, baseEditorID);

        if (!base_form) {
            logger::error("Failed to get base form.");
            return 0;
        }

        if (customID.has_value()) {
            const auto new_formid = GetByCustomID(customID.value(), baseFormID, baseEditorID);
            if (const auto dyn_form = _yield(new_formid, base_form)) return dyn_form->GetFormID();
        } else if (const auto formset = GetFormSet(baseFormID, baseEditorID); !formset.empty()) {
            for (const auto _formid : formset) {
                if (IsActive(_formid)) continue;
                if (const auto dyn_form = _yield(_formid, base_form)) return dyn_form->GetFormID();
                // else if (!Utilities::FunctionsSkyrim::GetFormByID(_formid)) Delete({baseFormID, baseEditorID},
                // _formid);
            }
        }

        if (const auto dyn_form = _yield(Create<T>(base_form), base_form)) {
            const auto new_formid = dyn_form->GetFormID();
            if (customID.has_value()) customIDforms[new_formid] = customID.value();
            return new_formid;
        }

        return 0;
    }

    void ReviveAll() {
        for (const auto& [base, formset] : forms) {
			auto* base_form = Utilities::FunctionsSkyrim::GetFormByID(base.first, base.second);
			if (!base_form) {
				logger::error("Failed to get base form.");
				continue;
			}
			for (const auto _formid : formset) {
				if (const auto dyn_form = _yield(_formid, base_form)) {
					logger::info("Revived form with ID: {:x}", dyn_form->GetFormID());
				}
			}
		}

    }

    void ApplyMissingActiveEffects() {
        if (act_effs.empty()) return;

        auto plyr = RE::PlayerCharacter::GetSingleton();
        auto mg_target = plyr->AsMagicTarget();
        if (!mg_target) {
            logger::error("Failed to get player as magic target.");
            return;
        }
        auto act_eff_list = mg_target->GetActiveEffectList();
        for (auto it = act_eff_list->begin(); it != act_eff_list->end(); ++it) {
            if (const auto* act_eff = *it) {
                if (const auto mg_item = act_eff->spell) {
                    const auto mg_item_formid = mg_item->GetFormID();
                    if (act_effs.contains(mg_item_formid)) act_effs.erase(mg_item_formid);
                }
            }
        }

        auto mg_caster = plyr->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
        if (!mg_caster) {
            logger::error("Failed to get player as magic caster.");
            return;
        }
        for (const auto& [item_formid, elapsed] : act_effs) {
            auto* item = RE::TESForm::LookupByID<RE::MagicItem>(item_formid);
            if (!item) {
                logger::error("Failed to get item by formid.");
                continue;
            }
            //        auto* OGform = GetOGFormOfDynamic(item_formid);
            //        if (!OGform){
            //            logger::error("Failed to get original form of dynamic form.");
            // continue;
            //        }
            //        ReviveDynamicForm(item, OGform, 0);
            mg_caster->CastSpellImmediate(item, false, plyr, 1.0f, false, 0.0f, nullptr);
        }

        // now i need to go to act eff list and adjust the elapsed time
        act_eff_list = mg_target->GetActiveEffectList();
        for (auto it = act_eff_list->begin(); it != act_eff_list->end(); ++it) {
            if (auto* act_eff = *it) {
                if (const auto mg_item = act_eff->spell) {
                    const auto mg_item_formid = mg_item->GetFormID();
                    if (act_effs.contains(mg_item_formid)) {
                        if (act_eff->duration > act_effs[mg_item_formid]) {
                            act_eff->elapsedSeconds = act_effs[mg_item_formid];
                        } else {
                            act_eff->elapsedSeconds = act_eff->duration - 1;
                        }
                        act_effs.erase(mg_item_formid);
                    }
                }
            }
        }

        act_effs.clear();
    };

    const std::set<FormID> GetFormSet(const FormID base_formid, std::string base_editorid = "") {
        if (base_editorid.empty()) {
            base_editorid = Utilities::FunctionsSkyrim::GetEditorID(base_formid);
            if (base_editorid.empty()) {
                return {};
            }
        }
        const std::pair<FormID, std::string> key = {base_formid, base_editorid};
        if (forms.contains(key)) return forms[key];
        return {};
    };

    const size_t GetNDeleted() { return deleted_forms.size(); }

    void SendData() {
        // std::lock_guard<std::mutex> lock(mutex);
        logger::info("--------Sending data (DFT) ---------");
        Clear();

        act_effs.clear();
        auto act_eff_list = RE::PlayerCharacter::GetSingleton()->AsMagicTarget()->GetActiveEffectList();

        for (auto it = act_eff_list->begin(); it != act_eff_list->end(); ++it) {
            if (const auto* act_eff = *it) {
                const auto act_eff_formid = act_eff->spell->GetFormID();
                if (active_forms.contains(act_eff_formid)) {
                    act_effs[act_eff_formid] = act_eff->elapsedSeconds;
                }
            }
        }

        int n_fakes = 0;
        for (const auto& [base_pair, dyn_formset] : forms) {
            Utilities::Types::DFSaveDataLHS lhs({base_pair.first, base_pair.second});
            Utilities::Types::DFSaveDataRHS rhs;
            for (const auto dyn_formid : dyn_formset) {
                if (!IsActive(dyn_formid)) logger::critical("Inactive form found in forms set.");
                const bool has_customid = customIDforms.contains(dyn_formid);
                const uint32_t customid = has_customid ? customIDforms[dyn_formid] : 0;
                const bool is_act_eff_spell = act_effs.contains(dyn_formid);
                const float act_eff_elpsd = is_act_eff_spell ? act_effs[dyn_formid] : -1.f;
                Utilities::Types::DFSaveData saveData({dyn_formid, {has_customid, customid}, act_eff_elpsd});
                rhs.push_back(saveData);
                n_fakes++;
            }
            if (!rhs.empty()) SetData(lhs, rhs);
        }

        logger::info("Number of dynamic forms sent: {}", n_fakes);
        logger::info("--------Data sent (DFT) ---------");
    };

    void ReceiveData() {
        // std::lock_guard<std::mutex> lock(mutex);
        logger::info("--------Receiving data (DFT) ---------");

        int n_fakes = 0;
        for (const auto& [lhs, rhs] : m_Data) {
            auto base_formid = lhs.first;
            const auto& base_editorid = lhs.second;
            const auto temp_form = Utilities::FunctionsSkyrim::GetFormByID(0, base_editorid);
            if (!temp_form) logger::critical("Failed to get base form.");
            else base_formid = temp_form->GetFormID();
            for (const auto& saveData : rhs) {
                const auto dyn_formid = saveData.dyn_formid;
                if (const auto dyn_form = RE::TESForm::LookupByID(dyn_formid); !dyn_form) {
                    logger::trace("Dynamic form {:x} does not exist.", dyn_formid);
                    continue;
                } 
                else if (const auto dyn_form_ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(dyn_formid)) {
                    logger::info("Dynamic form {:x} is a refr with name {}.", dyn_formid, dyn_form->GetName());
                    continue;
                }
                else if (!_underlying_check(temp_form, dyn_form)) {
                    // bcs load callback happens after the game loads, there is a chance that the game will assign new item to "previously" our dynamic formid
                    // especially for stuff like dynamic food which is not serialized by the game
                    logger::trace("Underlying check failed for dynamic form {:x} with name {}.", dyn_formid, dyn_form->GetName());
					continue;
                }
                const auto [has_customid, customid] = saveData.custom_id;
                const auto act_eff_elpsd = saveData.acteff_elapsed;
                forms[{base_formid, base_editorid}].insert(dyn_formid);
                if (has_customid) customIDforms[dyn_formid] = customid;
                if (act_eff_elpsd >= 0.f) act_effs[dyn_formid] = act_eff_elpsd;
                n_fakes++;
            }
        }

        logger::info("Number of dynamic forms received: {}", n_fakes);
        // need to check if formids and editorids are valid
        logger::info("--------Data received (DFT) ---------");
    };

    void Reset() {
        // std::lock_guard<std::mutex> lock(mutex);
        forms.clear();
        customIDforms.clear();
        active_forms.clear();
        deleted_forms.clear();
        act_effs.clear();
        block_create = false;
    }
};

DynamicFormTracker* DFT = nullptr;
