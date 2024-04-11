
#include <cJSON.h>
#include <esp_event.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_err.h>

typedef struct {
  char *buffer;
  int buffer_index;
} chunk_payload_t;

static char *buffer;
cJSON* time_buffer;

// for getting the data from HTTPS
esp_err_t client_event(esp_http_client_event_t *evt) {
  chunk_payload_t *chunk = evt->user_data;
  switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
      ESP_LOGI("DATA_SIZE", "%d", evt->data_len);
      // for receiving data in chuncks
      chunk->buffer =
        realloc(chunk->buffer, chunk->buffer_index + evt->data_len + 1);
      memcpy(&chunk->buffer[chunk->buffer_index], (uint8_t *)evt->data,
             evt->data_len);
      chunk->buffer_index += evt->data_len;
      chunk->buffer[chunk->buffer_index] = 0;
      break;
    default:
      break;
  }
  buffer = chunk->buffer;
  return ESP_OK;
}

char* client_setup(){
  chunk_payload_t chunk = {0};
  esp_http_client_config_t client_config = {.url = "http://worldtimeapi.org/api/timezone/Asia/Kathmandu",
    .method = HTTP_METHOD_GET,
    .event_handler = client_event,
    .user_data = &chunk};
  esp_http_client_handle_t client_handle = esp_http_client_init(&client_config);
  esp_http_client_set_header(client_handle, "Content_type", "application/json");
  esp_http_client_perform(client_handle);
  time_buffer = cJSON_Parse(buffer);
  esp_http_client_cleanup(client_handle);

  if (time_buffer == NULL) {
    printf("null\n\n\n");
    return(0);
  } else {
    cJSON *datetime =cJSON_GetObjectItemCaseSensitive(time_buffer, "datetime");
    // printf("\ndatetime: %s\n\n", datetime->valuestring);
    char* time=datetime->valuestring;
    return (time);
  }
}

// cJSON* time_from_net(){
//    return(time_buffer);
// }
