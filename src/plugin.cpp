#include "Utils.h"

using namespace DynamicForm;

const RE::FormID skull = 0x000319E4;
const RE::FormID sword = 0x00012EB7;
const RE::FormID helmet = 0x00012E4D;
const RE::FormID arrow = 0x0001397d;
RE::FormID fakeskull;
RE::FormID fakesword;
RE::FormID fakehelmet;
RE::FormID fakearrow;



class OurEventSink : public RE::BSTEventSink<RE::TESActivateEvent>,
                     public RE::BSTEventSink<RE::TESContainerChangedEvent>,
                    public RE::BSTEventSink<SKSE::CrosshairRefEvent>,
                    public RE::BSTEventSink<RE::BGSActorCellEvent>
   {
    OurEventSink() = default;
    OurEventSink(const OurEventSink&) = delete;
    OurEventSink(OurEventSink&&) = delete;
    OurEventSink& operator=(const OurEventSink&) = delete;
    OurEventSink& operator=(OurEventSink&&) = delete;

public:
    static OurEventSink* GetSingleton() {
        static OurEventSink singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent* event,
                                          RE::BSTEventSource<RE::TESActivateEvent>*) {
        logger::trace("activated2");
        if (!event) return RE::BSEventNotifyControl::kContinue;
        if (!event->objectActivated) return RE::BSEventNotifyControl::kContinue;
        if (!event->actionRef->IsPlayerRef()) return RE::BSEventNotifyControl::kContinue;
        if (event->objectActivated == RE::PlayerCharacter::GetSingleton()->GetGrabbedRef())
            return RE::BSEventNotifyControl::kContinue;

        if (event->objectActivated->HasContainer()) {
			logger::info("Container");
            auto inv = event->objectActivated->GetInventory();
            for (auto& item : inv) {
				logger::info("Item: {} formid {}", item.first->GetName(),item.first->GetFormID());
			}
		}

        logger::trace("activated1");
        const auto _formid = event->objectActivated->GetBaseObject()->GetFormID();
        switch (_formid) {
            case skull:
                SwapObjects(event->objectActivated.get(), RE::TESForm::LookupByID<RE::TESBoundObject>(fakeskull));
                logger::info("Swapped skull");
			    break;
   //         case sword:
			//	SwapObjects(event->objectActivated.get(), RE::TESForm::LookupByID<RE::TESBoundObject>(fakesword));
   //             logger::info("Swapped sword");
   //             break;
			//case helmet:
   //             SwapObjects(event->objectActivated.get(), RE::TESForm::LookupByID<RE::TESBoundObject>(fakehelmet));
   //             logger::info("Swapped helmet");
			//	break;
			default:
				logger::info("No swap");
				break;
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* event,
                                          RE::BSTEventSource<RE::TESContainerChangedEvent>*) {
    
        // from player inventory ->
        if (event->oldContainer == 20) {
            // a fake container left player inventory
            logger::trace("Item left player inventory.");
            // drop event
            if (!event->newContainer) {
                logger::trace("Dropped.");
                if (event->baseObj == fakeskull) {
                    auto refhandle = event->reference;
                    auto ref = WorldObjects::TryToGetRefFromHandle(refhandle);
                    //SwapObjects(ref, RE::TESForm::LookupByID<RE::TESBoundObject>(0x00013940));
                    SwapObjects(ref, RE::TESForm::LookupByID<RE::TESBoundObject>(skull));
                    /*event->reference.get().get()->Disable();
                    event->reference.get().get()->Enable();*/
                }
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    
    }

    RE::BSEventNotifyControl ProcessEvent(const SKSE::CrosshairRefEvent* event,
                                          RE::BSTEventSource<SKSE::CrosshairRefEvent>*) {
        if (!event->crosshairRef) return RE::BSEventNotifyControl::kContinue;
        //event->crosshairRef->SetActivationBlocked(1);
        auto crs_ref = event->crosshairRef.get();
        if (!crs_ref->HasContainer()) return RE::BSEventNotifyControl::kContinue;

        auto inv = crs_ref->GetInventory();
        for (auto& item : inv) {
            if (item.first->GetFormID() >= 0xFF000000 && !std::strlen(item.first->GetName())) {
                if (item.first->GetFormID() == 0xff000c8f) {
                    logger::info("Found fake skull in player inventory.");
                    ReviveDynamicForm(item.first, RE::TESForm::LookupByID(skull), 0xff000c8f);
                    fakeskull = item.first->GetFormID();
                } else {
                    logger::info("Item is null");
                    crs_ref->RemoveItem(item.first, item.second.first, RE::ITEM_REMOVE_REASON::kRemove, nullptr,
                                       nullptr);
                    continue;
                }
            }
            logger::info("Item: {} formid {} count {} weight {} empty name {}, formtype {}", item.first->GetName(),
                         item.first->GetFormID(), item.second.first, item.first->GetWeight(),
                         std::string(item.first->GetName()).empty(), item.first->FORMTYPE);
        }

        logger::trace("Crosshair ref.");

        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event,
                                          RE::BSTEventSource<RE::BGSActorCellEvent>*) {
        logger::info("Cell event.");
        if (!a_event) return RE::BSEventNotifyControl::kContinue;
        auto eventActorHandle = a_event->actor;
        auto eventActorPtr = eventActorHandle ? eventActorHandle.get() : nullptr;
        auto eventActor = eventActorPtr ? eventActorPtr.get() : nullptr;
        if (!eventActor) return RE::BSEventNotifyControl::kContinue;

        if (eventActor != RE::PlayerCharacter::GetSingleton()) return RE::BSEventNotifyControl::kContinue;

        auto cellID = a_event->cellID;
        auto* cellForm = cellID ? RE::TESForm::LookupByID(cellID) : nullptr;
        auto* cell = cellForm ? cellForm->As<RE::TESObjectCELL>() : nullptr;
        if (!cell) return RE::BSEventNotifyControl::kContinue;


        if (auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(0xff000d53)) {
            logger::info("Found ref with ID: {:x} and formiD {:x}", ref->GetFormID(),
                            ref->GetBaseObject()->GetFormID());
            if (auto projectile = ref->As<RE::Projectile>()) {
                projectile->Load3D(false);
                ref->Disable();
                ref->Enable(false);
                ref->SetActivationBlocked(false);
            }
        }
        else {
			logger::info("No ref found.");
		}

        return RE::BSEventNotifyControl::kContinue;
    }

};



void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        // Start
        auto* eventSink = OurEventSink::GetSingleton();
        auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
        /*eventSourceHolder->AddEventSink<RE::TESActivateEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESContainerChangedEvent>(eventSink);*/
        //SKSE::GetCrosshairRefEventSource()->AddEventSink(eventSink);
        RE::PlayerCharacter::GetSingleton()->AsBGSActorCellEventSource()->AddEventSink(eventSink);
    }
    if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
        // Post-load

        /*if (auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(0xff000d58)) {
            logger::info("Found ref with ID: {:x} and formiD {:x}", ref->GetFormID(),
                         ref->GetBaseObject()->GetFormID());
            if (auto projectile = ref->As<RE::Projectile>()) {
                projectile->Load3D(false);
                ref->Disable();
                ref->Enable(false);
            }
        }*/

    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}