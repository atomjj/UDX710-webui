/**
 * @file usb_mode.h
 * @brief USB模式切换头文件
 */

#ifndef USB_MODE_H
#define USB_MODE_H

#include "mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif

/* USB模式定义 */
#define USB_MODE_CDC_NCM  1
#define USB_MODE_CDC_ECM  2
#define USB_MODE_RNDIS    3

/* 配置文件路径 */
#define USB_MODE_CFG_PATH     "/mnt/data/mode.cfg"
#define USB_MODE_TMP_CFG_PATH "/mnt/data/mode_tmp.cfg"

/**
 * @brief 设置USB模式
 * @param mode 模式值 (1=CDC-NCM, 2=CDC-ECM, 3=RNDIS)
 * @param permanent 是否永久保存 (1=永久, 0=临时)
 * @return 0成功, -1失败
 */
int usb_mode_set(int mode, int permanent);

/**
 * @brief 获取当前USB模式
 * @return 模式值 (1=CDC-NCM, 2=CDC-ECM, 3=RNDIS), -1表示未设置
 */
int usb_mode_get(void);

/**
 * @brief 获取模式名称
 * @param mode 模式值
 * @return 模式名称字符串
 */
const char* usb_mode_name(int mode);

/* HTTP API处理函数 */
void handle_usb_mode_get(struct mg_connection *c, struct mg_http_message *hm);
void handle_usb_mode_set(struct mg_connection *c, struct mg_http_message *hm);

#ifdef __cplusplus
}
#endif

#endif /* USB_MODE_H */
