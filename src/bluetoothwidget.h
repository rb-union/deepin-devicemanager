#pragma once

#include "deviceinfowidgetbase.h"

class BluetoothWidget : public DeviceInfoWidgetBase
{
    Q_OBJECT
public:
    explicit BluetoothWidget(QWidget *parent = nullptr);
    void initWidget() override;
};
