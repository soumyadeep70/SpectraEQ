#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include <thread>

#include "AppState.h"
#include "audio/audio_processor.h"


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    AppState appState;
    qmlRegisterSingletonInstance("SpectraEQ", 1, 0, "AppState", &appState);

    engine.loadFromModule("SpectraEQ", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    AUDIO_PROCESSOR ap;

    QObject::connect(
        &appState,
        &AppState::selectedDeviceIndexChanged,
        [&]()
        {
            ap.set_selected_device_index(appState.selectedDeviceIndex());
        }
    );

    QObject::connect(
        &appState,
        &AppState::refreshDevicesRequested,
        [&]()
        {
            ap.refresh_available_devices();
        }
    );

    std::jthread updateAppState([&appState, &ap](const std::stop_token& st)
    {
        while (!st.stop_requested()) {
            const auto availableDevices = ap.get_available_devices();
            std::vector<std::string> deviceNames;
            deviceNames.reserve(availableDevices.size());
            for (const auto& dev: availableDevices) deviceNames.emplace_back(dev.name);

            appState.setAvailableDevices(deviceNames);
            appState.setSelectedDeviceIndex(ap.get_selected_device_index());
            appState.setDeviceSampleRate(ap.get_device_sample_rate());
            appState.setFrequencyBands(ap.get_frequency_bands(40.0f, 16000.0f, 80));

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    QGuiApplication::setWindowIcon(QIcon(":/ui/assets/app_icon.png"));
    const int result = QGuiApplication::exec();
    updateAppState.request_stop();
    return result;
}