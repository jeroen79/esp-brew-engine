/*
 * esp-brew-engine
 * Copyright (C) Dekien Jeroen 2024
 *
 */
#include "esp_log.h"
#include "wifi-connect-lite.h"
#include "settings-manager.h"
#include <esp_http_server.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_flash_partitions.h"
#include "esp_image_format.h"

WiFiConnectLite *wifiConnector;
SettingsManager *settingsManager;

static const char *TAG = "Main";

esp_err_t indexGetHandler(httpd_req_t *req)
{
  extern const unsigned char index_html_start[] asm("_binary_index_html_start");
  extern const unsigned char index_html_end[] asm("_binary_index_html_end");
  const size_t index_html_size = (index_html_end - index_html_start);
  httpd_resp_set_type(req, "text/html");
  // httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_send(req, (const char *)index_html_start, index_html_size);

  return ESP_OK;
}

esp_err_t appHandler(httpd_req_t *req)
{
  ESP_LOGI(TAG, "Rebooting into app");

  httpd_resp_set_status(req, HTTPD_500); // Assume failure

  // we only have space for one OTA so it is always ota0
  esp_partition_subtype_t t = ESP_PARTITION_SUBTYPE_APP_OTA_0;
  const esp_partition_t *app_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, t, NULL);

  if (app_partition == NULL)
  {
    ESP_LOGE(TAG, "Update Partition not found!");
    return ESP_FAIL;
  }

  if (esp_ota_set_boot_partition(app_partition) == ESP_OK)
  {
    ESP_LOGI(TAG, "Rebooting");

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_send(req, NULL, 0);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();

    return ESP_OK;
  }

  return ESP_FAIL;
}

esp_err_t flashHandler(httpd_req_t *req)
{
  ESP_LOGI(TAG, "Receiving Flash");

  char buf[256];
  httpd_resp_set_status(req, HTTPD_500); // Assume failure

  uint32_t ret;
  uint32_t remaining = req->content_len;

  esp_ota_handle_t update_handle = 0;

  // we only have space for one OTA so it is always ota0
  esp_partition_subtype_t t = ESP_PARTITION_SUBTYPE_APP_OTA_0;
  const esp_partition_t *update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, t, NULL);

  if (update_partition == NULL)
  {
    ESP_LOGE(TAG, "Update Partition not found!");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Writing partition: %s , size 0x%lu", update_partition->label, update_partition->size);

  esp_err_t err = ESP_OK;
  err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
    return ESP_FAIL;
  }

  while (remaining > 0)
  {
    // Read the data
    int nBytes = (std::min<uint32_t>)(remaining, sizeof(buf));

    if ((ret = httpd_req_recv(req, buf, nBytes)) <= 0)
    {
      if (ret == HTTPD_SOCK_ERR_TIMEOUT)
      {
        // Timeout, just continue
        continue;
      }

      return ESP_FAIL;
    }

    size_t bytes_read = ret;

    remaining -= bytes_read;
    err = esp_ota_write(update_handle, buf, bytes_read);
    if (err != ESP_OK)
    {
      return ESP_FAIL;
    }
  }

  if ((esp_ota_end(update_handle) == ESP_OK) && (esp_ota_set_boot_partition(update_partition) == ESP_OK))
  {
    ESP_LOGI(TAG, "Rebooting");

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_send(req, NULL, 0);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();

    return ESP_OK;
  }

  ESP_LOGE(TAG, "OTA failed (%s)!\n", esp_err_to_name(err));

  if (update_handle)
  {
    esp_ota_abort(update_handle);
  }

  httpd_resp_set_status(req, HTTPD_500); // Assume failure
  httpd_resp_send(req, NULL, 0);
  return ESP_FAIL;
}

httpd_handle_t startWebserver(void)
{

  httpd_uri_t indexUri;
  indexUri.uri = "/";
  indexUri.method = HTTP_GET;
  indexUri.handler = indexGetHandler;

  httpd_uri_t flashUri;
  flashUri.uri = "/flash";
  flashUri.method = HTTP_POST;
  flashUri.handler = flashHandler;

  httpd_uri_t appUri;
  appUri.uri = "/app";
  appUri.method = HTTP_POST;
  appUri.handler = appHandler;

  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  // whiout this the esp crashed whitout a proper warning
  config.stack_size = 20480;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK)
  {
    // Set URI handlers
    httpd_register_uri_handler(server, &indexUri);
    httpd_register_uri_handler(server, &flashUri);
    httpd_register_uri_handler(server, &appUri);
    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}

/* Inside .cpp file, app_main function must be declared with C linkage */
extern "C" void app_main(void)
{
  // Our settings manager is helper for the nvs_get/write
  settingsManager = new SettingsManager();
  settingsManager->Init();

  // WifiConnect is a helper that connects to wifi or starts a wifi ap
  wifiConnector = new WiFiConnectLite(settingsManager);
  wifiConnector->Connect();

  startWebserver();

  ESP_LOGI(TAG, "Loader booted");
}
