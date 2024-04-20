#pragma once

// #include <chrono>
#include "logger.h"
#include <windows.h>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include "SimpleIni.h"
#include <iostream>
#include <string>
#include <codecvt>
#include <mutex>
#include <algorithm>
#include <ClibUtil/editorID.hpp>
#include "rapidjson/document.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>

namespace Utilities {

    const auto mod_name = static_cast<std::string>(SKSE::PluginDeclaration::GetSingleton()->GetName());
    constexpr auto po3path = "Data/SKSE/Plugins/po3_Tweaks.dll";
    constexpr auto po3_UoTpath = "Data/SKSE/Plugins/po3_UseOrTake.dll";
    bool IsPo3Installed() { return std::filesystem::exists(po3path); };
    bool IsPo3_UoTInstalled() { return std::filesystem::exists(po3_UoTpath); };

    const auto po3_err_msgbox = std::format(
        "{}: If you are trying to use Editor IDs, but you must have powerofthree's Tweaks "
        "installed. See mod page for further instructions.",
        mod_name);

    const auto general_err_msgbox = std::format("{}: Something went wrong. Please contact the mod author.", mod_name);

    const auto init_err_msgbox = std::format("{}: The mod failed to initialize and will be terminated.", mod_name);

    std::string dec2hex(unsigned int dec) {
        std::stringstream stream;
        stream << std::hex << dec;
        std::string hexString = stream.str();
        return hexString;
    };

    std::string DecodeTypeCode(std::uint32_t typeCode) {
        char buf[4];
        buf[3] = char(typeCode);
        buf[2] = char(typeCode >> 8);
        buf[1] = char(typeCode >> 16);
        buf[0] = char(typeCode >> 24);
        return std::string(buf, buf + 4);
    }

    namespace MsgBoxesNotifs {

        // https://github.com/SkyrimScripting/MessageBox/blob/ac0ea32af02766582209e784689eb0dd7d731d57/include/SkyrimScripting/MessageBox.h#L9
        class SkyrimMessageBox {
            class MessageBoxResultCallback : public RE::IMessageBoxCallback {
                std::function<void(unsigned int)> _callback;

            public:
                ~MessageBoxResultCallback() override {}
                MessageBoxResultCallback(std::function<void(unsigned int)> callback) : _callback(callback) {}
                void Run(RE::IMessageBoxCallback::Message message) override {
                    _callback(static_cast<unsigned int>(message));
                }
            };

        public:
            static void Show(const std::string& bodyText, std::vector<std::string> buttonTextValues,
                             std::function<void(unsigned int)> callback) {
                auto* factoryManager = RE::MessageDataFactoryManager::GetSingleton();
                auto* uiStringHolder = RE::InterfaceStrings::GetSingleton();
                auto* factory = factoryManager->GetCreator<RE::MessageBoxData>(
                    uiStringHolder->messageBoxData);  // "MessageBoxData" <--- can we just use this string?
                auto* messagebox = factory->Create();
                RE::BSTSmartPointer<RE::IMessageBoxCallback> messageCallback =
                    RE::make_smart<MessageBoxResultCallback>(callback);
                messagebox->callback = messageCallback;
                messagebox->bodyText = bodyText;
                for (auto& text : buttonTextValues) messagebox->buttonText.push_back(text.c_str());
                messagebox->QueueMessage();
            }
        };

        void ShowMessageBox(const std::string& bodyText, std::vector<std::string> buttonTextValues,
                            std::function<void(unsigned int)> callback) {
            SkyrimMessageBox::Show(bodyText, buttonTextValues, callback);
        }

        namespace Windows {

            int GeneralErr() {
                MessageBoxA(nullptr, general_err_msgbox.c_str(), "Error", MB_OK | MB_ICONERROR);
                return 1;
            };

            int Po3ErrMsg() {
                MessageBoxA(nullptr, po3_err_msgbox.c_str(), "Error", MB_OK | MB_ICONERROR);
                return 1;
            };
        };

        namespace InGame {

            void IniCreated() { RE::DebugMessageBox("INI created. Customize it to your liking."); };

            void InitErr() { RE::DebugMessageBox(init_err_msgbox.c_str()); };

            void GeneralErr() { RE::DebugMessageBox(general_err_msgbox.c_str()); };

            void FormTypeErr(RE::FormID id) {
                RE::DebugMessageBox(std::format("{}: The form type of the item with FormID ({}) is not supported. "
                                                "Please contact the mod author.",
                                                Utilities::mod_name, Utilities::dec2hex(id))
                                        .c_str());
            };

            void FormIDError(RE::FormID id) {
                RE::DebugMessageBox(std::format("{}: The ID ({}) could not have been found.", Utilities::mod_name,
                                                Utilities::dec2hex(id))
                                        .c_str());
            }

            void EditorIDError(std::string id) {
                RE::DebugMessageBox(
                    std::format("{}: The ID ({}) could not have been found.", Utilities::mod_name, id).c_str());
            }

            void ProblemWithContainer(std::string id) {
                RE::DebugMessageBox(
                    std::format(
                        "{}: Problem with one of the items with the form id ({}). This is expected if you have changed "
                        "the list of containers in the INI file between saves. Corresponding items will be returned to "
                        "your inventory. You can suppress this message by changing the setting in your INI.",
                        Utilities::mod_name, id)
                        .c_str());
            };

            void UninstallSuccessful() {
                RE::DebugMessageBox(
                    std::format("{}: Uninstall successful. You can now safely remove the mod.", Utilities::mod_name)
                        .c_str());
            };

            void UninstallFailed() {
                RE::DebugMessageBox(
                    std::format("{}: Uninstall failed. Please contact the mod author.", Utilities::mod_name).c_str());
            };

            void CustomErrMsg(const std::string& msg) { RE::DebugMessageBox((mod_name + ": " + msg).c_str()); };
        };
    };

    // Utility functions
    namespace Functions {

        template <typename Key, typename Value>
        bool containsValue(const std::map<Key, Value>& myMap, const Value& valueToFind) {
            for (const auto& pair : myMap) {
                if (pair.second == valueToFind) {
                    return true;
                }
            }
            return false;
        }

        std::string GetPluginVersion(const unsigned int n_stellen) {
            const auto fullVersion = SKSE::PluginDeclaration::GetSingleton()->GetVersion();
            unsigned int i = 1;
            std::string version = std::to_string(fullVersion.major());
            if (n_stellen == i) return version;
            version += "." + std::to_string(fullVersion.minor());
            if (n_stellen == ++i) return version;
            version += "." + std::to_string(fullVersion.patch());
            if (n_stellen == ++i) return version;
            version += "." + std::to_string(fullVersion.build());
            return version;
        }

        template <typename KeyType, typename ValueType>
        std::vector<KeyType> getKeys(const std::map<KeyType, ValueType>& inputMap) {
            std::vector<KeyType> keys;
            for (const auto& pair : inputMap) {
                keys.push_back(pair.first);
            }
            return keys;
        }

        namespace Vector {

            template <typename T>
            std::vector<T> mergeVectors(const std::vector<T>& vec1, const std::vector<T>& vec2) {
                std::vector<T> mergedVec;

                // Reserve enough space to avoid frequent reallocation
                mergedVec.reserve(vec1.size() + vec2.size());

                // Insert elements from vec1
                mergedVec.insert(mergedVec.end(), vec1.begin(), vec1.end());

                // Insert elements from vec2
                mergedVec.insert(mergedVec.end(), vec2.begin(), vec2.end());

                return mergedVec;
            }

            template <typename T>
            bool HasElement(const std::vector<T>& vec, const T& element) {
                return std::find(vec.begin(), vec.end(), element) != vec.end();
            }

            std::vector<int> getComplementarySet(const std::vector<int>& reference, const std::vector<int>& subset) {
                std::vector<int> complementarySet;
                for (const auto& element : reference) {
                    if (std::find(subset.begin(), subset.end(), element) == subset.end()) {
                        complementarySet.push_back(element);
                    }
                }
                return complementarySet;
            }

            std::vector<std::string> SetToVector(const std::set<std::string>& input_set) {
                // Construct a vector using the range constructor
                std::vector<std::string> result(input_set.begin(), input_set.end());
                return result;
            }

        };

        namespace String {

            template <typename T>
            std::string join(const T& container, const std::string_view& delimiter) {
                std::ostringstream oss;
                auto iter = container.begin();

                if (iter != container.end()) {
                    oss << *iter;
                    ++iter;
                }

                for (; iter != container.end(); ++iter) {
                    oss << delimiter << *iter;
                }

                return oss.str();
            }

            std::string trim(const std::string& str) {
                // Find the first non-whitespace character from the beginning
                size_t start = str.find_first_not_of(" \t\n\r");

                // If the string is all whitespace, return an empty string
                if (start == std::string::npos) return "";

                // Find the last non-whitespace character from the end
                size_t end = str.find_last_not_of(" \t\n\r");

                // Return the substring containing the trimmed characters
                return str.substr(start, end - start + 1);
            }

            std::string toLowercase(const std::string& str) {
                std::string result = str;
                std::transform(result.begin(), result.end(), result.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return result;
            }

            std::string replaceLineBreaksWithSpace(const std::string& input) {
                std::string result = input;
                std::replace(result.begin(), result.end(), '\n', ' ');
                return result;
            }

            bool includesString(const std::string& input, const std::vector<std::string>& strings) {
                std::string lowerInput = toLowercase(input);

                for (const auto& str : strings) {
                    std::string lowerStr = str;
                    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    if (lowerInput.find(lowerStr) != std::string::npos) {
                        return true;  // The input string includes one of the strings
                    }
                }
                return false;  // None of the strings in 'strings' were found in the input string
            }

            // if it includes any of the words in the vector
            bool includesWord(const std::string& input, const std::vector<std::string>& strings) {
                std::string lowerInput = toLowercase(input);
                lowerInput = replaceLineBreaksWithSpace(lowerInput);
                lowerInput = trim(lowerInput);
                lowerInput = " " + lowerInput + " ";  // Add spaces to the beginning and end of the string

                for (const auto& str : strings) {
                    std::string lowerStr = str;
                    lowerStr = trim(lowerStr);
                    lowerStr = " " + lowerStr + " ";  // Add spaces to the beginning and end of the string
                    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                    // logger::trace("lowerInput: {} lowerStr: {}", lowerInput, lowerStr);

                    if (lowerInput.find(lowerStr) != std::string::npos) {
                        return true;  // The input string includes one of the strings
                    }
                }
                return false;  // None of the strings in 'strings' were found in the input string
            }

            std::vector<std::pair<int, bool>> encodeString(const std::string& inputString) {
                std::vector<std::pair<int, bool>> encodedValues;
                try {
                    for (int i = 0; i < 100 && inputString[i] != '\0'; i++) {
                        char ch = inputString[i];
                        if (std::isprint(ch) && (std::isalnum(ch) || std::isspace(ch) || std::ispunct(ch)) && ch >= 0 &&
                            ch <= 255) {
                            encodedValues.push_back(std::make_pair(static_cast<int>(ch), std::isupper(ch)));
                        }
                    }
                } catch (const std::exception& e) {
                    logger::error("Error encoding string: {}", e.what());
                    return encodeString("ERROR");
                }
                return encodedValues;
            }

            std::string decodeString(const std::vector<std::pair<int, bool>>& encodedValues) {
                std::string decodedString;
                for (const auto& pair : encodedValues) {
                    char ch = static_cast<char>(pair.first);
                    if (std::isalnum(ch) || std::isspace(ch) || std::ispunct(ch)) {
                        if (pair.second) {
                            decodedString += ch;
                        } else {
                            decodedString += static_cast<char>(std::tolower(ch));
                        }
                    }
                }
                return decodedString;
            }

        };

        bool isValidHexWithLength7or8(const char* input) {
            std::string inputStr(input);

            if (inputStr.substr(0, 2) == "0x") {
                // Remove "0x" from the beginning of the string
                inputStr = inputStr.substr(2);
            }

            std::regex hexRegex("^[0-9A-Fa-f]{7,8}$");  // Allow 7 to 8 characters
            bool isValid = std::regex_match(inputStr, hexRegex);
            return isValid;
        }
    };

    namespace Math {
        namespace LinAlg {
            namespace R3 {
                void rotateX(RE::NiPoint3& v, float angle) {
                    float y = v.y * cos(angle) - v.z * sin(angle);
                    float z = v.y * sin(angle) + v.z * cos(angle);
                    v.y = y;
                    v.z = z;
                }

                // Function to rotate a vector around the y-axis
                void rotateY(RE::NiPoint3& v, float angle) {
                    float x = v.x * cos(angle) + v.z * sin(angle);
                    float z = -v.x * sin(angle) + v.z * cos(angle);
                    v.x = x;
                    v.z = z;
                }

                // Function to rotate a vector around the z-axis
                void rotateZ(RE::NiPoint3& v, float angle) {
                    float x = v.x * cos(angle) - v.y * sin(angle);
                    float y = v.x * sin(angle) + v.y * cos(angle);
                    v.x = x;
                    v.y = y;
                }

                void rotate(RE::NiPoint3& v, float angleX, float angleY, float angleZ) {
                    rotateX(v, angleX);
                    rotateY(v, angleY);
                    rotateZ(v, angleZ);
                }
            };
        };
    };

    namespace FunctionsSkyrim {

        RE::TESForm* GetFormByID(const RE::FormID id, const std::string& editor_id = "") {
            if (!editor_id.empty()) {
                auto* form = RE::TESForm::LookupByEditorID(editor_id);
                if (form) return form;
            }
            auto form = RE::TESForm::LookupByID(id);
            if (form) return form;
            return nullptr;
        };

        template <class T = RE::TESForm>
        static T* GetFormByID(const RE::FormID id, const std::string& editor_id = "") {
            if (!editor_id.empty()) {
                auto* form = RE::TESForm::LookupByEditorID<T>(editor_id);
                if (form) return form;
            }
            T* form = RE::TESForm::LookupByID<T>(id);
            if (form) return form;
            return nullptr;
        };

        const std::string GetEditorID(const FormID a_formid) {
            if (const auto form = RE::TESForm::LookupByID(a_formid)) {
                return clib_util::editorID::get_editorID(form);
            } else {
                return "";
            }
        }

        FormID GetFormEditorIDFromString(const std::string formEditorId) {
            logger::trace("Getting formid from editorid: {}", formEditorId);
            if (Utilities::Functions::isValidHexWithLength7or8(formEditorId.c_str())) {
                logger::trace("formEditorId is in hex format.");
                int form_id_;
                std::stringstream ss;
                ss << std::hex << formEditorId;
                ss >> form_id_;
                auto temp_form = GetFormByID(form_id_, "");
                if (temp_form)
                    return temp_form->GetFormID();
                else {
                    logger::error("Formid is null for editorid {}", formEditorId);
                    return 0;
                }
            }
            if (formEditorId.empty())
                return 0;
            else if (!IsPo3Installed()) {
                logger::error("Po3 is not installed.");
                MsgBoxesNotifs::Windows::Po3ErrMsg();
                return 0;
            }
            auto temp_form = GetFormByID(0, formEditorId);
            if (temp_form) {
                logger::trace("Formid is not null with formid {}", temp_form->GetFormID());
                return temp_form->GetFormID();
            } else {
                logger::info("Formid is null for editorid {}", formEditorId);
                return 0;
            }
        }

        std::size_t GetExtraDataListLength(const RE::ExtraDataList* dataList) {
            std::size_t length = 0;

            for (auto it = dataList->begin(); it != dataList->end(); ++it) {
                // Increment the length for each element in the list
                ++length;
            }

            return length;
        }

        template <typename T>
        std::size_t GetListLength(RE::BSSimpleList<T>* dataList) {
            std::size_t length = 0;

            for (auto it = dataList->begin(); it != dataList->end(); ++it) {
                // Increment the length for each element in the list
                ++length;
            }

            return length;
        }

        bool FormIsOfType(RE::TESForm* form, RE::FormType type) {
            if (!form) return false;
            return form->GetFormType() == type;
        }

        bool FormIsOfType(const FormID formid, RE::FormType type) { return FormIsOfType(GetFormByID(formid), type); }

        bool IsFoodItem(RE::TESForm* form) {
            if (FormIsOfType(form, RE::AlchemyItem::FORMTYPE)) {
                RE::AlchemyItem* form_as_ = form->As<RE::AlchemyItem>();
                if (!form_as_) return false;
                if (!form_as_->IsFood()) return false;
            } else if (FormIsOfType(form, RE::IngredientItem::FORMTYPE)) {
                RE::IngredientItem* form_as_ = form->As<RE::IngredientItem>();
                if (!form_as_) return false;
                if (!form_as_->IsFood()) return false;

            }
            // else if(FormIsOfType(form,RE::MagicItem::FORMTYPE)){
            //     RE::MagicItem* form_as_ = form->As<RE::MagicItem>();
            //     if (!form_as_) return false;
            //     if (!form_as_->IsFood()) return false;
            // }
            else
                return false;
            return true;
        }

        bool IsFoodItem(const FormID formid) { return IsFoodItem(GetFormByID(formid)); }

        bool IsPoisonItem(RE::TESForm* form) {
            if (FormIsOfType(form, RE::AlchemyItem::FORMTYPE)) {
                RE::AlchemyItem* form_as_ = form->As<RE::AlchemyItem>();
                if (!form_as_) return false;
                if (!form_as_->IsPoison()) return false;
            } else
                return false;
            return true;
        }

        bool IsPoisonItem(const FormID formid) { return IsPoisonItem(GetFormByID(formid)); }

        bool IsMedicineItem(RE::TESForm* form) {
            if (FormIsOfType(form, RE::AlchemyItem::FORMTYPE)) {
                RE::AlchemyItem* form_as_ = form->As<RE::AlchemyItem>();
                if (!form_as_) return false;
                if (!form_as_->IsMedicine()) return false;
            } else
                return false;
            return true;
        }

        bool IsMedicineItem(const FormID formid) { return IsPoisonItem(GetFormByID(formid)); }

        bool IsCONT(RefID refid) {
            return RE::FormTypeToString(
                       RE::TESForm::LookupByID<RE::TESObjectREFR>(refid)->GetObjectReference()->GetFormType()) ==
                   "CONT";
        }

        template <typename T>
        struct FormTraits {
            static float GetWeight(T* form) {
                // Default implementation, assuming T has a member variable 'weight'
                return form->weight;
            }

            static void SetWeight(T* form, float weight) {
                // Default implementation, set the weight if T has a member variable 'weight'
                form->weight = weight;
            }

            static int GetValue(T* form) {
                // Default implementation, assuming T has a member variable 'value'
                return form->value;
            }

            static void SetValue(T* form, int value) { form->value = value; }

            static RE::BSTArray<RE::Effect*> GetEffects(T*) {
                RE::BSTArray<RE::Effect*> effects;
                return effects;
            }
        };

        template <>
        struct FormTraits<RE::AlchemyItem> {
            static float GetWeight(RE::AlchemyItem* form) { return form->weight; }

            static void SetWeight(RE::AlchemyItem* form, float weight) { form->weight = weight; }

            static int GetValue(RE::AlchemyItem* form) { return form->GetGoldValue(); }
            static void SetValue(RE::AlchemyItem* form, int value) {
                logger::trace("CostOverride: {}", form->data.costOverride);
                form->data.costOverride = value;
            }

            static RE::BSTArray<RE::Effect*> GetEffects(RE::AlchemyItem* form) { return form->effects; }
        };

        template <>
        struct FormTraits<RE::IngredientItem> {
            static float GetWeight(RE::IngredientItem* form) { return form->weight; }

            static void SetWeight(RE::IngredientItem* form, float weight) { form->weight = weight; }

            static int GetValue(RE::IngredientItem* form) { return form->GetGoldValue(); }
            static void SetValue(RE::IngredientItem* form, int value) { form->value = value; }

            static RE::BSTArray<RE::Effect*> GetEffects(RE::IngredientItem* form) { return form->effects; }
        };

        template <>
        struct FormTraits<RE::TESAmmo> {
            static float GetWeight(RE::TESAmmo* form) {
                // Default implementation, assuming T has a member variable 'weight'
                return form->GetWeight();
            }

            static void SetWeight(RE::TESAmmo*, float) {
                // Default implementation, set the weight if T has a member variable 'weight'
                return;
            }

            static int GetValue(RE::TESAmmo* form) {
                // Default implementation, assuming T has a member variable 'value'
                return form->value;
            }

            static void SetValue(RE::TESAmmo* form, int value) { form->value = value; }

            static RE::BSTArray<RE::Effect*> GetEffects(RE::TESAmmo*) {
                RE::BSTArray<RE::Effect*> effects;
                return effects;
            }
        };

        // credits to Qudix on xSE RE Discord for this
        void OpenContainer(RE::TESObjectREFR* a_this, std::uint32_t a_openType) {
            // a_openType is probably in alignment with RE::ContainerMenu::ContainerMode enum
            using func_t = decltype(&OpenContainer);
            REL::Relocation<func_t> func{RELOCATION_ID(50211, 51140)};
            func(a_this, a_openType);
        }

        void OverrideMGEFFs(RE::BSTArray<RE::Effect*>& effect_array, std::vector<FormID> new_effects,
                            std::vector<uint32_t> durations, std::vector<float> magnitudes) {
            size_t some_index = 0;
            for (auto* effect : effect_array) {
                auto* other_eff = GetFormByID<RE::EffectSetting>(new_effects[some_index]);
                if (!other_eff) {
                    effect->effectItem.duration = 0;
                    effect->effectItem.magnitude = 0;
                } else {
                    effect->baseEffect = other_eff;
                    effect->effectItem.duration = durations[some_index];
                    effect->effectItem.magnitude = magnitudes[some_index];
                }
                some_index++;
            }
        }

        const bool FormExists(const FormID formid, std::string editorid = "") {
            if (GetFormByID(formid, editorid)) return true;
            return false;
        }

        namespace Menu {

            void CloseMenu(const std::string_view menuname) {
                if (auto ui = RE::UI::GetSingleton()) {
                    if (!ui->IsMenuOpen(menuname)) return;
                }
                RE::BSFixedString menuName(menuname);
                if (const auto queue = RE::UIMessageQueue::GetSingleton()) {
                    queue->AddMessage(menuName, RE::UI_MESSAGE_TYPE::kHide, nullptr);
                }
            }

            // po3
            void SendInventoryUpdateMessage(RE::TESObjectREFR* a_inventoryRef, const RE::TESBoundObject* a_updateObj) {
                using func_t = decltype(&SendInventoryUpdateMessage);
                static REL::Relocation<func_t> func{RELOCATION_ID(51911, 52849)};
                return func(a_inventoryRef, a_updateObj);
            }

            // https://
            // github.com/Exit-9B/Dont-Eat-Spell-Tomes/blob/7b7f97353cc6e7ccfad813661f39710b46d82972/src/SpellTomeManager.cpp#L23-L32
            template <typename T>
            RE::TESObjectREFR* GetMenuOwner() {
                RE::TESObjectREFR* reference = nullptr;
                const auto ui = RE::UI::GetSingleton();
                const auto menu = ui ? ui->GetMenu<T>() : nullptr;
                const auto movie = menu ? menu->uiMovie : nullptr;

                if (movie) {
                    // Parapets: "Menu_mc" is stored in the class, but it's different in VR and we haven't RE'd it yet
                    RE::GFxValue isViewingContainer;
                    movie->Invoke("Menu_mc.isViewingContainer", &isViewingContainer, nullptr, 0);

                    if (isViewingContainer.GetBool()) {
                        auto refHandle = menu->GetTargetRefHandle();
                        RE::TESObjectREFRPtr refr;
                        RE::LookupReferenceByHandle(refHandle, refr);
                        return refr.get();
                    }
                }
                return reference;
            }

            RE::TESObjectREFR* GetContainerFromMenu() {
                if (auto ui_refid = RE::UI::GetSingleton()->GetMenu<RE::ContainerMenu>()->GetTargetRefHandle()) {
                    logger::trace("UI Reference id {}", ui_refid);
                    if (auto ui_ref = RE::TESObjectREFR::LookupByHandle(ui_refid)) {
                        logger::trace("UI Reference name {}", ui_ref->GetDisplayFullName());
                        return ui_ref.get();
                    }
                }
                return nullptr;
            }

            RE::TESObjectREFR* GetVendorChestFromMenu() {
                if (auto ui_refid = RE::UI::GetSingleton()->GetMenu<RE::BarterMenu>()->GetTargetRefHandle()) {
                    logger::trace("UI Reference id {}", ui_refid);
                    if (auto ui_ref = RE::TESObjectREFR::LookupByHandle(ui_refid)) {
                        logger::trace("UI Reference name {}", ui_ref->GetDisplayFullName());
                        if (auto barter_npc = ui_ref->GetBaseObject()->As<RE::TESNPC>()) {
                            for (auto& faction_rank : barter_npc->factions) {
                                if (auto merchant_chest = faction_rank.faction->vendorData.merchantContainer) {
                                    logger::trace("Found chest with refid {}", merchant_chest->GetFormID());
                                    return merchant_chest;
                                }
                            };
                        }
                    }
                }
                return nullptr;
            }

            // not tested!
            template <typename T>
            void RefreshItemList(RE::TESObjectREFR* inventory_owner) {
                if (!inventory_owner) {
                    logger::error("Inventory owner is null.");
                    return;
                }
                if (T::MENU_NAME != RE::BarterMenu::MENU_NAME && T::MENU_NAME != RE::ContainerMenu::MENU_NAME &&
                    T::MENU_NAME != RE::InventoryMenu::MENU_NAME) {
                    logger::error("Menu type not supported: {}", T::MENU_NAME);
                    return;
                }
                auto ui = RE::UI::GetSingleton();
                /*if (!ui->IsMenuOpen(T::MENU_NAME)){
                    logger::error("Menu is not open.");
                    return;
                }*/

                std::map<FormID, Count> item_map;
                auto inventory = inventory_owner->GetInventory();
                for (auto& [item, entry] : inventory) {
                    item_map[item->GetFormID()] = entry.first;
                }
                auto inventory_menu = ui->GetMenu<T>();
                if (inventory_menu) {
                    if (auto itemlist = inventory_menu->GetRuntimeData().itemList) {
                        logger::trace("Updating itemlist.");
                        for (auto* item : itemlist->items) {
                            auto temp_entry = item->data.objDesc;
                            if (!temp_entry) {
                                logger::error("Item entry is null.");
                                continue;
                            }
                            auto temp_obj = temp_entry->object;
                            if (!temp_obj) {
                                logger::error("Item object is null.");
                                continue;
                            }
                            item_map[temp_obj->GetFormID()] -= item->data.GetCount();
                        }
                    } else
                        logger::info("Itemlist is null.");
                } else
                    logger::info("Inventory menu is null.");

                for (auto& [formid, count] : item_map) {
                    if (count > 0) {
                        auto item = GetFormByID<RE::TESBoundObject>(formid);
                        if (!item) {
                            logger::error("Item is null.");
                            continue;
                        }
                        logger::trace("Sending inventory update message: {}", item->GetName());
                        SendInventoryUpdateMessage(inventory_owner, item);
                    }
                }
            }

            template <typename T>
            void RefreshItemList(RE::TESObjectREFR* inventory_owner, const std::vector<FormID> formids) {
                for (auto& formid : formids) {
                    auto item = GetFormByID<RE::TESBoundObject>(formid);
                    if (!item) {
                        logger::error("Item is null.");
                        continue;
                    }
                    SendInventoryUpdateMessage(inventory_owner, item);
                }
            }

            template <typename T>
            void UpdateItemList() {
                if (auto ui = RE::UI::GetSingleton(); ui->IsMenuOpen(T::MENU_NAME)) {
                    if (auto inventory_menu = ui->GetMenu<T>()) {
                        if (auto itemlist = inventory_menu->GetRuntimeData().itemList) {
                            logger::trace("Updating itemlist.");
                            itemlist->Update();
                        } else
                            logger::info("Itemlist is null.");
                    } else
                        logger::info("Inventory menu is null.");
                } else
                    logger::info("Inventory menu is not open.");
            }
        };

        namespace Inventory {

            const bool EntryHasXData(RE::InventoryEntryData* entry) {
                if (entry && entry->extraLists && !entry->extraLists->empty()) return true;
                return false;
            }

            const bool HasItemEntry(RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner,
                                    bool nonzero_entry_check = false) {
                if (!item) {
                    logger::warn("Item is null");
                    return 0;
                }
                if (!inventory_owner) {
                    logger::warn("Inventory owner is null");
                    return 0;
                }
                auto inventory = inventory_owner->GetInventory();
                auto it = inventory.find(item);
                bool has_entry = it != inventory.end();
                if (nonzero_entry_check) return has_entry && it->second.first > 0;
                return has_entry;
            }

            const std::int32_t GetItemCount(RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner) {
                if (HasItemEntry(item, inventory_owner, true)) {
                    auto inventory = inventory_owner->GetInventory();
                    auto it = inventory.find(item);
                    return it->second.first;
                }
                return 0;
            }

            const bool HasItem(RE::TESBoundObject* item, RE::TESObjectREFR* item_owner) {
                if (HasItemEntry(item, item_owner, true)) return true;
                return false;
            }

            void AddItem(RE::TESObjectREFR* addTo, RE::TESObjectREFR* addFrom, FormID item_id, Count count,
                         RE::ExtraDataList* xList = nullptr) {
                logger::trace("AddItem");
                // xList = nullptr;
                if (!addTo && !addFrom) {
                    logger::error("addTo and addFrom are both null!");
                    return;
                }

                logger::trace("Adding item.");

                auto bound = RE::TESForm::LookupByID<RE::TESBoundObject>(item_id);
                addTo->AddObjectToContainer(bound, xList, count, addFrom);
            }

            void RemoveAll(RE::TESBoundObject* item, RE::TESObjectREFR* item_owner) {
                if (!item) return;
                if (!item_owner) return;
                auto inventory = item_owner->GetInventory();
                auto it = inventory.find(item);
                bool has_entry = it != inventory.end();
                if (!has_entry) return;
                item_owner->RemoveItem(item, std::min(it->second.first, 1), RE::ITEM_REMOVE_REASON::kRemove, nullptr,
                                       nullptr);
            }

            void FavoriteItem(RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner) {
                if (!item) return;
                if (!inventory_owner) return;
                auto inventory_changes = inventory_owner->GetInventoryChanges();
                auto entries = inventory_changes->entryList;
                for (auto it = entries->begin(); it != entries->end(); ++it) {
                    if (!(*it)) {
                        logger::error("Item entry is null");
                        continue;
                    }
                    const auto object = (*it)->object;
                    if (!object) {
                        logger::error("Object is null");
                        continue;
                    }
                    auto formid = object->GetFormID();
                    if (!formid) logger::critical("Formid is null");
                    if (formid == item->GetFormID()) {
                        logger::trace("Favoriting item: {}", item->GetName());
                        const auto xLists = (*it)->extraLists;
                        bool no_extra_ = false;
                        if (!xLists || xLists->empty()) {
                            logger::trace("No extraLists");
                            no_extra_ = true;
                        }
                        logger::trace("asdasd");
                        if (no_extra_) {
                            logger::trace("No extraLists");
                            // inventory_changes->SetFavorite((*it), nullptr);
                        } else if (xLists->front()) {
                            logger::trace("ExtraLists found");
                            inventory_changes->SetFavorite((*it), xLists->front());
                        }
                        return;
                    }
                }
                logger::error("Item not found in inventory");
            }

            void FavoriteItem(const FormID formid, const FormID refid) {
                FavoriteItem(GetFormByID<RE::TESBoundObject>(formid), GetFormByID<RE::TESObjectREFR>(refid));
            }

            [[nodiscard]] const bool HasItemPlusCleanUp(RE::TESBoundObject* item, RE::TESObjectREFR* item_owner,
                                                        RE::ExtraDataList* xList = nullptr) {
                logger::trace("HasItemPlusCleanUp");

                if (HasItem(item, item_owner)) return true;
                if (HasItemEntry(item, item_owner)) {
                    item_owner->RemoveItem(item, 1, RE::ITEM_REMOVE_REASON::kRemove, xList, nullptr);
                    logger::trace("Item with zero count removed from player.");
                }
                return false;
            }

            [[nodiscard]] const bool IsFavorited(RE::TESBoundObject* item, RE::TESObjectREFR* inventory_owner) {
                if (!item) {
                    logger::warn("Item is null");
                    return false;
                }
                if (!inventory_owner) {
                    logger::warn("Inventory owner is null");
                    return false;
                }
                auto inventory = inventory_owner->GetInventory();
                auto it = inventory.find(item);
                if (it != inventory.end()) {
                    if (it->second.first <= 0) logger::warn("Item count is 0");
                    return it->second.second->IsFavorited();
                }
                return false;
            }

            [[nodiscard]] const bool IsFavorited(RE::FormID formid, RE::FormID refid) {
                return IsFavorited(GetFormByID<RE::TESBoundObject>(formid), GetFormByID<RE::TESObjectREFR>(refid));
            }

            [[nodiscard]] const bool IsPlayerFavorited(RE::TESBoundObject* item) {
                return IsFavorited(item, RE::PlayerCharacter::GetSingleton()->AsReference());
            }

            [[nodiscard]] const bool IsEquipped(RE::TESBoundObject* item) {
                logger::trace("IsEquipped");

                if (!item) {
                    logger::trace("Item is null");
                    return false;
                }

                auto player_ref = RE::PlayerCharacter::GetSingleton();
                auto inventory = player_ref->GetInventory();
                auto it = inventory.find(item);
                if (it != inventory.end()) {
                    if (it->second.first <= 0) logger::warn("Item count is 0");
                    return it->second.second->IsWorn();
                }
                return false;
            }

            [[nodiscard]] const bool IsEquipped(const FormID formid) {
                return IsEquipped(GetFormByID<RE::TESBoundObject>(formid));
            }

            void EquipItem(RE::TESBoundObject* item, bool unequip = false) {
                logger::trace("EquipItem");

                if (!item) {
                    logger::error("Item is null");
                    return;
                }
                auto player_ref = RE::PlayerCharacter::GetSingleton();
                auto inventory_changes = player_ref->GetInventoryChanges();
                auto entries = inventory_changes->entryList;
                for (auto it = entries->begin(); it != entries->end(); ++it) {
                    auto formid = (*it)->object->GetFormID();
                    if (formid == item->GetFormID()) {
                        if (!(*it) || !(*it)->extraLists) {
                            logger::error("Item extraLists is null");
                            return;
                        }
                        if (unequip) {
                            if ((*it)->extraLists->empty()) {
                                RE::ActorEquipManager::GetSingleton()->UnequipObject(
                                    player_ref, (*it)->object, nullptr, 1, (const RE::BGSEquipSlot*)nullptr, true,
                                    false, false);
                            } else if ((*it)->extraLists->front()) {
                                RE::ActorEquipManager::GetSingleton()->UnequipObject(
                                    player_ref, (*it)->object, (*it)->extraLists->front(), 1,
                                    (const RE::BGSEquipSlot*)nullptr, true, false, false);
                            }
                        } else {
                            if ((*it)->extraLists->empty()) {
                                RE::ActorEquipManager::GetSingleton()->EquipObject(player_ref, (*it)->object, nullptr,
                                                                                   1, (const RE::BGSEquipSlot*)nullptr,
                                                                                   true, false, false, false);
                            } else if ((*it)->extraLists->front()) {
                                RE::ActorEquipManager::GetSingleton()->EquipObject(
                                    player_ref, (*it)->object, (*it)->extraLists->front(), 1,
                                    (const RE::BGSEquipSlot*)nullptr, true, false, false, false);
                            }
                        }
                        return;
                    }
                }
            }

            void EquipItem(const FormID formid, bool unequip = false) {
                EquipItem(GetFormByID<RE::TESBoundObject>(formid), unequip);
            }

            bool IsQuestItem(const FormID formid, RE::TESObjectREFR* inv_owner) {
                const auto inventory = inv_owner->GetInventory();
                const auto item = GetFormByID<RE::TESBoundObject>(formid);
                if (item) {
                    const auto it = inventory.find(item);
                    if (it != inventory.end()) {
                        if (it->second.second->IsQuestObject()) return true;
                    }
                }
                return false;
            }
        };

        namespace WorldObject {

            template <typename T>
            void ForEachRefInCell(T func) {
                auto player_cell = RE::PlayerCharacter::GetSingleton()->GetParentCell();
                if (!player_cell) {
                    logger::error("Player cell is null.");
                    return;
                }
                auto& cell_runtime_data = player_cell->GetRuntimeData();
                for (auto& ref : cell_runtime_data.references) {
                    if (!ref) continue;
                    func(ref.get());
                }
            }

            RE::TESObjectREFR* TryToGetRefInCell(const FormID baseid, const Count count, float radius = 180,
                                                 unsigned int max_try = 2) {
                auto player_cell = RE::PlayerCharacter::GetSingleton()->GetParentCell();
                auto& cell_runtime_data = player_cell->GetRuntimeData();
                for (auto& ref : cell_runtime_data.references) {
                    if (!ref) continue;
                    /*if (ref->IsDisabled()) continue;
                    if (ref->IsMarkedForDeletion()) continue;
                    if (ref->IsDeleted()) continue;*/
                    if (ref->GetBaseObject()->GetFormID() == baseid && ref->extraList.GetCount() == count) {
                        logger::info("Ref found in cell: {} with id {}", ref->GetBaseObject()->GetName(),
                                     ref->GetFormID());
                        // get radius and check if ref is in radius
                        if (ref->GetFormID() < 4278190080) {
                            logger::trace("Ref is a placed reference. Continuing search.");
                            continue;
                        }
                        if (radius) {
                            auto player_pos = RE::PlayerCharacter::GetSingleton()->GetPosition();
                            auto ref_pos = ref->GetPosition();
                            if (player_pos.GetDistance(ref_pos) < radius)
                                return ref.get();
                            else
                                logger::trace("Ref is not in radius");
                        } else
                            return ref.get();
                    }
                }
                if (max_try) return TryToGetRefInCell(baseid, count, radius, --max_try);
                return nullptr;
            }

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

            const RefID TryToGetRefIDFromHandle(RE::ObjectRefHandle handle) {
                if (handle.get() && handle.get()->GetFormID()) return handle.get()->GetFormID();
                if (handle.native_handle()
                    //&& RE::TESObjectREFR::LookupByID<RE::TESObjectREFR>(handle.native_handle())
                )
                    return handle.native_handle();
                return 0;
            }

            float GetDistanceFromPlayer(RE::TESObjectREFR* ref) {
                if (!ref) {
                    logger::error("Ref is null.");
                    return 0;
                }
                auto player_pos = RE::PlayerCharacter::GetSingleton()->GetPosition();
                auto ref_pos = ref->GetPosition();
                return player_pos.GetDistance(ref_pos);
            }

            void SetObjectCount(RE::TESObjectREFR* ref, Count count) {
                if (!ref) {
                    logger::error("Ref is null.");
                    return;
                }
                int max_try = 10;
                while (ref->extraList.HasType(RE::ExtraDataType::kCount) && max_try) {
                    ref->extraList.RemoveByType(RE::ExtraDataType::kCount);
                    max_try--;
                }
                // ref->extraList.SetCount(static_cast<uint16_t>(count));
                auto xCount = new RE::ExtraCount(static_cast<int16_t>(count));
                ref->extraList.Add(xCount);
            }

            const int16_t GetObjectCount(RE::TESObjectREFR* ref) {
                if (!ref) {
                    logger::error("Ref is null.");
                    return 0;
                }
                if (ref->extraList.HasType(RE::ExtraDataType::kCount)) {
                    RE::ExtraCount* xCount = ref->extraList.GetByType<RE::ExtraCount>();
                    return xCount->count;
                }
                return 0;
            }

            RE::TESObjectREFR* DropObjectIntoTheWorld(RE::TESBoundObject* obj, Count count, bool owned = true) {
                auto player_ch = RE::PlayerCharacter::GetSingleton();
                if (!player_ch) {
                    logger::critical("Player character is null.");
                    return nullptr;
                }
                // PRINT IT
                const auto multiplier = 100.0f;
                const float qPI = 3.14159265358979323846f;
                auto orji_vec = RE::NiPoint3{multiplier, 0.f, player_ch->GetHeight()};
                Math::LinAlg::R3::rotateZ(orji_vec, qPI / 4.f - player_ch->GetAngleZ());
                auto drop_pos = player_ch->GetPosition() + orji_vec;
                auto player_cell = player_ch->GetParentCell();
                auto player_ws = player_ch->GetWorldspace();
                if (!player_cell && !player_ws) {
                    logger::critical("Player cell AND player world is null.");
                    return nullptr;
                }
                auto newPropRef = RE::TESDataHandler::GetSingleton()
                                      ->CreateReferenceAtLocation(obj, drop_pos, {0.0f, 0.0f, 0.0f}, player_cell,
                                                                  player_ws, nullptr, nullptr, {}, false, false)
                                      .get()
                                      .get();
                if (!newPropRef) {
                    logger::critical("New prop ref is null.");
                    return nullptr;
                }
                if (count > 1) SetObjectCount(newPropRef, count);
                if (owned) newPropRef->extraList.SetOwner(RE::TESForm::LookupByID<RE::TESForm>(0x07));
                return newPropRef;
            }

            // template <class... Args>
            // inline void CallFunctionOn(RE::TESForm* a_form, std::string_view formKind, std::string_view function,
            //                            Args... a_args) {
            //     const auto skyrimVM = RE::SkyrimVM::GetSingleton();
            //     auto vm = skyrimVM ? skyrimVM->impl : nullptr;
            //     if (vm) {
            //         RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
            //         auto args = RE::MakeFunctionArguments(std::forward<Args>(a_args)...);
            //         auto objectPtr = GetObjectPtr(a_form, std::string(formKind).c_str(), false);
            //         if (!objectPtr) {
            //             logger::error("Could not bind form");
            //         }
            //         vm->DispatchMethodCall(objectPtr, std::string(function).c_str(), args, callback);
            //     }
            // }

            // void ApplyHavokImpulse(RE::TESObjectREFR* target, float afX, float afY, float afZ, float afMagnitude) {
            //     CallFunctionOn(target, "ObjectReference", "ApplyHavokImpulse", afX, afY, afZ, afMagnitude);
            // }

            void SwapObjects(RE::TESObjectREFR* a_from, RE::TESBoundObject* a_to, const bool apply_havok = true) {
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
                if (!apply_havok) return;
                SKSE::GetTaskInterface()->AddTask([a_from]() {
                    a_from->Disable();
                    a_from->Enable(false);
                });
                /*a_from->Disable();
                a_from->Enable(false);*/

                /*float afX = 100;
                float afY = 100;
                float afZ = 100;
                float afMagnitude = 100;*/
                /*auto args = RE::MakeFunctionArguments(std::move(afX), std::move(afY), std::move(afZ),
                std::move(afMagnitude)); vm->DispatchMethodCall(object, "ApplyHavokImpulse", args, callback);*/
                // Looked up here (wSkeever): https:  // www.nexusmods.com/skyrimspecialedition/mods/73607
                /*SKSE::GetTaskInterface()->AddTask([a_from]() {
                    ApplyHavokImpulse(a_from, 0.f, 0.f, 10.f, 5000.f);
                });*/
                // SKSE::GetTaskInterface()->AddTask([a_from]() {
                //     // auto player_ch = RE::PlayerCharacter::GetSingleton();
                //     // player_ch->StartGrabObject();
                //     auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
                //     auto policy = vm->GetObjectHandlePolicy();
                //     auto handle = policy->GetHandleForObject(a_from->GetFormType(), a_from);
                //     RE::BSTSmartPointer<RE::BSScript::Object> object;
                //     vm->CreateObject2("ObjectReference", object);
                //     if (!object) logger::warn("Object is null");
                //     vm->BindObject(object, handle, false);
                //     auto args = RE::MakeFunctionArguments(std::move(0.f), std::move(0.f), std::move(1.f),
                //     std::move(5.f)); RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback; if
                //     (vm->DispatchMethodCall(object, "ApplyHavokImpulse", args, callback)) logger::trace("FUSRODAH");
                // });
            }

            [[nodiscard]] const bool PlayerPickUpObject(RE::TESObjectREFR* item, Count count,
                                                        const unsigned int max_try = 3) {
                logger::trace("PickUpItem");

                // std::lock_guard<std::mutex> lock(mutex);
                if (!item) {
                    logger::warn("Item is null");
                    return false;
                }

                auto actor = RE::PlayerCharacter::GetSingleton();
                auto item_bound = item->GetObjectReference();
                const auto item_count = Inventory::GetItemCount(item_bound, actor);
                logger::trace("Item count: {}", item_count);
                unsigned int i = 0;
                if (!item_bound) {
                    logger::warn("Item bound is null");
                    return false;
                }
                while (i < max_try) {
                    logger::trace("Critical: PickUpItem");
                    actor->PickUpObject(item, count, false, false);
                    logger::trace("Item picked up. Checking if it is in inventory...");
                    if (Inventory::GetItemCount(item_bound, actor) > item_count) {
                        logger::trace("Item picked up. Took {} extra tries.", i);
                        return true;
                    } else
                        logger::trace("item count: {}", Inventory::GetItemCount(item_bound, actor));
                    i++;
                }

                return false;
            }
        };

        namespace xData {

            namespace Copy {
                void CopyEnchantment(RE::ExtraEnchantment* from, RE::ExtraEnchantment* to) {
                    logger::trace("CopyEnchantment");
                    to->enchantment = from->enchantment;
                    to->charge = from->charge;
                    to->removeOnUnequip = from->removeOnUnequip;
                }

                void CopyHealth(RE::ExtraHealth* from, RE::ExtraHealth* to) {
                    logger::trace("CopyHealth");
                    to->health = from->health;
                }

                void CopyRank(RE::ExtraRank* from, RE::ExtraRank* to) {
                    logger::trace("CopyRank");
                    to->rank = from->rank;
                }

                void CopyTimeLeft(RE::ExtraTimeLeft* from, RE::ExtraTimeLeft* to) {
                    logger::trace("CopyTimeLeft");
                    to->time = from->time;
                }

                void CopyCharge(RE::ExtraCharge* from, RE::ExtraCharge* to) {
                    logger::trace("CopyCharge");
                    to->charge = from->charge;
                }

                void CopyScale(RE::ExtraScale* from, RE::ExtraScale* to) {
                    logger::trace("CopyScale");
                    to->scale = from->scale;
                }

                void CopyUniqueID(RE::ExtraUniqueID* from, RE::ExtraUniqueID* to) {
                    logger::trace("CopyUniqueID");
                    to->baseID = from->baseID;
                    to->uniqueID = from->uniqueID;
                }

                void CopyPoison(RE::ExtraPoison* from, RE::ExtraPoison* to) {
                    logger::trace("CopyPoison");
                    to->poison = from->poison;
                    to->count = from->count;
                }

                void CopyObjectHealth(RE::ExtraObjectHealth* from, RE::ExtraObjectHealth* to) {
                    logger::trace("CopyObjectHealth");
                    to->health = from->health;
                }

                void CopyLight(RE::ExtraLight* from, RE::ExtraLight* to) {
                    logger::trace("CopyLight");
                    to->lightData = from->lightData;
                }

                void CopyRadius(RE::ExtraRadius* from, RE::ExtraRadius* to) {
                    logger::trace("CopyRadius");
                    to->radius = from->radius;
                }

                void CopyHorse(RE::ExtraHorse* from, RE::ExtraHorse* to) {
                    logger::trace("CopyHorse");
                    to->horseRef = from->horseRef;
                }

                void CopyHotkey(RE::ExtraHotkey* from, RE::ExtraHotkey* to) {
                    logger::trace("CopyHotkey");
                    to->hotkey = from->hotkey;
                }

                void CopyTextDisplayData(RE::ExtraTextDisplayData* from, RE::ExtraTextDisplayData* to) {
                    to->displayName = from->displayName;
                    to->displayNameText = from->displayNameText;
                    to->ownerQuest = from->ownerQuest;
                    to->ownerInstance = from->ownerInstance;
                    to->temperFactor = from->temperFactor;
                    to->customNameLength = from->customNameLength;
                }

                void CopySoul(RE::ExtraSoul* from, RE::ExtraSoul* to) {
                    logger::trace("CopySoul");
                    to->soul = from->soul;
                }

                void CopyOwnership(RE::ExtraOwnership* from, RE::ExtraOwnership* to) {
                    logger::trace("CopyOwnership");
                    to->owner = from->owner;
                }
            };

            template <typename T>
            void CopyExtraData(T* from, T* to) {
                if (!from || !to) return;
                switch (T->EXTRADATATYPE) {
                    case RE::ExtraDataType::kEnchantment:
                        CopyEnchantment(from, to);
                        break;
                    case RE::ExtraDataType::kHealth:
                        CopyHealth(from, to);
                        break;
                    case RE::ExtraDataType::kRank:
                        CopyRank(from, to);
                        break;
                    case RE::ExtraDataType::kTimeLeft:
                        CopyTimeLeft(from, to);
                        break;
                    case RE::ExtraDataType::kCharge:
                        CopyCharge(from, to);
                        break;
                    case RE::ExtraDataType::kScale:
                        CopyScale(from, to);
                        break;
                    case RE::ExtraDataType::kUniqueID:
                        CopyUniqueID(from, to);
                        break;
                    case RE::ExtraDataType::kPoison:
                        CopyPoison(from, to);
                        break;
                    case RE::ExtraDataType::kObjectHealth:
                        CopyObjectHealth(from, to);
                        break;
                    case RE::ExtraDataType::kLight:
                        CopyLight(from, to);
                        break;
                    case RE::ExtraDataType::kRadius:
                        CopyRadius(from, to);
                        break;
                    case RE::ExtraDataType::kHorse:
                        CopyHorse(from, to);
                        break;
                    case RE::ExtraDataType::kHotkey:
                        CopyHotkey(from, to);
                        break;
                    case RE::ExtraDataType::kTextDisplayData:
                        CopyTextDisplayData(from, to);
                        break;
                    case RE::ExtraDataType::kSoul:
                        CopySoul(from, to);
                        break;
                    case RE::ExtraDataType::kOwnership:
                        CopyOwnership(from, to);
                        break;
                    default:
                        logger::warn("ExtraData type not found");
                        break;
                };
            }

            [[nodiscard]] const bool UpdateExtras(RE::ExtraDataList* copy_from, RE::ExtraDataList* copy_to) {
                logger::trace("UpdateExtras");
                if (!copy_from || !copy_to) return false;
                // Enchantment
                if (copy_from->HasType(RE::ExtraDataType::kEnchantment)) {
                    logger::trace("Enchantment found");
                    auto enchantment =
                        static_cast<RE::ExtraEnchantment*>(copy_from->GetByType(RE::ExtraDataType::kEnchantment));
                    if (enchantment) {
                        RE::ExtraEnchantment* enchantment_fake = RE::BSExtraData::Create<RE::ExtraEnchantment>();
                        // log the associated actor value
                        logger::trace("Associated actor value: {}", enchantment->enchantment->GetAssociatedSkill());
                        Copy::CopyEnchantment(enchantment, enchantment_fake);
                        copy_to->Add(enchantment_fake);
                    } else
                        return false;
                }
                // Health
                if (copy_from->HasType(RE::ExtraDataType::kHealth)) {
                    logger::trace("Health found");
                    auto health = static_cast<RE::ExtraHealth*>(copy_from->GetByType(RE::ExtraDataType::kHealth));
                    if (health) {
                        RE::ExtraHealth* health_fake = RE::BSExtraData::Create<RE::ExtraHealth>();
                        Copy::CopyHealth(health, health_fake);
                        copy_to->Add(health_fake);
                    } else
                        return false;
                }
                // Rank
                if (copy_from->HasType(RE::ExtraDataType::kRank)) {
                    logger::trace("Rank found");
                    auto rank = static_cast<RE::ExtraRank*>(copy_from->GetByType(RE::ExtraDataType::kRank));
                    if (rank) {
                        RE::ExtraRank* rank_fake = RE::BSExtraData::Create<RE::ExtraRank>();
                        Copy::CopyRank(rank, rank_fake);
                        copy_to->Add(rank_fake);
                    } else
                        return false;
                }
                // TimeLeft
                if (copy_from->HasType(RE::ExtraDataType::kTimeLeft)) {
                    logger::trace("TimeLeft found");
                    auto timeleft = static_cast<RE::ExtraTimeLeft*>(copy_from->GetByType(RE::ExtraDataType::kTimeLeft));
                    if (timeleft) {
                        RE::ExtraTimeLeft* timeleft_fake = RE::BSExtraData::Create<RE::ExtraTimeLeft>();
                        Copy::CopyTimeLeft(timeleft, timeleft_fake);
                        copy_to->Add(timeleft_fake);
                    } else
                        return false;
                }
                // Charge
                if (copy_from->HasType(RE::ExtraDataType::kCharge)) {
                    logger::trace("Charge found");
                    auto charge = static_cast<RE::ExtraCharge*>(copy_from->GetByType(RE::ExtraDataType::kCharge));
                    if (charge) {
                        RE::ExtraCharge* charge_fake = RE::BSExtraData::Create<RE::ExtraCharge>();
                        Copy::CopyCharge(charge, charge_fake);
                        copy_to->Add(charge_fake);
                    } else
                        return false;
                }
                // Scale
                if (copy_from->HasType(RE::ExtraDataType::kScale)) {
                    logger::trace("Scale found");
                    auto scale = static_cast<RE::ExtraScale*>(copy_from->GetByType(RE::ExtraDataType::kScale));
                    if (scale) {
                        RE::ExtraScale* scale_fake = RE::BSExtraData::Create<RE::ExtraScale>();
                        Copy::CopyScale(scale, scale_fake);
                        copy_to->Add(scale_fake);
                    } else
                        return false;
                }
                // UniqueID
                if (copy_from->HasType(RE::ExtraDataType::kUniqueID)) {
                    logger::trace("UniqueID found");
                    auto uniqueid = static_cast<RE::ExtraUniqueID*>(copy_from->GetByType(RE::ExtraDataType::kUniqueID));
                    if (uniqueid) {
                        RE::ExtraUniqueID* uniqueid_fake = RE::BSExtraData::Create<RE::ExtraUniqueID>();
                        Copy::CopyUniqueID(uniqueid, uniqueid_fake);
                        copy_to->Add(uniqueid_fake);
                    } else
                        return false;
                }
                // Poison
                if (copy_from->HasType(RE::ExtraDataType::kPoison)) {
                    logger::trace("Poison found");
                    auto poison = static_cast<RE::ExtraPoison*>(copy_from->GetByType(RE::ExtraDataType::kPoison));
                    if (poison) {
                        RE::ExtraPoison* poison_fake = RE::BSExtraData::Create<RE::ExtraPoison>();
                        Copy::CopyPoison(poison, poison_fake);
                        copy_to->Add(poison_fake);
                    } else
                        return false;
                }
                // ObjectHealth
                if (copy_from->HasType(RE::ExtraDataType::kObjectHealth)) {
                    logger::trace("ObjectHealth found");
                    auto objhealth =
                        static_cast<RE::ExtraObjectHealth*>(copy_from->GetByType(RE::ExtraDataType::kObjectHealth));
                    if (objhealth) {
                        RE::ExtraObjectHealth* objhealth_fake = RE::BSExtraData::Create<RE::ExtraObjectHealth>();
                        Copy::CopyObjectHealth(objhealth, objhealth_fake);
                        copy_to->Add(objhealth_fake);
                    } else
                        return false;
                }
                // Light
                if (copy_from->HasType(RE::ExtraDataType::kLight)) {
                    logger::trace("Light found");
                    auto light = static_cast<RE::ExtraLight*>(copy_from->GetByType(RE::ExtraDataType::kLight));
                    if (light) {
                        RE::ExtraLight* light_fake = RE::BSExtraData::Create<RE::ExtraLight>();
                        Copy::CopyLight(light, light_fake);
                        copy_to->Add(light_fake);
                    } else
                        return false;
                }
                // Radius
                if (copy_from->HasType(RE::ExtraDataType::kRadius)) {
                    logger::trace("Radius found");
                    auto radius = static_cast<RE::ExtraRadius*>(copy_from->GetByType(RE::ExtraDataType::kRadius));
                    if (radius) {
                        RE::ExtraRadius* radius_fake = RE::BSExtraData::Create<RE::ExtraRadius>();
                        Copy::CopyRadius(radius, radius_fake);
                        copy_to->Add(radius_fake);
                    } else
                        return false;
                }
                // Sound (Disabled)
                /*if (copy_from->HasType(RE::ExtraDataType::kSound)) {
                    logger::trace("Sound found");
                    auto sound = static_cast<RE::ExtraSound*>(copy_from->GetByType(RE::ExtraDataType::kSound));
                    if (sound) {
                        RE::ExtraSound* sound_fake = RE::BSExtraData::Create<RE::ExtraSound>();
                        sound_fake->handle = sound->handle;
                        copy_to->Add(sound_fake);
                    } else
                        RaiseMngrErr("Failed to get radius from copy_from");
                }*/
                // LinkedRef (Disabled)
                /*if (copy_from->HasType(RE::ExtraDataType::kLinkedRef)) {
                    logger::trace("LinkedRef found");
                    auto linkedref =
                        static_cast<RE::ExtraLinkedRef*>(copy_from->GetByType(RE::ExtraDataType::kLinkedRef));
                    if (linkedref) {
                        RE::ExtraLinkedRef* linkedref_fake = RE::BSExtraData::Create<RE::ExtraLinkedRef>();
                        linkedref_fake->linkedRefs = linkedref->linkedRefs;
                        copy_to->Add(linkedref_fake);
                    } else
                        RaiseMngrErr("Failed to get linkedref from copy_from");
                }*/
                // Horse
                if (copy_from->HasType(RE::ExtraDataType::kHorse)) {
                    logger::trace("Horse found");
                    auto horse = static_cast<RE::ExtraHorse*>(copy_from->GetByType(RE::ExtraDataType::kHorse));
                    if (horse) {
                        RE::ExtraHorse* horse_fake = RE::BSExtraData::Create<RE::ExtraHorse>();
                        Copy::CopyHorse(horse, horse_fake);
                        copy_to->Add(horse_fake);
                    } else
                        return false;
                }
                // Hotkey
                if (copy_from->HasType(RE::ExtraDataType::kHotkey)) {
                    logger::trace("Hotkey found");
                    auto hotkey = static_cast<RE::ExtraHotkey*>(copy_from->GetByType(RE::ExtraDataType::kHotkey));
                    if (hotkey) {
                        RE::ExtraHotkey* hotkey_fake = RE::BSExtraData::Create<RE::ExtraHotkey>();
                        Copy::CopyHotkey(hotkey, hotkey_fake);
                        copy_to->Add(hotkey_fake);
                    } else
                        return false;
                }
                // Weapon Attack Sound (Disabled)
                /*if (copy_from->HasType(RE::ExtraDataType::kWeaponAttackSound)) {
                    logger::trace("WeaponAttackSound found");
                    auto weaponattacksound = static_cast<RE::ExtraWeaponAttackSound*>(
                        copy_from->GetByType(RE::ExtraDataType::kWeaponAttackSound));
                    if (weaponattacksound) {
                        RE::ExtraWeaponAttackSound* weaponattacksound_fake =
                            RE::BSExtraData::Create<RE::ExtraWeaponAttackSound>();
                        weaponattacksound_fake->handle = weaponattacksound->handle;
                        copy_to->Add(weaponattacksound_fake);
                    } else
                        RaiseMngrErr("Failed to get weaponattacksound from copy_from");
                }*/
                // Activate Ref (Disabled)
                /*if (copy_from->HasType(RE::ExtraDataType::kActivateRef)) {
                    logger::trace("ActivateRef found");
                    auto activateref =
                        static_cast<RE::ExtraActivateRef*>(copy_from->GetByType(RE::ExtraDataType::kActivateRef));
                    if (activateref) {
                        RE::ExtraActivateRef* activateref_fake = RE::BSExtraData::Create<RE::ExtraActivateRef>();
                        activateref_fake->parents = activateref->parents;
                        activateref_fake->activateFlags = activateref->activateFlags;
                    } else
                        RaiseMngrErr("Failed to get activateref from copy_from");
                }*/
                // TextDisplayData
                if (copy_from->HasType(RE::ExtraDataType::kTextDisplayData)) {
                    logger::trace("TextDisplayData found");
                    auto textdisplaydata = static_cast<RE::ExtraTextDisplayData*>(
                        copy_from->GetByType(RE::ExtraDataType::kTextDisplayData));
                    if (textdisplaydata) {
                        RE::ExtraTextDisplayData* textdisplaydata_fake =
                            RE::BSExtraData::Create<RE::ExtraTextDisplayData>();
                        Copy::CopyTextDisplayData(textdisplaydata, textdisplaydata_fake);
                        copy_to->Add(textdisplaydata_fake);
                    } else
                        return false;
                }
                // Soul
                if (copy_from->HasType(RE::ExtraDataType::kSoul)) {
                    logger::trace("Soul found");
                    auto soul = static_cast<RE::ExtraSoul*>(copy_from->GetByType(RE::ExtraDataType::kSoul));
                    if (soul) {
                        RE::ExtraSoul* soul_fake = RE::BSExtraData::Create<RE::ExtraSoul>();
                        Copy::CopySoul(soul, soul_fake);
                        copy_to->Add(soul_fake);
                    } else
                        return false;
                }
                // Flags (OK)
                if (copy_from->HasType(RE::ExtraDataType::kFlags)) {
                    logger::trace("Flags found");
                    auto flags = static_cast<RE::ExtraFlags*>(copy_from->GetByType(RE::ExtraDataType::kFlags));
                    if (flags) {
                        SKSE::stl::enumeration<RE::ExtraFlags::Flag, std::uint32_t> flags_fake;
                        if (flags->flags.all(RE::ExtraFlags::Flag::kBlockActivate))
                            flags_fake.set(RE::ExtraFlags::Flag::kBlockActivate);
                        if (flags->flags.all(RE::ExtraFlags::Flag::kBlockPlayerActivate))
                            flags_fake.set(RE::ExtraFlags::Flag::kBlockPlayerActivate);
                        if (flags->flags.all(RE::ExtraFlags::Flag::kBlockLoadEvents))
                            flags_fake.set(RE::ExtraFlags::Flag::kBlockLoadEvents);
                        if (flags->flags.all(RE::ExtraFlags::Flag::kBlockActivateText))
                            flags_fake.set(RE::ExtraFlags::Flag::kBlockActivateText);
                        if (flags->flags.all(RE::ExtraFlags::Flag::kPlayerHasTaken))
                            flags_fake.set(RE::ExtraFlags::Flag::kPlayerHasTaken);
                        // RE::ExtraFlags* flags_fake = RE::BSExtraData::Create<RE::ExtraFlags>();
                        // flags_fake->flags = flags->flags;
                        // copy_to->Add(flags_fake);
                    } else
                        return false;
                }
                // Lock (Disabled)
                /*if (copy_from->HasType(RE::ExtraDataType::kLock)) {
                    logger::trace("Lock found");
                    auto lock = static_cast<RE::ExtraLock*>(copy_from->GetByType(RE::ExtraDataType::kLock));
                    if (lock) {
                        RE::ExtraLock* lock_fake = RE::BSExtraData::Create<RE::ExtraLock>();
                        lock_fake->lock = lock->lock;
                        copy_to->Add(lock_fake);
                    } else
                        RaiseMngrErr("Failed to get lock from copy_from");
                }*/
                // Teleport (Disabled)
                /*if (copy_from->HasType(RE::ExtraDataType::kTeleport)) {
                    logger::trace("Teleport found");
                    auto teleport =
                        static_cast<RE::ExtraTeleport*>(copy_from->GetByType(RE::ExtraDataType::kTeleport));
                    if (teleport) {
                        RE::ExtraTeleport* teleport_fake = RE::BSExtraData::Create<RE::ExtraTeleport>();
                        teleport_fake->teleportData = teleport->teleportData;
                        copy_to->Add(teleport_fake);
                    } else
                        RaiseMngrErr("Failed to get teleport from copy_from");
                }*/
                // LockList (Disabled)
                /*if (copy_from->HasType(RE::ExtraDataType::kLockList)) {
                    logger::trace("LockList found");
                    auto locklist =
                        static_cast<RE::ExtraLockList*>(copy_from->GetByType(RE::ExtraDataType::kLockList));
                    if (locklist) {
                        RE::ExtraLockList* locklist_fake = RE::BSExtraData::Create<RE::ExtraLockList>();
                        locklist_fake->list = locklist->list;
                        copy_to->Add(locklist_fake);
                    } else
                        RaiseMngrErr("Failed to get locklist from copy_from");
                }*/
                // OutfitItem (Disabled)
                /*if (copy_from->HasType(RE::ExtraDataType::kOutfitItem)) {
                    logger::trace("OutfitItem found");
                    auto outfititem =
                        static_cast<RE::ExtraOutfitItem*>(copy_from->GetByType(RE::ExtraDataType::kOutfitItem));
                    if (outfititem) {
                        RE::ExtraOutfitItem* outfititem_fake = RE::BSExtraData::Create<RE::ExtraOutfitItem>();
                        outfititem_fake->id = outfititem->id;
                        copy_to->Add(outfititem_fake);
                    } else
                        RaiseMngrErr("Failed to get outfititem from copy_from");
                }*/
                // CannotWear (Disabled)
                /*if (copy_from->HasType(RE::ExtraDataType::kCannotWear)) {
                    logger::trace("CannotWear found");
                    auto cannotwear =
                        static_cast<RE::ExtraCannotWear*>(copy_from->GetByType(RE::ExtraDataType::kCannotWear));
                    if (cannotwear) {
                        RE::ExtraCannotWear* cannotwear_fake = RE::BSExtraData::Create<RE::ExtraCannotWear>();
                        copy_to->Add(cannotwear_fake);
                    } else
                        RaiseMngrErr("Failed to get cannotwear from copy_from");
                }*/
                // Ownership (OK)
                if (copy_from->HasType(RE::ExtraDataType::kOwnership)) {
                    logger::trace("Ownership found");
                    auto ownership =
                        static_cast<RE::ExtraOwnership*>(copy_from->GetByType(RE::ExtraDataType::kOwnership));
                    if (ownership) {
                        logger::trace("length of fake extradatalist: {}", copy_to->GetCount());
                        RE::ExtraOwnership* ownership_fake = RE::BSExtraData::Create<RE::ExtraOwnership>();
                        Copy::CopyOwnership(ownership, ownership_fake);
                        copy_to->Add(ownership_fake);
                        logger::trace("length of fake extradatalist: {}", copy_to->GetCount());
                    } else
                        return false;
                }

                return true;
            }

            void PrintObjectExtraData(RE::TESObjectREFR* ref) {
                if (!ref) {
                    logger::error("Ref is null.");
                    return;
                }
                logger::trace("printing ExtraDataList");
                for (int i = 0; i < 191; i++) {
                    if (ref->extraList.HasType(static_cast<RE::ExtraDataType>(i))) {
                        logger::trace("ExtraDataList type: {}", i);
                    }
                }
            }

        };

        // SkyrimThiago <3
        // https://github.com/Thiago099/DPF-Dynamic-Persistent-Forms
        namespace DynamicForm {

            const bool IsDynamicFormID(const FormID a_formID) { return a_formID >= 0xFF000000; }

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

        };

    };

    namespace FunctionsPapyrus {
        //   using VM = RE::BSScript::Internal::VirtualMachine;
        // using ObjectPtr = RE::BSTSmartPointer<RE::BSScript::Object>;

        //   // https:// github.com/clayne/GTS_Plugin/blob/master/src/utils/papyrusUtils.hpp#L12
        //   inline RE::VMHandle GetHandle(RE::TESForm* a_form) {
        //       auto vm = VM::GetSingleton();
        //       auto policy = vm->GetObjectHandlePolicy();
        //       return policy->GetHandleForObject(a_form->GetFormType(), a_form);
        //   }

        //   // https://github.com/clayne/GTS_Plugin/blob/master/src/utils/papyrusUtils.hpp#L19
        //   inline ObjectPtr GetObjectPtr(RE::TESForm* a_form, const char* a_class, bool a_create) {
        //       auto vm = VM::GetSingleton();
        //       auto handle = GetHandle(a_form);

        //       ObjectPtr object = nullptr;
        //       bool found = vm->FindBoundObject(handle, a_class, object);
        //       if (!found && a_create) {
        //           vm->CreateObject2(a_class, object);
        //           vm->BindObject(object, handle, false);
        //       }

        //       return object;
        //   }

        //   // https://github.com/clayne/GTS_Plugin/blob/master/src/utils/papyrusUtils.hpp#L35
        //   template <class... Args>
        //   inline void CallFunctionOn(RE::TESForm* a_form, std::string_view formKind, std::string_view function,
        //                              Args... a_args) {
        //       const auto skyrimVM = RE::SkyrimVM::GetSingleton();
        //       auto vm = skyrimVM ? skyrimVM->impl : nullptr;
        //       if (vm) {
        //           RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
        //           auto args = RE::MakeFunctionArguments(std::forward<Args>(a_args)...);
        //           auto objectPtr = GetObjectPtr(a_form, std::string(formKind).c_str(), false);
        //           if (!objectPtr) {
        //               logger::error("Could not bind form");
        //           }
        //           vm->DispatchMethodCall(objectPtr, std::string(function).c_str(), args, callback);
        //       }
        //   }

        //   typedef void (*_ApplyHavokImpulse)(RE::BSScript::VMState* registry, uint32_t stackID, RE::TESObjectREFR*
        //   target, float afX,
        //                                      float afY, float afZ, float magnitude);
        //   const _ApplyHavokImpulse ApplyHavokImpulse = (_ApplyHavokImpulse)0x00908260;
        //   RE::SkyrimVM*& g_skyrimVM = *(RE::SkyrimVM**)0x012E568C;
    };

    namespace FunctionsJSON {

        FormID GetFormEditorID(const rapidjson::Value& section, const char* memberName) {
            if (!section.HasMember(memberName)) {
                logger::error("Member {} not found", memberName);
                return 0;
            }
            if (section[memberName].IsString()) {
                const std::string formEditorId = section[memberName].GetString();
                if (Utilities::Functions::isValidHexWithLength7or8(formEditorId.c_str())) {
                    int form_id_;
                    std::stringstream ss;
                    ss << std::hex << formEditorId;
                    ss >> form_id_;
                    auto temp_form = FunctionsSkyrim::GetFormByID(form_id_, "");
                    if (temp_form)
                        return temp_form->GetFormID();
                    else {
                        logger::error("Formid is null for editorid {}", formEditorId);
                        return 0;
                    }
                }
                if (formEditorId.empty())
                    return 0;
                else if (!IsPo3Installed()) {
                    logger::error("Po3 is not installed.");
                    MsgBoxesNotifs::Windows::Po3ErrMsg();
                    return 0;
                }
                auto temp_form = FunctionsSkyrim::GetFormByID(0, formEditorId);
                if (temp_form)
                    return temp_form->GetFormID();
                else {
                    logger::error("Formid is null for editorid {}", formEditorId);
                    return 0;
                }
            } else if (section[memberName].IsInt())
                return section[memberName].GetInt();
            return 0;
        }
    }

    namespace FunctionsYAML {}

    namespace Types {

        struct FormFormID {
            FormID form_id1;
            FormID form_id2;

            bool operator<(const FormFormID& other) const {
                // Compare form_id1 first
                if (form_id1 < other.form_id1) {
                    return true;
                }
                // If form_id1 is equal, compare form_id2
                if (form_id1 == other.form_id1 && form_id2 < other.form_id2) {
                    return true;
                }
                // If both form_id1 and form_id2 are equal or if form_id1 is greater, return false
                return false;
            }
        };

        struct FormEditorID {
            FormID form_id = 0;
            std::string editor_id = "";

            bool operator<(const FormEditorID& other) const {
                // Compare form_id first
                if (form_id < other.form_id) {
                    return true;
                }
                // If form_id is equal, compare editor_id
                if (form_id == other.form_id && editor_id < other.editor_id) {
                    return true;
                }
                // If both form_id and editor_id are equal or if form_id is greater, return false
                return false;
            }
        };

        struct FormEditorIDX : FormEditorID {
            bool is_fake = false;
            bool is_decayed = false;
            bool is_transforming = false;
            bool crafting_allowed = false;

            bool operator==(const FormEditorIDX& other) const { return form_id == other.form_id; }
        };

        // using EditorID = std::string;
        using NameID = std::string;
        using Duration = float;
        using DurationMGEFF = std::uint32_t;
        using StageNo = unsigned int;
        using StageName = std::string;

        struct StageEffect {
            FormID beffect;          // base effect
            float magnitude;         // in effectitem
            std::uint32_t duration;  // in effectitem (not Duration, this is in seconds)

            StageEffect() : beffect(0), magnitude(0), duration(0) {}
            StageEffect(FormID be, float mag, DurationMGEFF dur) : beffect(be), magnitude(mag), duration(dur) {}

            [[nodiscard]] const bool IsNull() const { return beffect == 0; }
            [[nodiscard]] const bool HasMagnitude() const { return magnitude != 0; }
            [[nodiscard]] const bool HasDuration() const { return duration != 0; }
        };

        struct Stage {
            FormID formid = 0;  // with which item is it represented
            Duration duration;  // duration of the stage
            StageNo no;         // used for sorting when multiple stages are present
            StageName name;     // name of the stage
            std::vector<StageEffect> mgeffect;

            bool crafting_allowed;

            Stage(){};
            Stage(FormID f, Duration d, StageNo s, StageName n, bool ca, std::vector<StageEffect> e)
                : formid(f), duration(d), no(s), name(n), crafting_allowed(ca), mgeffect(e) {
                if (!formid)
                    logger::critical("FormID is null");
                else
                    logger::info("Stage: FormID {}, Duration {}, StageNo {}, Name {}", formid, duration, no, name);
                if (e.empty()) mgeffect.clear();
            }

            bool operator<(const Stage& other) const {
                if (formid < other.formid) return true;
                if (other.formid < formid) return false;
                return no < other.no;
            }

            bool operator==(const Stage& other) const {
                return no == other.no && formid == other.formid && duration == other.duration;
            }

            RE::TESBoundObject* GetBound() const { return FunctionsSkyrim::GetFormByID<RE::TESBoundObject>(formid); };

            [[nodiscard]] const bool CheckIntegrity() const {
                if (!formid || !GetBound()) {
                    logger::error("FormID or bound is null");
                    return false;
                }
                if (duration <= 0) {
                    logger::error("Duration is 0 or negative");
                    return false;
                }
                return true;
            }

            const char* GetExtraText() const { return GetBound()->GetName(); }
        };

        using Stages = std::vector<Stage>;
        using StageDict = std::map<StageNo, Stage>;

        /*struct StageDict {
            StageDict() = default;
            StageDict(std::vector<StageNo> nos, std::vector<Stage> stages) {
                for (size_t i = 0; i < nos.size(); i++) {
                    stage_map[nos[i]] = stages[i];
                }
            }

            [[nodiscard]] const bool CheckIntegrity() const {
                for (const auto& [no, stage] : stage_map) {
                    if (!stage.CheckIntegrity()) return false;
                }
                return true;
            }

            [[nodiscard]] const bool IsEmpty() const { return stage_map.empty(); }

            [[nodiscard]] const bool contains(const StageNo no) const { return stage_map.contains(no); }

            [[nodiscard]] const Stage& GetStage(const StageNo no) const {
                if (contains(no)) return stage_map.at(no);
                logger::critical("Stage {} not found", no);
                return stage_map.at(0);
            }

        private:
            std::map<StageNo, Stage> stage_map;
        };*/

        struct StageInstancePlain {
            float start_time;
            StageNo no;
            Count count;

            float _elapsed;
            float _delay_start;
            float _delay_mag;
            FormID _delay_formid;

            bool is_fake = false;
            bool is_decayed = false;
            bool is_transforming = false;

            bool is_faved = false;
            bool is_equipped = false;

            FormID form_id = 0;  // for fake stuff
        };

        struct StageInstance {
            float start_time;  // start time of the stage
            StageNo no;
            Count count;
            // RefID location;  // RefID of the container where the fake food is stored or the real food itself when it
            // is
            //  out in the world
            FormEditorIDX xtra;

            // StageInstance() : start_time(0), no(0), count(0), location(0) {}
            StageInstance(const float st, const StageNo n, const Count c
                          //, RefID l
                          //,std::string ei
                          )
                : start_time(st),
                  no(n),
                  count(c)
            //, location(l)
            //,editorid(ei)
            {
                _elapsed = 0;
                _delay_start = start_time;
                _delay_mag = 1;
                _delay_formid = 0;
            }

            // StageInstance(const StageInstancePlain& plain)
            //     : start_time(plain.start_time),
            //       no(plain.no),
            //       count(plain.count),
            //       _elapsed(plain._elapsed),
            //       _delay_start(plain._delay_start),
            //       _delay_mag(plain._delay_mag),
            //       _delay_formid(plain._delay_formid) {
            //
            //     xtra.is_fake = plain.is_fake;
            //     xtra.is_decayed = plain.is_decayed;
            // }

            // define ==
            //  assumes that they are in the same inventory
            [[nodiscard]] bool operator==(const StageInstance& other) const {
                return no == other.no && count == other.count &&
                       // location == other.location &&
                       start_time == other.start_time && _elapsed == other._elapsed && xtra == other.xtra;
            }

            //         [[maybe_unused]] bool SameExceptCount(const StageInstance& other) const {
            //             return no == other.no && location == other.location &&
            //                    start_time == other.start_time && _elapsed == other._elapsed;
            //}

            // times are very close (abs diff less than 0.015h = 0.9min)
            // assumes that they are in the same inventory
            [[nodiscard]] bool AlmostSameExceptCount(StageInstance& other, const float curr_time) const {
                // bcs they are in the same inventory they will have same delay magnitude
                // delay starts might be different but if the elapsed times are close enough, we don't care
                return no == other.no &&
                       // location == other.location &&
                       std::abs(start_time - other.start_time) < 0.015 &&
                       std::abs(GetElapsed(curr_time) - other.GetElapsed(curr_time)) < 0.015 && xtra == other.xtra;
            }

            StageInstance& operator=(const StageInstance& other) {
                if (this != &other) {
                    start_time = other.start_time;
                    no = other.no;
                    count = other.count;
                    // location = other.location;

                    xtra = other.xtra;

                    _elapsed = other._elapsed;
                    _delay_start = other._delay_start;
                    _delay_mag = other._delay_mag;
                    _delay_formid = other._delay_formid;
                }
                return *this;
            }

            RE::TESBoundObject* GetBound() const {
                return FunctionsSkyrim::GetFormByID<RE::TESBoundObject>(xtra.form_id);
            };

            const float GetElapsed(const float curr_time) const {
                if (_delay_mag == 0) return _elapsed;
                return (curr_time - _delay_start) * GetDelaySlope() + _elapsed;
            }

            const float GetDelaySlope() const {
                const auto delay_magnitude = std::min(std::max(-1000.f, _delay_mag), 1000.f);
                // return 1 - delay_magnitude;
                // if (std::abs(delay_magnitude) < 0.0001f) {
                //     // If the delay slope is too close to 0, return a small non-zero value
                //     return delay_magnitude < 0 ? -0.0001f : 0.0001f;
                // }

                return delay_magnitude;
            }

            void SetNewStart(const float curr_time, const float overshot) {
                // overshot: by how much is the schwelle already ueberschritten
                start_time = curr_time - overshot / (GetDelaySlope() + std::numeric_limits<float>::epsilon());
                _delay_start = start_time;
                _elapsed = 0;
            }

            void SetDelay(const float time, const float delay, const FormID formid) {
                // yeni steigungla yeni ausgangspunkt yapiyoruz
                // call only from UpdateTimeModulationInInventory
                if (xtra.is_transforming) return;
                if (_delay_mag == delay && _delay_formid == formid) return;

                _elapsed = GetElapsed(time);
                _delay_start = time;
                _delay_mag = delay;
                _delay_formid = formid;
            }

            void SetTransform(const float time, const FormID formid) {
                if (xtra.is_transforming) {
                    if (_delay_formid != formid) {
                        RemoveTransform(time);
                        return SetTransform(time, formid);
                    } else
                        return;
                }
                SetDelay(time, 1, formid);
                xtra.is_transforming = true;
            }

            const float GetTransformElapsed(const float curr_time) const { return GetElapsed(curr_time) - _elapsed; }

            void RemoveTransform(const float curr_time) {
                if (!xtra.is_transforming) return;
                xtra.is_transforming = false;
                _delay_start = curr_time;
                _delay_mag = 1;
                _delay_formid = 0;
            }

            // use only for WO (e.g. HandleDrop)
            void RemoveTimeMod(const float time) {
                RemoveTransform(time);
                SetDelay(time, 1, 0);
            }

            const float GetDelayMagnitude() const { return GetDelaySlope(); }

            const FormID GetDelayerFormID() const { return _delay_formid; }

            const float GetHittingTime(float schranke) const {
                // _elapsed + dt*_delay_mag = schranke
                return _delay_start + (schranke - _elapsed) / (GetDelaySlope() + std::numeric_limits<float>::epsilon());
            }

            StageInstancePlain GetPlain() const {
                StageInstancePlain plain;
                plain.start_time = start_time;
                plain.no = no;
                plain.count = count;

                plain.is_fake = xtra.is_fake;
                plain.is_decayed = xtra.is_decayed;
                plain.is_transforming = xtra.is_transforming;

                plain._elapsed = _elapsed;
                plain._delay_start = _delay_start;
                plain._delay_mag = _delay_mag;
                plain._delay_formid = _delay_formid;

                if (xtra.is_fake) plain.form_id = xtra.form_id;

                return plain;
            }

            void SetDelay(const StageInstancePlain& plain) {
                _elapsed = plain._elapsed;
                _delay_start = plain._delay_start;
                _delay_mag = plain._delay_mag;
                _delay_formid = plain._delay_formid;
            }

        private:
            float _elapsed;        // y coord of the ausgangspunkt/elapsed time since the stage started
            float _delay_start;    // x coord of the ausgangspunkt
            float _delay_mag;      // slope
            FormID _delay_formid;  // formid of the time modulator
        };

        struct StageUpdate {
            const Stage* oldstage;
            const Stage* newstage;
            Count count = 0;
            Duration update_time = 0;
            bool new_is_fake = false;

            StageUpdate(const Stage* old, const Stage* new_, Count c, Duration u_t, bool fake)
                : oldstage(old), newstage(new_), count(c), update_time(u_t), new_is_fake(fake) {}
        };

        using SourceData = std::map<RefID, std::vector<StageInstance>>;
        using SaveDataLHS = std::pair<FormEditorID, RefID>;
        using SaveDataRHS = std::vector<StageInstancePlain>;

        struct DFSaveData {
            FormID dyn_formid = 0;
            std::pair<bool, uint32_t> custom_id = {false, 0};
            float acteff_elapsed = -1.f;
        };
        using DFSaveDataLHS = std::pair<FormID, std::string>;
        using DFSaveDataRHS = std::vector<DFSaveData>;

    };

    // https://github.com/ozooma10/OSLAroused-SKSE/blob/master/src/Utilities/Ticker.h
    class Ticker {
    public:
        Ticker(std::function<void()> onTick, std::chrono::milliseconds interval)
            : m_OnTick(onTick), m_Interval(interval), m_Running(false), m_ThreadActive(false) {}

        void Start() {
            if (m_Running) {
                return;
            }
            m_Running = true;
            logger::trace("Start Called with thread active state of: {}", m_ThreadActive);
            if (!m_ThreadActive) {
                std::thread tickerThread(&Ticker::RunLoop, this);
                tickerThread.detach();
            }
        }

        void Stop() { m_Running = false; }

        void UpdateInterval(std::chrono::milliseconds newInterval) {
            m_IntervalMutex.lock();
            m_Interval = newInterval;
            m_IntervalMutex.unlock();
        }

    private:
        void RunLoop() {
            m_ThreadActive = true;
            while (m_Running) {
                std::thread runnerThread(m_OnTick);
                runnerThread.detach();

                m_IntervalMutex.lock();
                std::chrono::milliseconds interval;
                if (m_Interval >= std::chrono::milliseconds(3000)) {
                    interval = m_Interval;
                } else {
                    interval = std::chrono::milliseconds(3000);
                }
                m_IntervalMutex.unlock();
                std::this_thread::sleep_for(interval);
            }
            m_ThreadActive = false;
        }

        std::function<void()> m_OnTick;
        std::chrono::milliseconds m_Interval;

        std::atomic<bool> m_ThreadActive;
        std::atomic<bool> m_Running;
        std::mutex m_IntervalMutex;
    };

    bool read_string(SKSE::SerializationInterface* a_intfc, std::string& a_str) {
        std::vector<std::pair<int, bool>> encodedStr;
        std::size_t size;
        if (!a_intfc->ReadRecordData(size)) {
            return false;
        }
        for (std::size_t i = 0; i < size; i++) {
            std::pair<int, bool> temp_pair;
            if (!a_intfc->ReadRecordData(temp_pair)) {
                return false;
            }
            encodedStr.push_back(temp_pair);
        }
        a_str = Functions::String::decodeString(encodedStr);
        return true;
    }

    bool write_string(SKSE::SerializationInterface* a_intfc, const std::string& a_str) {
        const auto encodedStr = Functions::String::encodeString(a_str);
        // i first need the size to know no of iterations
        const auto size = encodedStr.size();
        if (!a_intfc->WriteRecordData(size)) {
            return false;
        }
        for (const auto& temp_pair : encodedStr) {
            if (!a_intfc->WriteRecordData(temp_pair)) {
                return false;
            }
        }
        return true;
    }

    // github.com/ozooma10/OSLAroused/blob/29ac62f220fadc63c829f6933e04be429d4f96b0/src/PersistedData.cpp
    template <typename T, typename U>
    // BaseData is based off how powerof3's did it in Afterlife
    class BaseData {
    public:
        float GetData(T formId, T missing) {
            Locker locker(m_Lock);
            if (auto idx = m_Data.find(formId) != m_Data.end()) {
                return m_Data[formId];
            }
            return missing;
        }

        void SetData(T formId, U value) {
            Locker locker(m_Lock);
            m_Data[formId] = value;
        }

        virtual const char* GetType() = 0;

        virtual bool Save(SKSE::SerializationInterface*, std::uint32_t, std::uint32_t) { return false; };
        virtual bool Save(SKSE::SerializationInterface*) { return false; };
        virtual bool Load(SKSE::SerializationInterface*) { return false; };

        void Clear() {
            Locker locker(m_Lock);
            m_Data.clear();
        };

        virtual void DumpToLog() = 0;

    protected:
        std::map<T, U> m_Data;

        using Lock = std::recursive_mutex;
        using Locker = std::lock_guard<Lock>;
        mutable Lock m_Lock;
    };

    class SaveLoadData : public BaseData<Types::SaveDataLHS, Types::SaveDataRHS> {
    public:
        void DumpToLog() override {
            // nothing for now
        }

        [[nodiscard]] bool Save(SKSE::SerializationInterface* serializationInterface) override {
            assert(serializationInterface);
            Locker locker(m_Lock);

            const auto numRecords = m_Data.size();
            if (!serializationInterface->WriteRecordData(numRecords)) {
                logger::error("Failed to save {} data records", numRecords);
                return false;
            }

            for (const auto& [lhs, rhs] : m_Data) {
                // we serialize formid, editorid, and refid separately
                std::uint32_t formid = lhs.first.form_id;
                logger::trace("Formid:{}", formid);
                if (!serializationInterface->WriteRecordData(formid)) {
                    logger::error("Failed to save formid");
                    return false;
                }

                const std::string editorid = lhs.first.editor_id;
                logger::trace("Editorid:{}", editorid);
                write_string(serializationInterface, editorid);

                std::uint32_t refid = lhs.second;
                logger::trace("Refid:{}", refid);
                if (!serializationInterface->WriteRecordData(refid)) {
                    logger::error("Failed to save refid");
                    return false;
                }

                // save the number of rhs records
                const auto numRhsRecords = rhs.size();
                if (!serializationInterface->WriteRecordData(numRhsRecords)) {
                    logger::error("Failed to save the size {} of rhs records", numRhsRecords);
                    return false;
                }

                for (const auto& rhs_ : rhs) {
                    logger::trace("size of rhs_: {}", sizeof(rhs_));
                    if (!serializationInterface->WriteRecordData(rhs_)) {
                        logger::error("Failed to save data");
                        return false;
                    }
                }
            }
            return true;
        }

        [[nodiscard]] bool Save(SKSE::SerializationInterface* serializationInterface, std::uint32_t type,
                                std::uint32_t version) override {
            if (!serializationInterface->OpenRecord(type, version)) {
                logger::error("Failed to open record for Data Serialization!");
                return false;
            }

            return Save(serializationInterface);
        }

        [[nodiscard]] bool Load(SKSE::SerializationInterface* serializationInterface) override {
            assert(serializationInterface);

            std::size_t recordDataSize;
            serializationInterface->ReadRecordData(recordDataSize);
            logger::info("Loading data from serialization interface with size: {}", recordDataSize);

            Locker locker(m_Lock);
            m_Data.clear();

            logger::trace("Loading data from serialization interface.");
            for (auto i = 0; i < recordDataSize; i++) {
                Types::SaveDataRHS rhs;

                std::uint32_t formid = 0;
                logger::trace("ReadRecordData:{}", serializationInterface->ReadRecordData(formid));
                if (!serializationInterface->ResolveFormID(formid, formid)) {
                    logger::error("Failed to resolve form ID, 0x{:X}.", formid);
                    continue;
                }

                std::string editorid;
                if (!read_string(serializationInterface, editorid)) {
                    logger::error("Failed to read editorid");
                    return false;
                }

                std::uint32_t refid = 0;
                logger::trace("ReadRecordData:{}", serializationInterface->ReadRecordData(refid));

                logger::trace("Formid:{}", formid);
                logger::trace("Refid:{}", refid);
                logger::trace("Editorid:{}", editorid);

                Types::SaveDataLHS lhs({formid, editorid}, refid);
                logger::trace("Reading value...");

                std::size_t rhsSize = 0;
                logger::trace("ReadRecordData: {}", serializationInterface->ReadRecordData(rhsSize));
                logger::trace("rhsSize: {}", rhsSize);

                for (auto j = 0; j < rhsSize; j++) {
                    Types::StageInstancePlain rhs_;
                    logger::trace("ReadRecordData: {}", serializationInterface->ReadRecordData(rhs_));
                    // print the content of rhs_ which is StageInstancePlain
                    logger::trace(
                        "rhs_ content: start_time: {}, no: {},"
                        "count: {}, is_fake: {}, is_decayed: {}, _elapsed: {}, _delay_start: {}, _delay_mag: {}, "
                        "_delay_formid: {}",
                        rhs_.start_time, rhs_.no, rhs_.count, rhs_.is_fake, rhs_.is_decayed, rhs_._elapsed,
                        rhs_._delay_start, rhs_._delay_mag, rhs_._delay_formid);
                    rhs.push_back(rhs_);
                }

                m_Data[lhs] = rhs;
                logger::info("Loaded data for formid {}, editorid {}, and refid {}", formid, editorid, refid);
            }

            return true;
        }
    };

    class DFSaveLoadData : public BaseData<Types::DFSaveDataLHS, Types::DFSaveDataRHS> {
    public:
        void DumpToLog() override {
            // nothing for now
        }

        [[nodiscard]] bool Save(SKSE::SerializationInterface* serializationInterface) override {
            assert(serializationInterface);
            Locker locker(m_Lock);

            const auto numRecords = m_Data.size();
            if (!serializationInterface->WriteRecordData(numRecords)) {
                logger::error("Failed to save {} data records", numRecords);
                return false;
            }

            for (const auto& [lhs, rhs] : m_Data) {
                // we serialize formid, editorid, and refid separately
                std::uint32_t formid = lhs.first;
                logger::trace("Formid:{}", formid);
                if (!serializationInterface->WriteRecordData(formid)) {
                    logger::error("Failed to save formid");
                    return false;
                }

                const std::string editorid = lhs.second;
                logger::trace("Editorid:{}", editorid);
                write_string(serializationInterface, editorid);

                // save the number of rhs records
                const auto numRhsRecords = rhs.size();
                if (!serializationInterface->WriteRecordData(numRhsRecords)) {
                    logger::error("Failed to save the size {} of rhs records", numRhsRecords);
                    return false;
                }

                for (const auto& rhs_ : rhs) {
                    logger::trace("size of rhs_: {}", sizeof(rhs_));
                    if (!serializationInterface->WriteRecordData(rhs_)) {
                        logger::error("Failed to save data");
                        return false;
                    }
                }
            }
            return true;
        }

        [[nodiscard]] bool Save(SKSE::SerializationInterface* serializationInterface, std::uint32_t type,
                                std::uint32_t version) override {
            if (!serializationInterface->OpenRecord(type, version)) {
                logger::error("Failed to open record for Data Serialization!");
                return false;
            }

            return Save(serializationInterface);
        }

        [[nodiscard]] bool Load(SKSE::SerializationInterface* serializationInterface) override {
            assert(serializationInterface);

            std::size_t recordDataSize;
            serializationInterface->ReadRecordData(recordDataSize);
            logger::info("Loading data from serialization interface with size: {}", recordDataSize);

            Locker locker(m_Lock);
            m_Data.clear();

            logger::trace("Loading data from serialization interface.");
            for (auto i = 0; i < recordDataSize; i++) {
                Types::DFSaveDataRHS rhs;

                std::uint32_t formid = 0;
                logger::trace("ReadRecordData:{}", serializationInterface->ReadRecordData(formid));
                if (!serializationInterface->ResolveFormID(formid, formid)) {
                    logger::error("Failed to resolve form ID, 0x{:X}.", formid);
                    continue;
                }

                std::string editorid;
                if (!read_string(serializationInterface, editorid)) {
                    logger::error("Failed to read editorid");
                    return false;
                }

                logger::trace("Formid:{}", formid);
                logger::trace("Editorid:{}", editorid);

                Types::DFSaveDataLHS lhs({formid, editorid});
                logger::trace("Reading value...");

                std::size_t rhsSize = 0;
                logger::trace("ReadRecordData: {}", serializationInterface->ReadRecordData(rhsSize));
                logger::trace("rhsSize: {}", rhsSize);

                for (auto j = 0; j < rhsSize; j++) {
                    Types::DFSaveData rhs_;
                    logger::trace("ReadRecordData: {}", serializationInterface->ReadRecordData(rhs_));
                    logger::trace(
                        "rhs_ content: dyn_formid: {}, customid_bool: {},"
                        "customid: {}, acteff_elapsed: {}",
                        rhs_.dyn_formid, rhs_.custom_id.first, rhs_.custom_id.second, rhs_.acteff_elapsed);
                    rhs.push_back(rhs_);
                }

                m_Data[lhs] = rhs;
                logger::info("Loaded data for formid {}, editorid {}", formid, editorid);
            }

            return true;
        }
    };
};
