#include "Manager.h"


const RE::FormID skull = 0x000319E4;
const RE::FormID sword = 0x00012EB7;
const RE::FormID helmet = 0x00012E4D;
const RE::FormID rawbeef = 0x00065c99;
RE::FormID fakeskull;
RE::FormID fakesword;
RE::FormID fakehelmet;
RE::FormID fakerawbeef;



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
                Utilities::FunctionsSkyrim::WorldObject::SwapObjects(event->objectActivated.get(), RE::TESForm::LookupByID<RE::TESBoundObject>(fakeskull));
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
                    auto ref = Utilities::FunctionsSkyrim::WorldObject::TryToGetRefFromHandle(refhandle);
                    //SwapObjects(ref, RE::TESForm::LookupByID<RE::TESBoundObject>(0x00013940));
                    Utilities::FunctionsSkyrim::WorldObject::SwapObjects(
                        ref, RE::TESForm::LookupByID<RE::TESBoundObject>(skull));
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
                    std::string(item.first->GetName()).empty(), item.first->GetFormType());
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
        if (!Utilities::IsPo3Installed()) {
            logger::error("Po3 is not installed.");
            Utilities::MsgBoxesNotifs::Windows::Po3ErrMsg();
            return;
        }
        DFT = DynamicFormTracker::GetSingleton();
        auto* eventSink = OurEventSink::GetSingleton();
        auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
        eventSourceHolder->AddEventSink<RE::TESFormDeleteEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESActivateEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESContainerChangedEvent>(eventSink);
        SKSE::GetCrosshairRefEventSource()->AddEventSink(eventSink);
    }
    if (message->type == SKSE::MessagingInterface::kPostLoadGame) {

        DFT->ReviveAll();
        DFT->DeleteInactives();

        // Post-load
        auto player = RE::PlayerCharacter::GetSingleton();
        
        std::optional<int> cstmid;
        auto asd = 0;
        const auto base_id = sword;
        FormID fake;
        while (asd > 0) {
            fake = DFT->FetchCreate<RE::TESObjectWEAP>(
                base_id, clib_util::editorID::get_editorID(RE::TESForm::LookupByID(base_id)), cstmid);
            player->AddObjectToContainer(RE::TESForm::LookupByID<RE::TESBoundObject>(fake), nullptr, 1, nullptr);
            RE::TESForm::LookupByID<RE::TESObjectWEAP>(fake)->fullName = "asd";
            logger::info("Created fake with ID: {:x} and name {}", fake,
                         RE::TESForm::LookupByID<RE::TESObjectWEAP>(fake)->GetName());
            asd--;
        }

    }
}


void SaveCallback(SKSE::SerializationInterface* serializationInterface) {
    DFT->SendData();
    if (!DFT->Save(serializationInterface, Settings::kDFDataKey, Settings::kSerializationVersion)) {
        logger::critical("Failed to save Data");
    }
}

void LoadCallback(SKSE::SerializationInterface* serializationInterface) {

    logger::info("Loading Data from skse co-save.");

    DFT->Reset();

    std::uint32_t type;
    std::uint32_t version;
    std::uint32_t length;

    unsigned int cosave_found = 0;
    while (serializationInterface->GetNextRecordInfo(type, version, length)) {
        auto temp = Utilities::DecodeTypeCode(type);

        if (version != Settings::kSerializationVersion) {
            logger::critical("Loaded data has incorrect version. Recieved ({}) - Expected ({}) for Data Key ({})",
                             version, Settings::kSerializationVersion, temp);
            continue;
        }
        switch (type) {
            case Settings::kDFDataKey: {
                logger::trace("Loading Record: {} - Version: {} - Length: {}", temp, version, length);
                if (!DFT->Load(serializationInterface))
                    logger::critical("Failed to Load Data for DFT");
                else
                    cosave_found++;
            } break;
            default:
                logger::critical("Unrecognized Record Type: {}", temp);
                break;
        }
    }

    if (cosave_found == 1) {
        logger::info("Receiving Data.");
        DFT->ReceiveData();
        logger::info("Data loaded from skse co-save.");
    } else
        logger::info("No cosave data found.");

}


void InitializeSerialization() {
    auto* serialization = SKSE::GetSerializationInterface();
    serialization->SetUniqueID(Settings::kDFDataKey);
    serialization->SetSaveCallback(SaveCallback);
    serialization->SetLoadCallback(LoadCallback);
    SKSE::log::trace("Cosave serialization initialized.");
}


SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    InitializeSerialization();
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}