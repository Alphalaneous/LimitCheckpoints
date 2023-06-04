#include "includes.h"
#include <thread>
#include <fstream>

int checkpointCount = 20;

gd::CheckpointObject* (__thiscall* PlayLayer_createCheckpoint)(gd::PlayLayer* self);

gd::CheckpointObject* __fastcall PlayLayer_createCheckpoint_H(gd::PlayLayer* self, void*) {

    gd::CheckpointObject* ret = PlayLayer_createCheckpoint(self);

    if (self->m_checkpoints->count() > checkpointCount) {

        for (int i = 0; i < self->m_checkpoints->count() - checkpointCount + 1; i++) {

            gd::CheckpointObject* checkpoint = reinterpret_cast<gd::CheckpointObject*>(self->m_checkpoints->objectAtIndex(i));

            if (checkpoint != nullptr) {
                gd::GameObject* obj = checkpoint->m_gameObject;

                if (obj != nullptr) {

                    obj->setVisible(false);
                    self->removeObjectFromSection(obj);
                    obj->removeGlow();

                    self->m_checkpoints->removeObjectAtIndex(i);
                }
            }
        }
    }

    return ret;
}

void save() {
    std::ofstream configFile("checkpointCount.txt", std::ios::out | std::ios::trunc);
    configFile << std::to_string(checkpointCount);
    configFile.close();
}

void load() {
    if (!std::filesystem::exists("checkpointCount.txt")) {
        save();
        checkpointCount = 20;
        return;
    }

    std::ifstream configFile("checkpointCount.txt");

    try {
        std::stringstream buffer;
        buffer << configFile.rdbuf();

        std::string value = buffer.str();

        checkpointCount = stoi(value);
        
    }
    catch (std::exception& E) {
        checkpointCount = 20;
    }

    configFile.close();
}


void MH_CALL OnText(MegaHackExt::TextBox* obj, const char* text)
{
    std::string actualText = "";

    for (int i = 7; i < 12; i++) {

        if (!isdigit(text[i])) {
            break;
        }

        actualText += text[i];
    }

    if (actualText == "0") {
        actualText = "1";
    }

    obj->set(("Limit: " + actualText).c_str());

    if (actualText == "") {
        checkpointCount = 1;
    }
    else {
        checkpointCount = stoi(actualText);
    }

    save();

}

DWORD WINAPI thread_func(void* hModule) {
    MH_Initialize();
    load();

    using namespace MegaHackExt;

    Window* window = Window::Create("Checkpoints");

    TextBox* textbox = TextBox::Create("Limit: 20");

    textbox->set(("Limit: " + std::to_string(checkpointCount)).c_str());

    textbox->setCallback(OnText);

    window->add(textbox);

    Client::commit(window);


    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    MH_CreateHook(
        reinterpret_cast<void*>(base + 0x20B050),
        PlayLayer_createCheckpoint_H,
        reinterpret_cast<void**>(&PlayLayer_createCheckpoint)
    );

    MH_EnableHook(MH_ALL_HOOKS);
    
    return 0;
}



BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        CreateThread(0, 0x100, thread_func, handle, 0, 0);
        
    }
    return TRUE;
}