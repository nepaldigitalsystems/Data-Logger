#include <cJSON.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <mdns.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <server.h>

static const char *TAG = "HTTPD SERVER";

// reading the html file
extern const char html[] asm("_binary_test_html_start");

// setup for the home page
esp_err_t uri_home(httpd_req_t *req) {
  ESP_LOGI(TAG, "uri_handler is starting\n");
  httpd_resp_sendstr(req, html);
  return ESP_OK;
}

uint8_t time_data[]={};
static int client_id=0;

static esp_err_t uri_ws(httpd_req_t *req) {
  client_id = httpd_req_to_sockfd(req);
  if (req->method == HTTP_GET)
    return ESP_OK;
  // char *response = {};
  char *response = malloc(20); // Allocate 100 bytes for response
  httpd_ws_frame_t ws_recv= {
    .type = HTTPD_WS_TYPE_TEXT,
    .payload = malloc(1024),
  };
  httpd_ws_recv_frame(req, &ws_recv, 1024);
  // printf("payload: %.*s\n",ws_recv.len, ws_recv.payload);
  cJSON *carrier = cJSON_Parse((char*)ws_recv.payload);
  if (carrier != NULL) {
    cJSON *data = cJSON_GetObjectItemCaseSensitive(carrier, "data");
    if(strcmp(data->valuestring,"date")==0){
      ESP_LOGI("WEBSOCKET", "Sending date");
      // snprintf(response, 20, "\nDATE: %02x/%02x/%02x\n\n", time_data[5], time_data[4], time_data[3]);
      snprintf(response, 20, "\{\"date\":\"%02x%02x%02x\"}", time_data[5], time_data[4], time_data[3]);
    }else if(strcmp(data->valuestring,"time")==0){
      ESP_LOGI("WEBSOCKET", "Sending time");
      // snprintf(response, 20, "\nTIME: %02x:%02x:%02x\n\n", time_data[2], time_data[1], time_data[0]);
      snprintf(response, 20, "\{\"time\":\"%02x%02x%02x\"}", time_data[2], time_data[1], time_data[0]);
    }else{
      ESP_LOGE("WEBSOCKET","No such request found");
      response  = "request not found ['_']";
    }
  }else {
    response  = "invalid request [-_-]\n";
  }
  httpd_ws_frame_t ws_response = {
    // .final = true,
    // .fragmented = true,
    .type = HTTPD_WS_TYPE_TEXT,
    .payload = (uint8_t *)response,
    .len = strlen(response)
  };
  return httpd_ws_send_frame(req, &ws_response);
}


// setup for the server
void server_init() {
  httpd_handle_t httpd_handler = NULL;
  httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();
  httpd_start(&httpd_handler, &httpd_config);
  httpd_uri_t httpd_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = uri_home,
  };
  httpd_register_uri_handler(httpd_handler, &httpd_uri);
  httpd_uri_t ws_uri = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = uri_ws,
    .is_websocket = true,
  };
  httpd_register_uri_handler(httpd_handler, &ws_uri);
}


void mdns_service() {
  esp_err_t esp = mdns_init();
  if (esp) {
    ESP_LOGE("MDNS","something went wrong");
    return;
  }
  mdns_hostname_set("void-esp32");
  mdns_instance_name_set("just for learning purpose");
}

