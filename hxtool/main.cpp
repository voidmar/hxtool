#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <string>
#include <unordered_map>

#include <windows.h>
#include <memory>
#include <assert.h>
#include <algorithm>
#include <functional>

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"

#include "serial.h"
#include "packet.h"
#include "serial.h"

#include "hx7318.h"

GLFWwindow* window = nullptr;

SerialPort device_port;
HANDLE valid_ports_changed_event = INVALID_HANDLE_VALUE;
std::vector<int> valid_devices;
char valid_device_names[256] = "\0\0"; // double terminated string for IMGUI
int selected_device = 0;

HX7318RegisterTable device_registers;
bool calculate_negative_voltages = true;
bool use_single_voltage = false;

void rebuild_valid_device_list()
{
    valid_devices.clear();

    // enumerate serial devices listed in the registry
    HKEY com_device_key = 0;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Hardware\\DeviceMap\\SerialComm", 0, KEY_READ, &com_device_key);

    for (int device_idx = 0; ; ++device_idx)
    {
        wchar_t device_reg_path[128];
        DWORD device_reg_path_chars = 128; // countof
        BYTE device_name_data[32];
        DWORD device_name_bytes = sizeof(device_name_data);
        DWORD reg_type;

        int err = RegEnumValue(com_device_key, device_idx, device_reg_path, &device_reg_path_chars, 0, &reg_type, device_name_data, &device_name_bytes);
        if (err != ERROR_SUCCESS)
            break;

        if (reg_type != REG_SZ)
            continue;

        if (wcswcs(device_reg_path, L"VCP") || wcswcs(device_reg_path, L"USBSER000"))
        {
            int port = 0;
            if (swscanf((wchar_t*)device_name_data, L"COM%d", &port) == 1)
            {
                valid_devices.push_back(port);
            }
        }
    }

    // and trigger an event whenever that registry key changes
    RegNotifyChangeKeyValue(com_device_key, false, REG_NOTIFY_CHANGE_LAST_SET, valid_ports_changed_event, TRUE);

    RegCloseKey(com_device_key);

    // build special name string for IMGUI to use
    auto last_valid_device_name = valid_device_names;
    for (auto p : valid_devices)
    {
        char device_name[16];
        sprintf_s(device_name, "COM%d", p);

        strcat(last_valid_device_name, device_name);
        last_valid_device_name += strlen(device_name);
        *last_valid_device_name++ = '\0';
    }
    *last_valid_device_name++ = '\0';
}

void disconnect_device()
{
    if (device_port.Is_Open())
    {
        HX16318_SendShutdownPacket(device_port);

        device_port.Close();
    }
}

void attempt_reconnect_device_to_selected_port()
{
    disconnect_device();

    wchar_t com_port_path[64];
    wsprintf(com_port_path, LR"(\\.\COM%d)", valid_devices[selected_device]);
    device_port.Open(com_port_path);

    HX16318_SendConnectPacket(device_port);
}

void scan_for_valid_port_changes()
{
    bool has_valid_device = !valid_devices.empty();
    int last_valid_device = has_valid_device ? valid_devices[selected_device] : 0;

    if (WaitForSingleObject(valid_ports_changed_event, 0) == WAIT_OBJECT_0)
    {
        rebuild_valid_device_list();

        auto new_valid_port_it = std::find(valid_devices.begin(), valid_devices.end(), last_valid_device);
        if (new_valid_port_it == valid_devices.end())
        {
            device_port.Close(); // the port is already dead, no point in sending the disconnection packet

            selected_device = 0;
            if (!valid_devices.empty())
                attempt_reconnect_device_to_selected_port();
        }
        else
        {
            selected_device = std::distance(new_valid_port_it, valid_devices.begin());
        }
    }
}

void init_program()
{
    valid_ports_changed_event = CreateEvent(NULL, TRUE, FALSE, NULL);

    rebuild_valid_device_list();

    attempt_reconnect_device_to_selected_port();

    device_registers.read_from_device(device_port);

    device_registers.copy_to_initial();
}

void shutdown_program()
{
    disconnect_device();

    CloseHandle(valid_ports_changed_event);
}

void run_connection_gui()
{
    scan_for_valid_port_changes();

    if (!valid_devices.size())
    {
        ImGui::TextDisabled("USB device not found");
        return;
    }

    if (ImGui::Combo("", &selected_device, valid_device_names))
    {
        attempt_reconnect_device_to_selected_port();
    }

    ImGui::SameLine();

    bool was_connected = device_port.Is_Open();
    bool stay_connected = was_connected;
    if (ImGui::Checkbox("Connected", &stay_connected))
    {
        if (was_connected && !stay_connected)
            disconnect_device();
        else if (!was_connected && stay_connected)
            attempt_reconnect_device_to_selected_port();
    }

}

#define IS_BIT_SET(r,b) (((r) & (1 << (b))) != 0)
#define CLEAR_BIT(r,b) ((r) & ~(1 << (b)))
#define SET_BIT(r,b) ((r) | (1 << b))
#define CHANGE_BIT(r,b,v) ((v) ? SET_BIT(r,b) : CLEAR_BIT(r,b))

void run_panel_settings_gui()
{
    auto& control_00 = device_registers.control_00;
    auto& control_01 = device_registers.control_01;

    bool flip_horizontal = IS_BIT_SET(control_00, 7);
    if (ImGui::Checkbox("Flip Horizontal", &flip_horizontal))
    {
        control_00 = CHANGE_BIT(control_00, 7, flip_horizontal);
    }

    bool flip_vertical = IS_BIT_SET(control_00, 6);
    if (ImGui::Checkbox("Flip Vertical", &flip_vertical))
    {
        control_00 = CHANGE_BIT(control_00, 6, flip_vertical);
    }

    bool single_cell = IS_BIT_SET(control_00, 4);
    if (ImGui::Checkbox("Single cell", &single_cell))
    {
        control_00 = CHANGE_BIT(control_00, 4, single_cell);
    }

    bool rgb_gamma = IS_BIT_SET(control_01, 1);
    if (ImGui::Checkbox("RGB Gamma", &rgb_gamma))
    {
        control_01 = CHANGE_BIT(control_01, 1, rgb_gamma);
    }

    ImGui::Separator();

    ImGui::Checkbox("Use single voltage", &use_single_voltage);
    ImGui::Checkbox("Calculate negative voltages", &calculate_negative_voltages);

}

void push_channel_color_scheme(float r, float g, float b)
{
    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(r, g, b, h, s, v);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(h, s * 0.5f, v * 0.5f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(h, s * 0.6f, v * 0.5f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(h, s * 0.7f, v * 0.5f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(h, s * 0.9f, v * 0.7f));

    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(h, s * 0.6f, v * 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(h, s * 0.7f, v * 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(h, s * 0.8f, v * 0.8f));
}

void pop_channel_color_scheme()
{
    ImGui::PopStyleColor(7);
}

float get_gamma_table_value(void* data, int idx)
{
    HXRegister* channel_table = (HXRegister*)data;

    int min_idx = idx / 32;
    int max_idx = min_idx + 1;

    return channel_table[idx];
}

void run_subframe_gamma_gui(HXRegister* channel_table)
{
    ImGui::BeginGroup();

    ImGui::PushID(channel_table);

    ImVec2 graph_size = { ImGui::GetFrameHeightWithSpacing() * 14 + ImGui::GetFrameHeight() * 2, 255 };

    ImGui::PlotLines("", get_gamma_table_value, channel_table, 8, 0, nullptr, 0, 255, graph_size);

    for (int i = 0; i < 8; i++)
    {
        if (i > 0)
            ImGui::SameLine();

        ImGui::PushID(i);

        ImGui::PushItemWidth(ImGui::GetFrameHeight() * 2);

        int channel_gui_idx = use_single_voltage ? 0 : i;

        int value = channel_table[channel_gui_idx];
        if (ImGui::DragInt("##v", &value, 0.1f, 0, 255, "%.0f"))
        {
            channel_table[channel_gui_idx] = (uint8_t)value;
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%.2X", value);

        ImGui::PopItemWidth();

        ImGui::PopID();
    }

    ImGui::PopID();

    ImGui::EndGroup();

    if (use_single_voltage)
    {
        for (int i = 1; i < 8; ++i)
        {
            channel_table[i] = channel_table[0].value;
            //assert(channel_table[i].dirty);
        }
    }
}

void run_color_gamma_gui(HXRegister* channel_table)
{
    run_subframe_gamma_gui(channel_table);

    ImGui::SameLine();
    if (calculate_negative_voltages)
    {
        for (int i = 0; i < 8; ++i)
        {
            channel_table[i + 8] = 255 - channel_table[i];
        }

        HXRegister temp_channel_table[8];
        memcpy(temp_channel_table, channel_table + 8, sizeof(temp_channel_table));

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        run_subframe_gamma_gui(temp_channel_table);
        ImGui::PopStyleVar();
    }
    else
    {
        run_subframe_gamma_gui(channel_table + 8);
    }
}

void run_gamma_gui()
{
    ImGui::Columns(2, 0, false);

    int reg_value = device_registers.com_p;
    if (ImGui::SliderInt("COM_P", &reg_value, 0, 255))
        device_registers.com_p = reg_value;

    reg_value = device_registers.com_n;
    if (ImGui::SliderInt("COM_N", &reg_value, 0, 255))
        device_registers.com_n = reg_value;

    // No idea what these two registers do
//     ImGui::NextColumn();
//     reg_value = device_registers.ring_p;
//     if (ImGui::SliderInt("RING_P", &reg_value, 0, 255))
//         device_registers.ring_p = reg_value;
// 
//     reg_value = device_registers.ring_n;
//     if (ImGui::SliderInt("RING_N", &reg_value, 0, 255))
//         device_registers.ring_n = reg_value;

    ImGui::Columns();

    if (IS_BIT_SET(device_registers.control_01, 1))
    {
        push_channel_color_scheme(1, 0, 0);
        run_color_gamma_gui(device_registers.gamma_table_r);
        pop_channel_color_scheme();

        ImGui::Separator();

        push_channel_color_scheme(0, 1, 0);
        run_color_gamma_gui(device_registers.gamma_table_g);
        ImGui::SameLine();
        pop_channel_color_scheme();

        ImGui::Separator();

        push_channel_color_scheme(0, 0, 1);
        run_color_gamma_gui(device_registers.gamma_table_b);
        pop_channel_color_scheme();
    }
    else
    {
        push_channel_color_scheme(1, 1, 1);
        run_color_gamma_gui(device_registers.gamma_table_r);
        pop_channel_color_scheme();
    }
}

float old_gui_scale = 0.0f;

void run_program_gui()
{
    float gui_scale;
    glfwGetWindowContentScale(window, &gui_scale, nullptr);

    if (gui_scale != old_gui_scale)
    {
        ImGui::GetStyle() = ImGuiStyle();
        ImGui::StyleColorsDark();
        ImGui::GetStyle().ScaleAllSizes(gui_scale);
        ImGui::GetIO().FontGlobalScale = gui_scale;
        old_gui_scale = gui_scale;
    }

    ImGui_ImplGlfwGL3_NewFrame();
    glClearColor(0.2f, 0.2f, 0.2f, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    //ImGui::GetIO().DisplayFramebufferScale

    ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ (float)width, (float)height });

    if (ImGui::Begin("hxtool", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
    {
        run_connection_gui();

        ImGui::Separator();

        run_panel_settings_gui();

        ImGui::Separator();

        run_gamma_gui();

        ImGui::Separator();

        if (ImGui::Button("Read from EEPROM"))
        {
            device_registers.read_from_device(device_port, true, 0xA8);
        }

        ImGui::SameLine();

        static bool enable_writes = false;

        if (!enable_writes)
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

        push_channel_color_scheme(1, 0, 0);
        if (ImGui::Button("Write to EEPROM") && enable_writes)
        {
            device_registers.write_to_device(device_port, false, 0xA8);
        }
        pop_channel_color_scheme();

        if (!enable_writes)
            ImGui::PopStyleVar(1);

        ImGui::SameLine();

        ImGui::Checkbox("Enable writing", &enable_writes);

    }
    ImGui::End();

    if (device_port.Is_Open())
    {
        // write any dirty register settings
        device_registers.write_to_device(device_port);
    }

    ImGui::Render();
}


int main(int argc, char* argv[])
{
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(800, 600, "hxtool", NULL, NULL);

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwDestroyWindow(window);
        return false;
    }

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfwGL3_Init(window, true);

    init_program();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        run_program_gui();

        ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

}
