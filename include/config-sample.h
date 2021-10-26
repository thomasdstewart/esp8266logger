#define SSID "wifi_ssid"
#define PASS "wifo_ssid"
#define LOGGER_NAME "01"
#define URL "https://user:password@pushgateway.example.org/metrics/job/esp8266logger"
// openssl s_client -connect beryl.stewarts.org.uk:443 < /dev/null 2>/dev/null | openssl x509 -fingerprint -noout -in /dev/stdin | awk -F= '{print $2}' | tr 'A-Z' 'a-z' | sed 's/:/, 0x/g' | sed 's/^/{0x/' | sed 's/$/}/'
#define URL_FP {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67}
