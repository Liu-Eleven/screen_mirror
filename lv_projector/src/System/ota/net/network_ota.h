#ifndef _NETWORK_OTA_H
#define _NETWORK_OTA_H

#define OTA_CHECK_0_IP      "47.93.37.190"
#define OTA_CONFIG_URL_0    "http://47.93.37.190/upgrade_package/test/H135/ota_config.ini"
#define OTA_FW_URL_0        "http://47.93.37.190/upgrade_package/test/H135/update.swu "

#define OTA_CHECK_1_IP      "47.93.37.190"
#define OTA_CONFIG_URL_1    "http://47.93.37.190/upgrade_package/test/H135/ota_config.ini"
#define OTA_FW_URL_1        "http://47.93.37.190/upgrade_package/test/H135/update.swu"

#define LOCAL_OTA_CONFIG        "/mnt/UDISK/ota_config.ini"

#define NET_OTA_CONFIG_PATH     "/tmp/ota_config.ini"
#define NET_OTA_FW_PATH         "/tmp/update.swu"
#define NET_OTA_UPDATE_LOG      "/tmp/ota_result.log"

#define MAINKEY_OTA_NAME        "ota"
#define SUBKEY_PRODUCT_NAME     "product"
#define SUBKEY_VERSION_NAME     "version"
#define SUBKEY_CHECK_NAME       "md5"
#define SUBKEY_TEST_NAME        "test_mode"

#endif
