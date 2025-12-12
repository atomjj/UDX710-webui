/**
 * @file usb_mode.c
 * @brief USB模式切换实现
 * 
 * 支持三种USB网络模式：CDC-NCM, CDC-ECM, RNDIS
 * 临时模式写入 /mnt/data/mode_tmp.cfg
 * 永久模式写入 /mnt/data/mode.cfg 并删除临时文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mongoose.h"
#include "usb_mode.h"
#include "http_utils.h"

/* 获取模式名称 */
const char* usb_mode_name(int mode) {
    switch (mode) {
        case USB_MODE_CDC_NCM: return "cdc_ncm";
        case USB_MODE_CDC_ECM: return "cdc_ecm";
        case USB_MODE_RNDIS:   return "rndis";
        default: return "unknown";
    }
}

/* 模式名称转数值 */
static int usb_mode_from_name(const char *name) {
    if (strcmp(name, "cdc_ncm") == 0) return USB_MODE_CDC_NCM;
    if (strcmp(name, "cdc_ecm") == 0) return USB_MODE_CDC_ECM;
    if (strcmp(name, "rndis") == 0)   return USB_MODE_RNDIS;
    return -1;
}

/* 读取配置文件中的模式值 */
static int read_mode_from_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    int mode = -1;
    if (fscanf(f, "%d", &mode) != 1) {
        mode = -1;
    }
    fclose(f);
    return mode;
}

/* 写入模式值到配置文件 */
static int write_mode_to_file(const char *path, int mode) {
    FILE *f = fopen(path, "w");
    if (!f) {
        printf("[usb_mode] 无法写入文件: %s\n", path);
        return -1;
    }
    fprintf(f, "%d", mode);
    fclose(f);
    return 0;
}

/* 获取当前USB模式 */
int usb_mode_get(void) {
    /* 优先读取临时配置 */
    int mode = read_mode_from_file(USB_MODE_TMP_CFG_PATH);
    if (mode > 0) return mode;
    
    /* 读取永久配置 */
    mode = read_mode_from_file(USB_MODE_CFG_PATH);
    return mode;
}

/* 设置USB模式 */
int usb_mode_set(int mode, int permanent) {
    if (mode < USB_MODE_CDC_NCM || mode > USB_MODE_RNDIS) {
        printf("[usb_mode] 无效的模式值: %d\n", mode);
        return -1;
    }
    
    if (permanent) {
        /* 永久模式：写入永久配置，删除临时配置 */
        if (write_mode_to_file(USB_MODE_CFG_PATH, mode) != 0) {
            return -1;
        }
        unlink(USB_MODE_TMP_CFG_PATH);
        printf("[usb_mode] 永久设置模式: %s (%d)\n", usb_mode_name(mode), mode);
    } else {
        /* 临时模式：写入临时配置 */
        if (write_mode_to_file(USB_MODE_TMP_CFG_PATH, mode) != 0) {
            return -1;
        }
        printf("[usb_mode] 临时设置模式: %s (%d)\n", usb_mode_name(mode), mode);
    }
    
    return 0;
}

/* GET /api/usb/mode - 获取当前USB模式 */
void handle_usb_mode_get(struct mg_connection *c, struct mg_http_message *hm) {
    HTTP_CHECK_GET(c, hm);
    
    int mode = usb_mode_get();
    const char *mode_name = (mode > 0) ? usb_mode_name(mode) : "unknown";
    
    /* 检查是否有临时配置 */
    int is_temporary = (access(USB_MODE_TMP_CFG_PATH, F_OK) == 0);
    
    char json[256];
    snprintf(json, sizeof(json),
        "{\"Code\":0,\"Error\":\"\",\"Data\":{"
        "\"mode\":\"%s\",\"mode_value\":%d,\"is_temporary\":%s"
        "}}",
        mode_name, mode, is_temporary ? "true" : "false");
    
    HTTP_OK(c, json);
}

/* POST /api/usb/mode - 设置USB模式 */
void handle_usb_mode_set(struct mg_connection *c, struct mg_http_message *hm) {
    HTTP_CHECK_POST(c, hm);
    
    char mode_str[32] = {0};
    int permanent = 0;
    int bval = 0;
    
    /* 解析JSON参数 */
    char *mode_val = mg_json_get_str(hm->body, "$.mode");
    if (mode_val) {
        strncpy(mode_str, mode_val, sizeof(mode_str) - 1);
        free(mode_val);
    }
    
    if (mg_json_get_bool(hm->body, "$.permanent", &bval)) {
        permanent = bval;
    }
    
    /* 验证模式 */
    if (strlen(mode_str) == 0) {
        HTTP_OK(c, "{\"Code\":1,\"Error\":\"mode参数不能为空\",\"Data\":null}");
        return;
    }
    
    int mode = usb_mode_from_name(mode_str);
    if (mode < 0) {
        HTTP_OK(c, "{\"Code\":1,\"Error\":\"无效的模式，支持: cdc_ncm, cdc_ecm, rndis\",\"Data\":null}");
        return;
    }
    
    /* 设置模式 */
    if (usb_mode_set(mode, permanent) != 0) {
        HTTP_OK(c, "{\"Code\":1,\"Error\":\"设置模式失败\",\"Data\":null}");
        return;
    }
    
    char json[256];
    snprintf(json, sizeof(json),
        "{\"Code\":0,\"Error\":\"\",\"Data\":{"
        "\"mode\":\"%s\",\"permanent\":%s,\"message\":\"设置成功，重启后生效\""
        "}}",
        mode_str, permanent ? "true" : "false");
    
    HTTP_OK(c, json);
}
