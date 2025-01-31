// Settings
// Generated from:-
// main/settings.def
// components/ESP32-RevK/settings.def

#include <stdint.h>
#include "sdkconfig.h"
#include "settings.h"
revk_settings_bits_t revk_settings_bits={0};
#define	str(s)	#s
#define	quote(s)	str(s)
revk_settings_t const revk_settings[]={
 {.type=REVK_SETTINGS_UNSIGNED,.gpio=1,.name="tx",.comment="Tx",.len=2,.def="15",.ptr=&tx,.size=sizeof(revk_gpio_t),.fix=1,.set=1,.flags="- ~↓↕⇕"},
 {.type=REVK_SETTINGS_UNSIGNED,.gpio=1,.name="rx",.comment="Rx",.len=2,.def="-16",.ptr=&rx,.size=sizeof(revk_gpio_t),.fix=1,.set=1,.flags="- ~↓↕⇕"},
 {.type=REVK_SETTINGS_UNSIGNED,.gpio=1,.name="run",.comment="Run button input",.len=3,.def="17",.ptr=&run,.size=sizeof(revk_gpio_t),.fix=1,.set=1,.flags="- ~↓↕⇕"},
 {.type=REVK_SETTINGS_UNSIGNED,.gpio=1,.name="pwr",.comment="Power relay control output",.len=3,.def="13",.ptr=&pwr,.size=sizeof(revk_gpio_t),.fix=1,.set=1,.flags="- ~↓↕⇕"},
 {.type=REVK_SETTINGS_UNSIGNED,.gpio=1,.name="mtr",.comment="Motor relay control output",.len=3,.def="47",.ptr=&mtr,.size=sizeof(revk_gpio_t),.fix=1,.set=1,.flags="- ~↓↕⇕"},
 {.type=REVK_SETTINGS_STRING,.name="pwrtopic",.comment="MQTT topic to control power relay",.len=8,.ptr=&pwrtopic,.malloc=1},
 {.type=REVK_SETTINGS_STRING,.name="mtrtopic",.comment="MQTT topic to control motor relay",.len=8,.ptr=&mtrtopic,.malloc=1},
 {.type=REVK_SETTINGS_BIT,.name="noecho",.comment="No local echo by default",.group=1,.len=6,.dot=2,.bit=REVK_SETTINGS_BITFIELD_noecho},
 {.type=REVK_SETTINGS_BIT,.name="nobig",.comment="No big text on paper tap",.group=1,.len=5,.dot=2,.bit=REVK_SETTINGS_BITFIELD_nobig},
 {.type=REVK_SETTINGS_BIT,.name="nover",.comment="No version print",.group=1,.len=5,.dot=2,.bit=REVK_SETTINGS_BITFIELD_nover},
 {.type=REVK_SETTINGS_BIT,.name="nocave",.comment="No colossal cave",.group=1,.len=6,.dot=2,.bit=REVK_SETTINGS_BITFIELD_nocave},
 {.type=REVK_SETTINGS_BIT,.name="nodc4",.comment="No DC4 handling (paper tape)",.group=1,.len=5,.dot=2,.bit=REVK_SETTINGS_BITFIELD_nodc4},
 {.type=REVK_SETTINGS_STRING,.name="wru",.comment="Who aRe yoU string",.len=3,.ptr=&wru,.malloc=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="ack",.comment="ACK",.len=3,.def="6",.ptr=&ack,.size=sizeof(uint8_t)},
 {.type=REVK_SETTINGS_UNSIGNED,.name="think",.comment="Think NULLs",.len=5,.def="10",.ptr=&think,.size=sizeof(uint8_t)},
 {.type=REVK_SETTINGS_BIT,.name="autocave",.comment="Auto start colossal cave",.group=2,.len=8,.dot=4,.bit=REVK_SETTINGS_BITFIELD_autocave},
 {.type=REVK_SETTINGS_BIT,.name="autoon",.comment="Auto power on",.group=2,.len=6,.dot=4,.bit=REVK_SETTINGS_BITFIELD_autoon},
 {.type=REVK_SETTINGS_BIT,.name="autoprompt",.comment="Auto prompt",.group=2,.len=10,.dot=4,.bit=REVK_SETTINGS_BITFIELD_autoprompt},
 {.type=REVK_SETTINGS_STRING,.name="autoconnect",.comment="Auto connect to host",.group=2,.len=11,.dot=4,.ptr=&autoconnect,.malloc=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="tapelead",.comment="Tape lead NULLs",.group=3,.len=8,.dot=4,.def="15",.ptr=&tapelead,.size=sizeof(uint8_t)},
 {.type=REVK_SETTINGS_UNSIGNED,.name="tapetail",.comment="Tape tail NULLs",.group=3,.len=8,.dot=4,.def="15",.ptr=&tapetail,.size=sizeof(uint8_t)},
 {.type=REVK_SETTINGS_UNSIGNED,.name="port",.comment="TCP port",.len=4,.def="33",.ptr=&port,.size=sizeof(uint16_t)},
 {.type=REVK_SETTINGS_UNSIGNED,.name="baud",.comment="Baud rate",.len=4,.def="110",.ptr=&baud,.size=sizeof(uint16_t),.decimal=2},
 {.type=REVK_SETTINGS_UNSIGNED,.name="databits",.comment="Data bits",.len=8,.def="8",.ptr=&databits,.size=sizeof(uint8_t)},
 {.type=REVK_SETTINGS_UNSIGNED,.name="stop",.comment="Stop bits (multiples of 0.5 bits)",.len=4,.def="2",.ptr=&stop,.size=sizeof(uint8_t),.decimal=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="linelen",.comment="Line length characters",.len=7,.def="72",.ptr=&linelen,.size=sizeof(uint8_t)},
 {.type=REVK_SETTINGS_UNSIGNED,.name="timecr",.comment="Time for CR (s) for whole line",.group=4,.len=6,.dot=4,.def="0.2",.ptr=&timecr,.size=sizeof(uint16_t),.decimal=3,.old="crtime"	},
 {.type=REVK_SETTINGS_UNSIGNED,.name="timepwron",.comment="Time for power on",.group=4,.len=9,.dot=4,.def="0.1",.ptr=&timepwron,.size=sizeof(uint16_t),.decimal=3},
 {.type=REVK_SETTINGS_UNSIGNED,.name="timemtron",.comment="Time for motor on",.group=4,.len=9,.dot=4,.def="0.25",.ptr=&timemtron,.size=sizeof(uint16_t),.decimal=3},
 {.type=REVK_SETTINGS_UNSIGNED,.name="timemtroff",.comment="Time for motor off",.group=4,.len=10,.dot=4,.def="1.25",.ptr=&timemtroff,.size=sizeof(uint16_t),.decimal=3},
 {.type=REVK_SETTINGS_UNSIGNED,.name="timepwroff",.comment="Time for power off",.group=4,.len=10,.dot=4,.def="0.2",.ptr=&timepwroff,.size=sizeof(uint16_t),.decimal=3},
 {.type=REVK_SETTINGS_UNSIGNED,.name="timeremidle",.comment="Idle time at end of remote",.group=4,.len=11,.dot=4,.def="1",.ptr=&timeremidle,.size=sizeof(uint32_t),.decimal=3,.old="idle"	},
 {.type=REVK_SETTINGS_UNSIGNED,.name="timekeyidle",.comment="Idle time at end of manual working",.group=4,.len=11,.dot=4,.def="600",.ptr=&timekeyidle,.size=sizeof(uint32_t),.decimal=3,.old="keyidle"	},
#ifdef	CONFIG_REVK_SETTINGS_PASSWORD
 {.type=REVK_SETTINGS_STRING,.name="password",.comment="Settings password (this is not sent securely so use with care on local networks you control)",.len=8,.ptr=&password,.malloc=1,.revk=1,.hide=1,.secret=1},
#endif
 {.type=REVK_SETTINGS_STRING,.name="hostname",.comment="Host name",.len=8,.ptr=&hostname,.malloc=1,.revk=1,.hide=1},
 {.type=REVK_SETTINGS_STRING,.name="appname",.comment="Application name",.len=7,.dq=1,.def=quote(CONFIG_REVK_APPNAME),.ptr=&appname,.malloc=1,.revk=1,.hide=1},
 {.type=REVK_SETTINGS_STRING,.name="otahost",.comment="OTA hostname",.group=5,.len=7,.dot=3,.dq=1,.def=quote(CONFIG_REVK_OTAHOST),.ptr=&otahost,.malloc=1,.revk=1,.live=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="otadays",.comment="OTA auto load (days)",.group=5,.len=7,.dot=3,.dq=1,.def=quote(CONFIG_REVK_OTADAYS),.ptr=&otadays,.size=sizeof(uint8_t),.revk=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="otastart",.comment="OTA check after startup (min seconds)",.group=5,.len=8,.dot=3,.def="600",.ptr=&otastart,.size=sizeof(uint16_t),.revk=1},
 {.type=REVK_SETTINGS_BIT,.name="otaauto",.comment="OTA auto upgrade",.group=5,.len=7,.dot=3,.def="1",.bit=REVK_SETTINGS_BITFIELD_otaauto,.revk=1,.hide=1,.live=1},
#ifdef	CONFIG_REVK_WEB_BETA
 {.type=REVK_SETTINGS_BIT,.name="otabeta",.comment="OTA from beta release",.group=5,.len=7,.dot=3,.bit=REVK_SETTINGS_BITFIELD_otabeta,.revk=1,.hide=1,.live=1},
#endif
 {.type=REVK_SETTINGS_BLOB,.name="otacert",.comment="OTA cert of otahost",.group=5,.len=7,.dot=3,.dq=1,.def=quote(CONFIG_REVK_OTACERT),.ptr=&otacert,.malloc=1,.revk=1,.base64=1},
 {.type=REVK_SETTINGS_STRING,.name="ntphost",.comment="NTP host",.len=7,.dq=1,.def=quote(CONFIG_REVK_NTPHOST),.ptr=&ntphost,.malloc=1,.revk=1},
 {.type=REVK_SETTINGS_STRING,.name="tz",.comment="Timezone (<a href='https://gist.github.com/alwynallan/24d96091655391107939' target=_blank>info</a>)",.len=2,.dq=1,.def=quote(CONFIG_REVK_TZ),.ptr=&tz,.malloc=1,.revk=1,.hide=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="watchdogtime",.comment="Watchdog (seconds)",.len=12,.dq=1,.def=quote(CONFIG_REVK_WATCHDOG),.ptr=&watchdogtime,.size=sizeof(uint32_t),.revk=1},
 {.type=REVK_SETTINGS_STRING,.name="topicgroup",.comment="MQTT Alternative hostname accepted for commands",.group=6,.len=10,.dot=5,.ptr=&topicgroup,.malloc=1,.revk=1,.array=2},
 {.type=REVK_SETTINGS_STRING,.name="topiccommand",.comment="MQTT Topic for commands",.group=6,.len=12,.dot=5,.def="command",.ptr=&topiccommand,.malloc=1,.revk=1,.old="prefixcommand"			},
 {.type=REVK_SETTINGS_STRING,.name="topicsetting",.comment="MQTT Topic for settings",.group=6,.len=12,.dot=5,.def="setting",.ptr=&topicsetting,.malloc=1,.revk=1,.old="prefixsetting"			},
 {.type=REVK_SETTINGS_STRING,.name="topicstate",.comment="MQTT Topic for state",.group=6,.len=10,.dot=5,.def="state",.ptr=&topicstate,.malloc=1,.revk=1,.old="prefixstate"			},
 {.type=REVK_SETTINGS_STRING,.name="topicevent",.comment="MQTT Topic for event",.group=6,.len=10,.dot=5,.def="event",.ptr=&topicevent,.malloc=1,.revk=1,.old="prefixevent"			},
 {.type=REVK_SETTINGS_STRING,.name="topicinfo",.comment="MQTT Topic for info",.group=6,.len=9,.dot=5,.def="info",.ptr=&topicinfo,.malloc=1,.revk=1,.old="prefixinfo"			},
 {.type=REVK_SETTINGS_STRING,.name="topicerror",.comment="MQTT Topic for error",.group=6,.len=10,.dot=5,.def="error",.ptr=&topicerror,.malloc=1,.revk=1,.old="prefixerror"			},
 {.type=REVK_SETTINGS_STRING,.name="topicha",.comment="MQTT Topic for homeassistant",.group=6,.len=7,.dot=5,.def="homeassistant",.ptr=&topicha,.malloc=1,.revk=1},
 {.type=REVK_SETTINGS_BIT,.name="prefixapp",.comment="MQTT use appname/ in front of hostname in topic",.group=7,.len=9,.dot=6,.dq=1,.def=quote(CONFIG_REVK_PREFIXAPP),.bit=REVK_SETTINGS_BITFIELD_prefixapp,.revk=1},
 {.type=REVK_SETTINGS_BIT,.name="prefixhost",.comment="MQTT use (appname/)hostname/topic instead of topic/(appname/)hostname",.group=7,.len=10,.dot=6,.dq=1,.def=quote(CONFIG_REVK_PREFIXHOST),.bit=REVK_SETTINGS_BITFIELD_prefixhost,.revk=1},
#ifdef	CONFIG_REVK_BLINK_DEF
 {.type=REVK_SETTINGS_UNSIGNED,.gpio=1,.name="blink",.comment="R, G, B LED array (set all the same for WS2812 LED)",.len=5,.dq=1,.def=quote(CONFIG_REVK_BLINK),.ptr=&blink,.size=sizeof(revk_gpio_t),.fix=1,.set=1,.flags="- ~↓↕⇕",.revk=1,.array=3},
#endif
 {.type=REVK_SETTINGS_BIT,.name="dark",.comment="Default LED off",.len=4,.bit=REVK_SETTINGS_BITFIELD_dark,.revk=1,.live=1},
#ifdef  CONFIG_IDF_TARGET_ESP32S3
 {.type=REVK_SETTINGS_UNSIGNED,.gpio=1,.name="factorygpio",.comment="Factory reset GPIO (press 3 times)",.len=11,.def="-21",.ptr=&factorygpio,.size=sizeof(revk_gpio_t),.fix=1,.set=1,.flags="- ~↓↕⇕",.revk=1},
#else
 {.type=REVK_SETTINGS_UNSIGNED,.gpio=1,.name="factorygpio",.comment="Factory reset GPIO (press 3 times)",.len=11,.ptr=&factorygpio,.size=sizeof(revk_gpio_t),.fix=1,.set=1,.flags="- ~↓↕⇕",.revk=1},
#endif
#ifdef  CONFIG_REVK_APMODE
#ifdef	CONFIG_REVK_APCONFIG
 {.type=REVK_SETTINGS_UNSIGNED,.name="apport",.comment="TCP port for config web pages on AP",.group=8,.len=6,.dot=2,.dq=1,.def=quote(CONFIG_REVK_APPORT),.ptr=&apport,.size=sizeof(uint16_t),.revk=1},
#endif
 {.type=REVK_SETTINGS_UNSIGNED,.name="aptime",.comment="Limit AP to time (seconds)",.group=8,.len=6,.dot=2,.dq=1,.def=quote(CONFIG_REVK_APTIME),.ptr=&aptime,.size=sizeof(uint32_t),.revk=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="apwait",.comment="Wait off line before starting AP (seconds)",.group=8,.len=6,.dot=2,.dq=1,.def=quote(CONFIG_REVK_APWAIT),.ptr=&apwait,.size=sizeof(uint32_t),.revk=1},
 {.type=REVK_SETTINGS_UNSIGNED,.gpio=1,.name="apgpio",.comment="Start AP on GPIO",.group=8,.len=6,.dot=2,.dq=1,.def=quote(CONFIG_REVK_APGPIO),.ptr=&apgpio,.size=sizeof(revk_gpio_t),.fix=1,.set=1,.flags="- ~↓↕⇕",.revk=1},
#endif
#ifdef  CONFIG_REVK_MQTT
 {.type=REVK_SETTINGS_STRING,.name="mqtthost",.comment="MQTT hostname",.group=9,.len=8,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MQTTHOST),.ptr=&mqtthost,.malloc=1,.revk=1,.array=CONFIG_REVK_MQTT_CLIENTS,.hide=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="mqttport",.comment="MQTT port",.group=9,.len=8,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MQTTPORT),.ptr=&mqttport,.size=sizeof(uint16_t),.revk=1,.array=CONFIG_REVK_MQTT_CLIENTS,.hide=1},
 {.type=REVK_SETTINGS_STRING,.name="mqttuser",.comment="MQTT username",.group=9,.len=8,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MQTTUSER),.ptr=&mqttuser,.malloc=1,.revk=1,.array=CONFIG_REVK_MQTT_CLIENTS,.hide=1},
 {.type=REVK_SETTINGS_STRING,.name="mqttpass",.comment="MQTT password",.group=9,.len=8,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MQTTPASS),.ptr=&mqttpass,.malloc=1,.revk=1,.array=CONFIG_REVK_MQTT_CLIENTS,.secret=1,.hide=1},
 {.type=REVK_SETTINGS_BLOB,.name="mqttcert",.comment="MQTT CA certificate",.group=9,.len=8,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MQTTCERT),.ptr=&mqttcert,.malloc=1,.revk=1,.array=CONFIG_REVK_MQTT_CLIENTS,.base64=1},
#endif
 {.type=REVK_SETTINGS_BLOB,.name="clientkey",.comment="Client Key (OTA and MQTT TLS)",.group=10,.len=9,.dot=6,.ptr=&clientkey,.malloc=1,.revk=1,.base64=1},
 {.type=REVK_SETTINGS_BLOB,.name="clientcert",.comment="Client certificate (OTA and MQTT TLS)",.group=10,.len=10,.dot=6,.ptr=&clientcert,.malloc=1,.revk=1,.base64=1},
#if     defined(CONFIG_REVK_WIFI) || defined(CONFIG_REVK_MESH)
 {.type=REVK_SETTINGS_UNSIGNED,.name="wifireset",.comment="Restart if WiFi off for this long (seconds)",.group=11,.len=9,.dot=4,.dq=1,.def=quote(CONFIG_REVK_WIFIRESET),.ptr=&wifireset,.size=sizeof(uint16_t),.revk=1,.hide=1},
 {.type=REVK_SETTINGS_STRING,.name="wifissid",.comment="WiFI SSID (name)",.group=11,.len=8,.dot=4,.dq=1,.def=quote(CONFIG_REVK_WIFISSID),.ptr=&wifissid,.malloc=1,.revk=1,.hide=1},
 {.type=REVK_SETTINGS_STRING,.name="wifipass",.comment="WiFi password",.group=11,.len=8,.dot=4,.dq=1,.def=quote(CONFIG_REVK_WIFIPASS),.ptr=&wifipass,.malloc=1,.revk=1,.hide=1,.secret=1},
 {.type=REVK_SETTINGS_STRING,.name="wifiip",.comment="WiFi Fixed IP",.group=11,.len=6,.dot=4,.dq=1,.def=quote(CONFIG_REVK_WIFIIP),.ptr=&wifiip,.malloc=1,.revk=1},
 {.type=REVK_SETTINGS_STRING,.name="wifigw",.comment="WiFi Fixed gateway",.group=11,.len=6,.dot=4,.dq=1,.def=quote(CONFIG_REVK_WIFIGW),.ptr=&wifigw,.malloc=1,.revk=1},
 {.type=REVK_SETTINGS_STRING,.name="wifidns",.comment="WiFi fixed DNS",.group=11,.len=7,.dot=4,.dq=1,.def=quote(CONFIG_REVK_WIFIDNS),.ptr=&wifidns,.malloc=1,.revk=1,.array=3},
 {.type=REVK_SETTINGS_OCTET,.name="wifibssid",.comment="WiFI BSSID",.group=11,.len=9,.dot=4,.dq=1,.def=quote(CONFIG_REVK_WIFIBSSID),.ptr=&wifibssid,.size=sizeof(uint8_t[6]),.revk=1,.hex=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="wifichan",.comment="WiFI channel",.group=11,.len=8,.dot=4,.dq=1,.def=quote(CONFIG_REVK_WIFICHAN),.ptr=&wifichan,.size=sizeof(uint8_t),.revk=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="wifiuptime",.comment="WiFI turns off after this many seconds",.group=11,.len=10,.dot=4,.dq=1,.def=quote(CONFIG_REVK_WIFIUPTIME),.ptr=&wifiuptime,.size=sizeof(uint16_t),.revk=1},
 {.type=REVK_SETTINGS_BIT,.name="wifips",.comment="WiFi power save",.group=11,.len=6,.dot=4,.dq=1,.def=quote(CONFIG_REVK_WIFIPS),.bit=REVK_SETTINGS_BITFIELD_wifips,.revk=1},
 {.type=REVK_SETTINGS_BIT,.name="wifimaxps",.comment="WiFi power save (max)",.group=11,.len=9,.dot=4,.dq=1,.def=quote(CONFIG_REVK_WIFIMAXPS),.bit=REVK_SETTINGS_BITFIELD_wifimaxps,.revk=1},
#endif
#ifndef	CONFIG_REVK_MESH
 {.type=REVK_SETTINGS_STRING,.name="apssid",.comment="AP mode SSID (name)",.group=8,.len=6,.dot=2,.dq=1,.def=quote(CONFIG_REVK_APSSID),.ptr=&apssid,.malloc=1,.revk=1},
 {.type=REVK_SETTINGS_STRING,.name="appass",.comment="AP mode password",.group=8,.len=6,.dot=2,.dq=1,.def=quote(CONFIG_REVK_APPASS),.ptr=&appass,.malloc=1,.revk=1,.secret=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="apmax",.comment="AP max clients",.group=8,.len=5,.dot=2,.dq=1,.def=quote(CONFIG_REVK_APMAX),.ptr=&apmax,.size=sizeof(uint8_t),.revk=1,.hide=1},
 {.type=REVK_SETTINGS_STRING,.name="apip",.comment="AP mode block",.group=8,.len=4,.dot=2,.dq=1,.def=quote(CONFIG_REVK_APIP),.ptr=&apip,.malloc=1,.revk=1},
 {.type=REVK_SETTINGS_BIT,.name="aplr",.comment="AP LR mode",.group=8,.len=4,.dot=2,.dq=1,.def=quote(CONFIG_REVK_APLR),.bit=REVK_SETTINGS_BITFIELD_aplr,.revk=1},
 {.type=REVK_SETTINGS_BIT,.name="aphide",.comment="AP hide SSID",.group=8,.len=6,.dot=2,.dq=1,.def=quote(CONFIG_REVK_APHIDE),.bit=REVK_SETTINGS_BITFIELD_aphide,.revk=1},
#endif
#ifdef	CONFIG_REVK_MESH
 {.type=REVK_SETTINGS_STRING,.name="nodename",.comment="Mesh node name",.len=8,.ptr=&nodename,.malloc=1,.revk=1,.hide=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="meshreset",.comment="Reset if mesh off for this long (seconds)",.group=12,.len=9,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MESHRESET),.ptr=&meshreset,.size=sizeof(uint16_t),.revk=1,.hide=1},
 {.type=REVK_SETTINGS_OCTET,.name="meshid",.comment="Mesh ID (hex)",.group=12,.len=6,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MESHID),.ptr=&meshid,.size=sizeof(uint8_t[6]),.revk=1,.hex=1,.hide=1},
 {.type=REVK_SETTINGS_OCTET,.name="meshkey",.comment="Mesh key",.group=12,.len=7,.dot=4,.ptr=&meshkey,.size=sizeof(uint8_t[16]),.revk=1,.secret=1,.hex=1,.hide=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="meshwidth",.comment="Mesh width",.group=12,.len=9,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MESHWIDTH),.ptr=&meshwidth,.size=sizeof(uint16_t),.revk=1,.hide=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="meshdepth",.comment="Mesh depth",.group=12,.len=9,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MESHDEPTH),.ptr=&meshdepth,.size=sizeof(uint16_t),.revk=1,.hide=1},
 {.type=REVK_SETTINGS_UNSIGNED,.name="meshmax",.comment="Mesh max devices",.group=12,.len=7,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MESHMAX),.ptr=&meshmax,.size=sizeof(uint16_t),.revk=1,.hide=1},
 {.type=REVK_SETTINGS_STRING,.name="meshpass",.comment="Mesh AP password",.group=12,.len=8,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MESHPASS),.ptr=&meshpass,.malloc=1,.revk=1,.secret=1,.hide=1},
 {.type=REVK_SETTINGS_BIT,.name="meshlr",.comment="Mesh use LR mode",.group=12,.len=6,.dot=4,.dq=1,.def=quote(CONFIG_REVK_MESHLR),.bit=REVK_SETTINGS_BITFIELD_meshlr,.revk=1,.hide=1},
 {.type=REVK_SETTINGS_BIT,.name="meshroot",.comment="This is preferred mesh root",.group=12,.len=8,.dot=4,.bit=REVK_SETTINGS_BITFIELD_meshroot,.revk=1,.hide=1},
#endif
{0}};
#undef quote
#undef str
revk_gpio_t tx={0};
revk_gpio_t rx={0};
revk_gpio_t run={0};
revk_gpio_t pwr={0};
revk_gpio_t mtr={0};
char* pwrtopic=NULL;
char* mtrtopic=NULL;
char* wru=NULL;
uint8_t ack=0;
uint8_t think=0;
char* autoconnect=NULL;
uint8_t tapelead=0;
uint8_t tapetail=0;
uint16_t port=0;
uint16_t baud=0;
uint8_t databits=0;
uint8_t stop=0;
uint8_t linelen=0;
uint16_t timecr=0;
uint16_t timepwron=0;
uint16_t timemtron=0;
uint16_t timemtroff=0;
uint16_t timepwroff=0;
uint32_t timeremidle=0;
uint32_t timekeyidle=0;
#ifdef	CONFIG_REVK_SETTINGS_PASSWORD
char* password=NULL;
#endif
char* hostname=NULL;
char* appname=NULL;
char* otahost=NULL;
uint8_t otadays=0;
uint16_t otastart=0;
#ifdef	CONFIG_REVK_WEB_BETA
#endif
revk_settings_blob_t* otacert=NULL;
char* ntphost=NULL;
char* tz=NULL;
uint32_t watchdogtime=0;
char* topicgroup[2]={0};
char* topiccommand=NULL;
char* topicsetting=NULL;
char* topicstate=NULL;
char* topicevent=NULL;
char* topicinfo=NULL;
char* topicerror=NULL;
char* topicha=NULL;
#ifdef	CONFIG_REVK_BLINK_DEF
revk_gpio_t blink[3]={0};
#endif
#ifdef  CONFIG_IDF_TARGET_ESP32S3
revk_gpio_t factorygpio={0};
#else
revk_gpio_t factorygpio={0};
#endif
#ifdef  CONFIG_REVK_APMODE
#ifdef	CONFIG_REVK_APCONFIG
uint16_t apport=0;
#endif
uint32_t aptime=0;
uint32_t apwait=0;
revk_gpio_t apgpio={0};
#endif
#ifdef  CONFIG_REVK_MQTT
char* mqtthost[CONFIG_REVK_MQTT_CLIENTS]={0};
uint16_t mqttport[CONFIG_REVK_MQTT_CLIENTS]={0};
char* mqttuser[CONFIG_REVK_MQTT_CLIENTS]={0};
char* mqttpass[CONFIG_REVK_MQTT_CLIENTS]={0};
revk_settings_blob_t* mqttcert[CONFIG_REVK_MQTT_CLIENTS]={0};
#endif
revk_settings_blob_t* clientkey=NULL;
revk_settings_blob_t* clientcert=NULL;
#if     defined(CONFIG_REVK_WIFI) || defined(CONFIG_REVK_MESH)
uint16_t wifireset=0;
char* wifissid=NULL;
char* wifipass=NULL;
char* wifiip=NULL;
char* wifigw=NULL;
char* wifidns[3]={0};
uint8_t wifibssid[6]={0};
uint8_t wifichan=0;
uint16_t wifiuptime=0;
#endif
#ifndef	CONFIG_REVK_MESH
char* apssid=NULL;
char* appass=NULL;
uint8_t apmax=0;
char* apip=NULL;
#endif
#ifdef	CONFIG_REVK_MESH
char* nodename=NULL;
uint16_t meshreset=0;
uint8_t meshid[6]={0};
uint8_t meshkey[16]={0};
uint16_t meshwidth=0;
uint16_t meshdepth=0;
uint16_t meshmax=0;
char* meshpass=NULL;
#endif
const char revk_settings_secret[]="✶✶✶✶✶✶✶✶";
