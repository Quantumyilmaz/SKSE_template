#include "Utils.h"

using namespace DynamicForm;

const RE::FormID skull = 0x000319E4;
const RE::FormID sword = 0x00012EB7;
const RE::FormID helmet = 0x00012E4D;
RE::FormID fakeskull;
RE::FormID fakesword;
RE::FormID fakehelmet;



class OurEventSink : public RE::BSTEventSink<RE::TESActivateEvent>,
                     public RE::BSTEventSink<RE::TESContainerChangedEvent>,
                    public RE::BSTEventSink<SKSE::CrosshairRefEvent>,
                     public RE::BSTEventSink<RE::TESFormDeleteEvent>
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
        auto crs_ref = event->crosshairRef.get();
        if (!crs_ref->HasContainer()) return RE::BSEventNotifyControl::kContinue;

        auto inv = crs_ref->GetInventory();
        for (auto& item : inv) {
            logger::info("Item: {} formid {} count {} weight {} empty name {}, formtype {}", item.first->GetName(),
                    item.first->GetFormID(), item.second.first, item.first->GetWeight(),
                    std::string(item.first->GetName()).empty(), item.first->FORMTYPE);
        }

        logger::trace("Crosshair ref.");

        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* event,
                                          RE::BSTEventSource<RE::TESFormDeleteEvent>*) {

        if (!event) return RE::BSEventNotifyControl::kContinue;
        
        logger::info("form with formid {:x} deleted", event->formID);
  //      if (auto temp = RE::TESForm::LookupByID(event->formID)) {
		//	logger::info("form with formid {:x} and name {} deleted", event->formID, temp->GetName());
		//}
  //      else {
		//	logger::info("form with formid {:x} not found", event->formID);
		//}
        return RE::BSEventNotifyControl::kContinue;
    }
};



void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        // Start
        auto* eventSink = OurEventSink::GetSingleton();
        auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
        eventSourceHolder->AddEventSink<RE::TESFormDeleteEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESActivateEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESContainerChangedEvent>(eventSink);
        SKSE::GetCrosshairRefEventSource()->AddEventSink(eventSink);
    }
    if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
        // Post-load
        /*auto player = RE::PlayerCharacter::GetSingleton();
        fakeskull = CreateFake<RE::TESObjectMISC>(RE::TESForm::LookupByID(skull)->As<RE::TESObjectMISC>());
        logger::info("Created fake skull with ID: {:x}", fakeskull);
        player->AddObjectToContainer(RE::TESForm::LookupByID<RE::TESBoundObject>(fakeskull),nullptr,3,nullptr);*/
        /*if (auto bound = RE::TESForm::LookupByID<RE::TESBoundObject>(0xff000c92)) {
            logger::info("Found fake skull with ID: {:x}", bound->GetFormID());
            ReviveDynamicForm(bound, RE::TESForm::LookupByID(skull),0);
        }
        else {
            logger::info("No fake skull found.");
        }*/
        //      SKSE::GetTaskInterface()->AddTask([]() {
        //          if (auto temp = RE::TESForm::LookupByID(0xff000c8f);temp && std::strlen(temp->GetName())==0) {
        //              logger::info("{}", temp->IsDeleted());
        //              temp->SetDelete(true);
        //              logger::info("{}", temp->IsDeleted());
        //              delete temp;
        //	}
        //});
        /*fakesword = CreateFake<RE::TESObjectWEAP>(RE::TESForm::LookupByID(sword)->As<RE::TESObjectWEAP>(),
        0xff000c8d); logger::info("Created fake sword with ID: {:x}", fakesword);*/
        /*fakehelmet =
            CreateFake<RE::TESObjectARMO>(RE::TESForm::LookupByID(helmet)->As<RE::TESObjectARMO>(), 0xff000c95);
        logger::info("Created fake helmet with ID: {:x}", fakehelmet);*/

        /*auto player = RE::PlayerCharacter::GetSingleton();
        auto inv = player->GetInventory();
        for (auto& item : inv) {
            if (item.first->GetFormID() >= 0xFF000000 && !std::strlen(item.first->GetName())) {
                if (item.first->GetFormID() == 0xff000c91) {
                    logger::info("Found fake skull in player inventory.");
                    ReviveDynamicForm(item.first, RE::TESForm::LookupByID(skull), 0xff000c91);
                    fakeskull = item.first->GetFormID();
                }
                else {
                    logger::info("Item is null");
                    player->RemoveItem(item.first, item.second.first, RE::ITEM_REMOVE_REASON::kRemove, nullptr,
        nullptr); continue;
                }
            }
            logger::info("Item: {} formid {} count {} weight {} empty name {}, formtype {}", item.first->GetName(),
                            item.first->GetFormID(),item.second.first, item.first->GetWeight(),
        std::string(item.first->GetName()).empty(),item.first->FORMTYPE);
        }*/

        //DeleteDynamicForm(0xff000c92);

    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}