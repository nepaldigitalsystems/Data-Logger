#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <netdb.h>
#include <nvs_flash.h>

static char *TAG = "WIFI_CONNECT";

static const uint8_t CONNECTED = BIT0;
static const uint8_t DISCONNECTED = BIT1;
EventGroupHandle_t wifi_events;
esp_netif_t *esp_netif;

void event_handler(void *event_handler_arg, esp_event_base_t event_base,
                   int32_t event_id, void *event_data) {
  switch (event_id) {
  case WIFI_EVENT_STA_START:
    ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
    esp_wifi_connect();
    break;
  case WIFI_EVENT_STA_CONNECTED:
    ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
    break;
  case WIFI_EVENT_STA_DISCONNECTED:
    ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
    xEventGroupSetBits(wifi_events, DISCONNECTED);
    break;
  case IP_EVENT_STA_GOT_IP:
    ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
    xEventGroupSetBits(wifi_events, CONNECTED);
    break;
  default:
    break;
  }
}

void connect_init(void) {
  nvs_flash_init();
  esp_netif_init();
  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wifi_init_config);
  esp_event_loop_create_default();
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler,
                             NULL);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
}

esp_err_t connect_sta(char *SSID, char *PASSWORD, int TIMEOUT) {
  wifi_events = xEventGroupCreate();
  esp_netif = esp_netif_create_default_wifi_sta();
  esp_wifi_set_mode(WIFI_MODE_STA);

  wifi_config_t wifi_config = {};
  strncpy((char *)wifi_config.sta.ssid, SSID, sizeof(wifi_config.sta.ssid) - 1);
  strncpy((char *)wifi_config.sta.password, PASSWORD,
          sizeof(wifi_config.sta.password) - 1);
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_start();

  EventBits_t result =
      xEventGroupWaitBits(wifi_events, CONNECTED | DISCONNECTED, true, false,
                          pdMS_TO_TICKS(TIMEOUT));
  if (result) {
    return ESP_OK;
  } else {
    return ESP_FAIL;
  }
};
