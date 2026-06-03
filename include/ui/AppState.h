#pragma once

#include <QObject>
#include <QVariantList>
#include <qqmlintegration.h>

class AppState : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList availableDevices READ availableDevices NOTIFY availableDevicesChanged)
    Q_PROPERTY(quint32 selectedDeviceIndex READ selectedDeviceIndex WRITE setSelectedDeviceIndex NOTIFY selectedDeviceIndexChanged)
    Q_PROPERTY(qreal deviceSampleRate READ deviceSampleRate NOTIFY deviceSampleRateChanged)
    Q_PROPERTY(QVariantList frequencyBands READ frequencyBands NOTIFY frequencyBandsChanged)

public:
    explicit AppState(QObject *parent = nullptr) : QObject(parent)
    {
        m_deviceSampleRate = 0.0;
        m_selectedDeviceIndex = 0;
        m_peakLevel = 0.0;
    }

    [[nodiscard]] QVariantList availableDevices() const { return m_availableDevices; }
    [[nodiscard]] quint32 selectedDeviceIndex() const { return m_selectedDeviceIndex; }
    [[nodiscard]] qreal deviceSampleRate() const { return m_deviceSampleRate; }
    [[nodiscard]] QVariantList frequencyBands() const { return m_frequencyBands; }

    void setAvailableDevices(const std::vector<std::string>& device_names)
    {
        QMetaObject::invokeMethod(this, [this, device_names]() {
            m_availableDevices.clear();
            for (const auto& v : device_names) m_availableDevices.append(QString::fromStdString(v));
            emit availableDevicesChanged();
        }, Qt::QueuedConnection);
    }

    void setSelectedDeviceIndex(const size_t index)
    {
        if (m_selectedDeviceIndex == index) return;
        m_selectedDeviceIndex = static_cast<quint32>(index);
        emit selectedDeviceIndexChanged();
    }

    void setDeviceSampleRate(const uint32_t sampleRate)
    {
        if (qFuzzyCompare(m_deviceSampleRate, sampleRate)) return;
        m_deviceSampleRate = sampleRate;
        emit deviceSampleRateChanged();
    }

    void setFrequencyBands(const std::vector<float>& newBands)
    {
        QMetaObject::invokeMethod(this, [this, newBands]() {
            m_frequencyBands.clear();
            for (const auto& v : newBands) m_frequencyBands.append(v);
            emit frequencyBandsChanged();
        }, Qt::QueuedConnection);
    }

    Q_INVOKABLE void refreshDevices()
    {
        emit refreshDevicesRequested();
    }

signals:
    void availableDevicesChanged();
    void selectedDeviceIndexChanged();
    void deviceSampleRateChanged();
    void frequencyBandsChanged();
    void refreshDevicesRequested();

private:
    QVariantList m_availableDevices;
    quint32 m_selectedDeviceIndex;
    qreal m_deviceSampleRate;
    QVariantList m_frequencyBands;
    qreal m_peakLevel;
};