#
# Copyright (C) 2016 The AOSParadox Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Overlay
DEVICE_PACKAGE_OVERLAYS += device/motorola/titan/overlay
PRODUCT_PACKAGE_OVERLAYS += device/motorola/titan/overlay

$(call inherit-product-if-exists, vendor/motorola/titan/titan-vendor.mk)

# Ramdisk
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,device/motorola/titan/ramdisk,root)

# Prebuilt
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,device/motorola/titan/prebuilt/system,system)

# Hadware QCOM
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    ro.hardware=qcom

PRODUCT_AAPT_CONFIG := normal
PRODUCT_AAPT_PREF_CONFIG := xhdpi

# CodeAurora MSM8226 Tree
include device/qcom/msm8226/msm8226.mk

# ANT+
PRODUCT_PACKAGES += \
    AntHalService \
    com.dsi.ant.antradio_library \
    libantradio

# Dalvik
PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.boot-dex2oat-threads=2 \
    dalvik.vm.dex2oat-threads=4

# FRP
PRODUCT_PROPERTY_OVERRIDES += \
    ro.frp.pst=/dev/block/platform/msm_sdcc.1/by-name/config

# GPS
PRODUCT_PACKAGES += \
    gps.msm8226

# Hardware
PRODUCT_PROPERTY_OVERRIDES += \
    qemu.hw.mainkeys=0

# Keystore
PRODUCT_PACKAGES += \
    keystore.msm8226 \
    keystore.qcom

# Lights
PRODUCT_PACKAGES += \
    lights.msm8226

# Power
PRODUCT_PACKAGES += \
    power.msm8226 \
    power.qcom

# Sensors
PRODUCT_PACKAGES += \
    sensord

# Wifi
PRODUCT_PACKAGES += \
    libcurl \
    libqsap_sdk \
    libQWiFiSoftApCfg \
    wcnss_service
