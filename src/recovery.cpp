#include "recovery.h"

#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "async_ota.h"
#include "data.h"
#include "settings.h"
#include "task_queue.h"

#define SSID_NAME ("Owie-recovery")

namespace {
DNSServer dnsServer;
AsyncWebServer webServer(80);
}  // namespace

void recovery_setup() {
  // Keep recovery AP low power and isolated from station mode.
  WiFi.setOutputPower(0);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID_NAME);
  dnsServer.start(53, "*", WiFi.softAPIP());  // DNS spoofing.
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  // Recovery mode always starts from default settings.
  nukeSettings();
  webServer.onNotFound([&](AsyncWebServerRequest *request) {
    // Captive-portal style redirect straight to OTA updater.
    request->redirect("http://" + WiFi.softAPIP().toString() + "/update");
  });
  webServer.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(
        200, "text/css", STYLES_CSS_PROGMEM_ARRAY, STYLES_CSS_SIZE);
    response->addHeader("Cache-Control", "max-age=3600");
    request->send(response);
  });
  AsyncOta.listen(&webServer);
  webServer.begin();
  // Poll DNS in the cooperative task queue.
  TaskQueue.postRecurringTask([&]() { dnsServer.processNextRequest(); });
}
