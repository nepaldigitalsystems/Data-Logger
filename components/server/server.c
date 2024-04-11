
#include <cJSON.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <mdns.h>

// user defined header
#include <stdio.h>
// #include <toggle.h>

static const char *TAG = "HTTPD SERVER";

// reading the html file
extern const char html[] asm("_binary_test_html_start");
// setup for the home page
esp_err_t uri_home(httpd_req_t *req) {
  ESP_LOGI(TAG, "uri_handler is starting\n");
  httpd_resp_sendstr(req, html);
  // httpd_resp_sendstr(req, "this is home");
  return ESP_OK;
}

esp_err_t uri_hello(httpd_req_t *req) {
  ESP_LOGI(TAG, "uri_handler is starting\n");
  httpd_resp_sendstr(req, "hello to the world");
  return ESP_OK;
}

// setup foh receiving command from client
// esp_err_t uri_resp(httpd_req_t *req) {
//   char buffer[100];
//   memset(buffer, 0, sizeof(buffer));
//   httpd_req_recv(req, buffer, req->content_len);
//   cJSON *carrier = cJSON_Parse(buffer);
//   if (carrier != NULL) {
//     cJSON *level = cJSON_GetObjectItem(carrier, "level");
//     cJSON *gpio = cJSON_GetObjectItem(carrier, "gpio");
//     printf("\nGPIO -> %d\nLEVEL -> %s\n", gpio->valueint,
//            (int)level->valueint == 1 ? "true" : "false");
//   } else {
//     printf("invalid request\n");
//   }
//   cJSON_Delete(carrier);
//   return ESP_OK;
// }

// setup for the server
void server() {
  httpd_handle_t httpd_handler = NULL;
  httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();
  httpd_start(&httpd_handler, &httpd_config);

  httpd_uri_t httpd_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = uri_home,
  };
  httpd_register_uri_handler(httpd_handler, &httpd_uri);

  httpd_uri_t hello_uri = {
      .uri = "/hello",
      .method = HTTP_GET,
      .handler = uri_hello,
  };
  httpd_register_uri_handler(httpd_handler, &hello_uri);

  // httpd_uri_t toggle_uri = {
  //     .uri = "/toggle",
  //     .method = HTTP_POST,
  //     .handler = uri_resp,
  // };
  // httpd_register_uri_handler(httpd_handler, &toggle_uri);
}

// setup for the mdns
void mdns_service() {
  // initialize mnds
  esp_err_t esp = mdns_init();
  if (esp) {
    printf("something went wrong with mdns");
    return;
  }

  // setup the names
  mdns_hostname_set("void-esp32");
  mdns_instance_name_set("learn to use the server");
}

// void app_main() {
//   // initializing and connecting to the given AP
//   connect_init();
//   connect_sta("nepaldigisys", "NDS_0ffice", 10000);
//
//   mdns_service();
//
//   server();
// }
