#pragma once

//#include <chrono>
#include "logger.h"

void SwapObjects(RE::TESObjectREFR* a_from, RE::TESBoundObject* a_to) {
    logger::trace("SwapObjects");
    if (!a_from) {
        logger::error("Ref is null.");
        return;
    }
    auto ref_base = a_from->GetBaseObject();
    if (!ref_base) {
        logger::error("Ref base is null.");
        return;
    }
    if (!a_to) {
        logger::error("Base is null.");
        return;
    }
    if (ref_base->GetFormID() == a_to->GetFormID()) {
        logger::trace("Ref and base are the same.");
        return;
    }
    a_from->SetObjectReference(a_to);
}

namespace DynamicForm {

    static void copyBookAppearence(RE::TESForm* source, RE::TESForm* target) {
        auto* sourceBook = source->As<RE::TESObjectBOOK>();

        auto* targetBook = target->As<RE::TESObjectBOOK>();

        if (sourceBook && targetBook) {
            targetBook->inventoryModel = sourceBook->inventoryModel;
        }
    }

    template <class T>

    void copyComponent(RE::TESForm* from, RE::TESForm* to) {
        auto fromT = from->As<T>();

        auto toT = to->As<T>();

        if (fromT && toT) {
            toT->CopyComponent(fromT);
        }
    }

    static void copyFormArmorModel(RE::TESForm* source, RE::TESForm* target) {
        auto* sourceModelBipedForm = source->As<RE::TESObjectARMO>();

        auto* targeteModelBipedForm = target->As<RE::TESObjectARMO>();

        if (sourceModelBipedForm && targeteModelBipedForm) {
            logger::info("armor");

            targeteModelBipedForm->armorAddons = sourceModelBipedForm->armorAddons;
        }
    }

    static void copyFormObjectWeaponModel(RE::TESForm* source, RE::TESForm* target) {
        auto* sourceModelWeapon = source->As<RE::TESObjectWEAP>();

        auto* targeteModelWeapon = target->As<RE::TESObjectWEAP>();

        if (sourceModelWeapon && targeteModelWeapon) {
            logger::info("weapon");

            targeteModelWeapon->firstPersonModelObject = sourceModelWeapon->firstPersonModelObject;

            targeteModelWeapon->attackSound = sourceModelWeapon->attackSound;

            targeteModelWeapon->attackSound2D = sourceModelWeapon->attackSound2D;

            targeteModelWeapon->attackSound = sourceModelWeapon->attackSound;

            targeteModelWeapon->attackFailSound = sourceModelWeapon->attackFailSound;

            targeteModelWeapon->idleSound = sourceModelWeapon->idleSound;

            targeteModelWeapon->equipSound = sourceModelWeapon->equipSound;

            targeteModelWeapon->unequipSound = sourceModelWeapon->unequipSound;

            targeteModelWeapon->soundLevel = sourceModelWeapon->soundLevel;
        }
    }

    static void copyMagicEffect(RE::TESForm* source, RE::TESForm* target) {
        auto* sourceEffect = source->As<RE::EffectSetting>();

        auto* targetEffect = target->As<RE::EffectSetting>();

        if (sourceEffect && targetEffect) {
            targetEffect->effectSounds = sourceEffect->effectSounds;

            targetEffect->data.castingArt = sourceEffect->data.castingArt;

            targetEffect->data.light = sourceEffect->data.light;

            targetEffect->data.hitEffectArt = sourceEffect->data.hitEffectArt;

            targetEffect->data.effectShader = sourceEffect->data.effectShader;

            targetEffect->data.hitVisuals = sourceEffect->data.hitVisuals;

            targetEffect->data.enchantShader = sourceEffect->data.enchantShader;

            targetEffect->data.enchantEffectArt = sourceEffect->data.enchantEffectArt;

            targetEffect->data.enchantVisuals = sourceEffect->data.enchantVisuals;

            targetEffect->data.projectileBase = sourceEffect->data.projectileBase;

            targetEffect->data.explosion = sourceEffect->data.explosion;

            targetEffect->data.impactDataSet = sourceEffect->data.impactDataSet;

            targetEffect->data.imageSpaceMod = sourceEffect->data.imageSpaceMod;
        }
    }

    void copyAppearence(RE::TESForm* source, RE::TESForm* target) {
        copyFormArmorModel(source, target);

        copyFormObjectWeaponModel(source, target);

        copyMagicEffect(source, target);

        copyBookAppearence(source, target);

        copyComponent<RE::BGSPickupPutdownSounds>(source, target);

        copyComponent<RE::BGSMenuDisplayObject>(source, target);

        copyComponent<RE::TESModel>(source, target);

        copyComponent<RE::TESBipedModelForm>(source, target);
    }


    void ReviveDynamicForm(RE::TESForm* fake, RE::TESForm* base, const FormID setFormID) { 
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

        } 
        else if (bookBaseForm && bookNewForm) {
            bookNewForm->data.flags = bookBaseForm->data.flags;

            bookNewForm->data.teaches.spell = bookBaseForm->data.teaches.spell;

            bookNewForm->data.teaches.actorValueToAdvance = bookBaseForm->data.teaches.actorValueToAdvance;

            bookNewForm->data.type = bookBaseForm->data.type;

            bookNewForm->inventoryModel = bookBaseForm->inventoryModel;

            bookNewForm->itemCardDescription = bookBaseForm->itemCardDescription;

        } 
        else if (ammoBaseForm && ammoNewForm) {
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
    const RE::FormID CreateFake(T* baseForm, const FormID setFormID = 0) {
        logger::trace("CreateFakeContainer");

        if (!baseForm) {
            logger::error("Real form is null.");

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
        logger::info("Original form id: {:x}", new_form->GetFormID());

        ReviveDynamicForm(new_form, baseForm, setFormID);

        const auto new_formid = new_form->GetFormID();

        logger::info("Created form with type: {}, Base ID: {:x}, Ref ID: {:x}, Name: {}",

                RE::FormTypeToString(new_form->GetFormType()), new_form->GetFormID(), new_form->GetFormID(),

                new_form->GetName());

        // write the new_formid to txt file in a new line and create the file if it doesn't exist
        std::string filepath = "Data/SKSE/Plugins/deneme/created_forms.txt";

        // Open the file for appending
        std::ofstream file(filepath, std::ios::app);
        if (file.is_open()) {
            file << std::hex << std::setw(8) << std::setfill('0') << new_formid << std::endl;
            file.close();
        }


        

        return new_formid;
    }
};


namespace WorldObjects {
    RE::TESObjectREFR* TryToGetRefFromHandle(RE::ObjectRefHandle& handle, unsigned int max_try = 2) {
        RE::TESObjectREFR* ref = nullptr;
        if (auto handle_ref = RE::TESObjectREFR::LookupByHandle(handle.native_handle())) {
            logger::trace("Handle ref found");
            ref = handle_ref.get();
            return ref;
            /*if (!ref->IsDisabled() && !ref->IsMarkedForDeletion() && !ref->IsDeleted()) {
                return ref;
            }*/
        }
        if (handle.get()) {
            ref = handle.get().get();
            return ref;
            /*if (!ref->IsDisabled() && !ref->IsMarkedForDeletion() && !ref->IsDeleted()) {
                return ref;
            }*/
        }
        if (auto ref_ = RE::TESForm::LookupByID<RE::TESObjectREFR>(handle.native_handle())) {
            return ref_;
            /*if (!ref_->IsDisabled() && !ref_->IsMarkedForDeletion() && !ref_->IsDeleted()) {
                return ref_;
            }*/
        }
        if (max_try && handle) return TryToGetRefFromHandle(handle, --max_try);
        return nullptr;
    }
};