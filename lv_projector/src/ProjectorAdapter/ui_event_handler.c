#include "projector_adapter.h"

int projector_ui_on_screen_loaded(void)
{
    return projector_adapter_start_services(projector_adapter_get_protocol());
}

int projector_ui_on_screen_unloaded(void)
{
    return projector_adapter_stop_services();
}

int projector_ui_on_protocol_selected(ProjectorAdapterProtocol protocol)
{
    return projector_adapter_on_protocol_selected(protocol);
}

int projector_ui_on_device_selected(const ProjectorAdapterDevice *device)
{
    return projector_adapter_on_device_selected(device);
}

int projector_ui_on_disconnect_requested(void)
{
    return projector_adapter_disconnect();
}
