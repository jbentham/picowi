// PicoWi IOCTL definitions, see http://iosoft.blog/picowi for details
//
// Copyright (c) 2022, Jeremy P Bentham
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#define IOCTL_WAIT          30      // Time to wait for ioctl response (msec)
#define IOCTL_POLL_MSEC     10      // Polling interval for ioctl responses
#define IOCTL_MAX_BLKLEN    1600    // Max IOCTL length (really 1536)

#define SDPCM_CHAN_CTRL     0       // SDPCM control channel
#define SDPCM_CHAN_EVT      1       // SDPCM async event channel
#define SDPCM_CHAN_DATA     2       // SDPCM data channel

// WiFi bands
#define WIFI_BAND_ANY       0
#define WIFI_BAND_5GHZ      1
#define WIFI_BAND_2_4GHZ    2
  
#pragma pack(1)

// SDPCM header
typedef struct {
    uint16_t len,       // sdpcm_header.frametag
             notlen;
    uint8_t  seq,       // sdpcm_sw_header
             chan,
             nextlen,
             hdrlen,
             flow,
             credit,
             reserved[2];
} SDPCM_HDR;

// IOCTL header
typedef struct {
    uint32_t cmd;       // cdc_header
    uint16_t outlen,
             inlen;
    uint32_t flags,
             status;
} IOCTL_HDR;

// IOCTL command with SDPCM and IOCTL headers
typedef struct
{
    SDPCM_HDR sdpcm;
    IOCTL_HDR ioctl;
    uint8_t data[IOCTL_MAX_BLKLEN];
} IOCTL_CMD;

// BDC header
typedef struct {
    uint8_t flags;
    uint8_t priority;
    uint8_t flags2;
    uint8_t offset;
} BDC_HDR;

// IOCTL response with SDPCM header
// (then an IOCTL header after some padding)
typedef union
{
    SDPCM_HDR sdpcm;
    uint8_t data[IOCTL_MAX_BLKLEN];
} IOCTL_RSP;

// IOCTL command or response message
typedef struct
{
    union 
    {
        IOCTL_CMD cmd;
        IOCTL_RSP rsp;
        uint8_t data[IOCTL_MAX_BLKLEN];
    };
} IOCTL_MSG;
#pragma pack()

int ioctl_set_uint32(char *name, int wait_msec, uint32_t val);
int ioctl_set_data2(char *name, int namelen, int wait_msec, void *data, int len);
int ioctl_set_intx2(char *name, int wait_msec, int val1, int val2);
int ioctl_get_data(char *name, int wait_msec, uint8_t *data, int dlen);
int ioctl_set_data(char *name, int wait_msec, void *data, int len);
int ioctl_wr_int32(int cmd, int wait_msec, int val);
int ioctl_wr_data(int cmd, int wait_msec, void *data, int len);
int ioctl_rd_data(int cmd, int wait_msec, void *data, int len);
int ioctl_cmd(int cmd, char *name, int namelen, int wait_msec, int wr, void *data, int dlen);
int ioctl_resp_match(int cmd, void *data, int dlen);
void ioctl_err_display(int retval);

#define WLC_IOCTL_MAGIC                    (0x14e46c77)
#define WLC_IOCTL_VERSION                  (1)
#define WLC_IOCTL_SMLEN                    (256)
#define WLC_IOCTL_MEDLEN                   (1536)
#define WLC_IOCTL_MAXLEN                   (8192)

#define WLC_GET_MAGIC                      ( (uint32_t)0 )
#define WLC_GET_VERSION                    ( (uint32_t)1 )
#define WLC_UP                             ( (uint32_t)2 )
#define WLC_DOWN                           ( (uint32_t)3 )
#define WLC_GET_LOOP                       ( (uint32_t)4 )
#define WLC_SET_LOOP                       ( (uint32_t)5 )
#define WLC_DUMP                           ( (uint32_t)6 )
#define WLC_GET_MSGLEVEL                   ( (uint32_t)7 )
#define WLC_SET_MSGLEVEL                   ( (uint32_t)8 )
#define WLC_GET_PROMISC                    ( (uint32_t)9 )
#define WLC_SET_PROMISC                    ( (uint32_t)10 )
#define WLC_GET_RATE                       ( (uint32_t)12 )
#define WLC_GET_INSTANCE                   ( (uint32_t)14 )
#define WLC_GET_INFRA                      ( (uint32_t)19 )
#define WLC_SET_INFRA                      ( (uint32_t)20 )
#define WLC_GET_AUTH                       ( (uint32_t)21 )
#define WLC_SET_AUTH                       ( (uint32_t)22 )
#define WLC_GET_BSSID                      ( (uint32_t)23 )
#define WLC_SET_BSSID                      ( (uint32_t)24 )
#define WLC_GET_SSID                       ( (uint32_t)25 )
#define WLC_SET_SSID                       ( (uint32_t)26 )
#define WLC_RESTART                        ( (uint32_t)27 )
#define WLC_GET_CHANNEL                    ( (uint32_t)29 )
#define WLC_SET_CHANNEL                    ( (uint32_t)30 )
#define WLC_GET_SRL                        ( (uint32_t)31 )
#define WLC_SET_SRL                        ( (uint32_t)32 )
#define WLC_GET_LRL                        ( (uint32_t)33 )
#define WLC_SET_LRL                        ( (uint32_t)34 )
#define WLC_GET_PLCPHDR                    ( (uint32_t)35 )
#define WLC_SET_PLCPHDR                    ( (uint32_t)36 )
#define WLC_GET_RADIO                      ( (uint32_t)37 )
#define WLC_SET_RADIO                      ( (uint32_t)38 )
#define WLC_GET_PHYTYPE                    ( (uint32_t)39 )
#define WLC_DUMP_RATE                      ( (uint32_t)40 )
#define WLC_SET_RATE_PARAMS                ( (uint32_t)41 )
#define WLC_GET_KEY                        ( (uint32_t)44 )
#define WLC_SET_KEY                        ( (uint32_t)45 )
#define WLC_GET_REGULATORY                 ( (uint32_t)46 )
#define WLC_SET_REGULATORY                 ( (uint32_t)47 )
#define WLC_GET_PASSIVE_SCAN               ( (uint32_t)48 )
#define WLC_SET_PASSIVE_SCAN               ( (uint32_t)49 )
#define WLC_SCAN                           ( (uint32_t)50 )
#define WLC_SCAN_RESULTS                   ( (uint32_t)51 )
#define WLC_DISASSOC                       ( (uint32_t)52 )
#define WLC_REASSOC                        ( (uint32_t)53 )
#define WLC_GET_ROAM_TRIGGER               ( (uint32_t)54 )
#define WLC_SET_ROAM_TRIGGER               ( (uint32_t)55 )
#define WLC_GET_ROAM_DELTA                 ( (uint32_t)56 )
#define WLC_SET_ROAM_DELTA                 ( (uint32_t)57 )
#define WLC_GET_ROAM_SCAN_PERIOD           ( (uint32_t)58 )
#define WLC_SET_ROAM_SCAN_PERIOD           ( (uint32_t)59 )
#define WLC_EVM                            ( (uint32_t)60 )
#define WLC_GET_TXANT                      ( (uint32_t)61 )
#define WLC_SET_TXANT                      ( (uint32_t)62 )
#define WLC_GET_ANTDIV                     ( (uint32_t)63 )
#define WLC_SET_ANTDIV                     ( (uint32_t)64 )
#define WLC_GET_CLOSED                     ( (uint32_t)67 )
#define WLC_SET_CLOSED                     ( (uint32_t)68 )
#define WLC_GET_MACLIST                    ( (uint32_t)69 )
#define WLC_SET_MACLIST                    ( (uint32_t)70 )
#define WLC_GET_RATESET                    ( (uint32_t)71 )
#define WLC_SET_RATESET                    ( (uint32_t)72 )
#define WLC_LONGTRAIN                      ( (uint32_t)74 )
#define WLC_GET_BCNPRD                     ( (uint32_t)75 )
#define WLC_SET_BCNPRD                     ( (uint32_t)76 )
#define WLC_GET_DTIMPRD                    ( (uint32_t)77 )
#define WLC_SET_DTIMPRD                    ( (uint32_t)78 )
#define WLC_GET_SROM                       ( (uint32_t)79 )
#define WLC_SET_SROM                       ( (uint32_t)80 )
#define WLC_GET_WEP_RESTRICT               ( (uint32_t)81 )
#define WLC_SET_WEP_RESTRICT               ( (uint32_t)82 )
#define WLC_GET_COUNTRY                    ( (uint32_t)83 )
#define WLC_SET_COUNTRY                    ( (uint32_t)84 )
#define WLC_GET_PM                         ( (uint32_t)85 )
#define WLC_SET_PM                         ( (uint32_t)86 )
#define WLC_GET_WAKE                       ( (uint32_t)87 )
#define WLC_SET_WAKE                       ( (uint32_t)88 )
#define WLC_GET_FORCELINK                  ( (uint32_t)90 )
#define WLC_SET_FORCELINK                  ( (uint32_t)91 )
#define WLC_FREQ_ACCURACY                  ( (uint32_t)92 )
#define WLC_CARRIER_SUPPRESS               ( (uint32_t)93 )
#define WLC_GET_PHYREG                     ( (uint32_t)94 )
#define WLC_SET_PHYREG                     ( (uint32_t)95 )
#define WLC_GET_RADIOREG                   ( (uint32_t)96 )
#define WLC_SET_RADIOREG                   ( (uint32_t)97 )
#define WLC_GET_REVINFO                    ( (uint32_t)98 )
#define WLC_GET_UCANTDIV                   ( (uint32_t)99 )
#define WLC_SET_UCANTDIV                   ( (uint32_t)100 )
#define WLC_R_REG                          ( (uint32_t)101 )
#define WLC_W_REG                          ( (uint32_t)102 )
#define WLC_GET_MACMODE                    ( (uint32_t)105 )
#define WLC_SET_MACMODE                    ( (uint32_t)106 )
#define WLC_GET_MONITOR                    ( (uint32_t)107 )
#define WLC_SET_MONITOR                    ( (uint32_t)108 )
#define WLC_GET_GMODE                      ( (uint32_t)109 )
#define WLC_SET_GMODE                      ( (uint32_t)110 )
#define WLC_GET_LEGACY_ERP                 ( (uint32_t)111 )
#define WLC_SET_LEGACY_ERP                 ( (uint32_t)112 )
#define WLC_GET_RX_ANT                     ( (uint32_t)113 )
#define WLC_GET_CURR_RATESET               ( (uint32_t)114 )
#define WLC_GET_SCANSUPPRESS               ( (uint32_t)115 )
#define WLC_SET_SCANSUPPRESS               ( (uint32_t)116 )
#define WLC_GET_AP                         ( (uint32_t)117 )
#define WLC_SET_AP                         ( (uint32_t)118 )
#define WLC_GET_EAP_RESTRICT               ( (uint32_t)119 )
#define WLC_SET_EAP_RESTRICT               ( (uint32_t)120 )
#define WLC_SCB_AUTHORIZE                  ( (uint32_t)121 )
#define WLC_SCB_DEAUTHORIZE                ( (uint32_t)122 )
#define WLC_GET_WDSLIST                    ( (uint32_t)123 )
#define WLC_SET_WDSLIST                    ( (uint32_t)124 )
#define WLC_GET_ATIM                       ( (uint32_t)125 )
#define WLC_SET_ATIM                       ( (uint32_t)126 )
#define WLC_GET_RSSI                       ( (uint32_t)127 )
#define WLC_GET_PHYANTDIV                  ( (uint32_t)128 )
#define WLC_SET_PHYANTDIV                  ( (uint32_t)129 )
#define WLC_AP_RX_ONLY                     ( (uint32_t)130 )
#define WLC_GET_TX_PATH_PWR                ( (uint32_t)131 )
#define WLC_SET_TX_PATH_PWR                ( (uint32_t)132 )
#define WLC_GET_WSEC                       ( (uint32_t)133 )
#define WLC_SET_WSEC                       ( (uint32_t)134 )
#define WLC_GET_PHY_NOISE                  ( (uint32_t)135 )
#define WLC_GET_BSS_INFO                   ( (uint32_t)136 )
#define WLC_GET_PKTCNTS                    ( (uint32_t)137 )
#define WLC_GET_LAZYWDS                    ( (uint32_t)138 )
#define WLC_SET_LAZYWDS                    ( (uint32_t)139 )
#define WLC_GET_BANDLIST                   ( (uint32_t)140 )
#define WLC_GET_BAND                       ( (uint32_t)141 )
#define WLC_SET_BAND                       ( (uint32_t)142 )
#define WLC_SCB_DEAUTHENTICATE             ( (uint32_t)143 )
#define WLC_GET_SHORTSLOT                  ( (uint32_t)144 )
#define WLC_GET_SHORTSLOT_OVERRIDE         ( (uint32_t)145 )
#define WLC_SET_SHORTSLOT_OVERRIDE         ( (uint32_t)146 )
#define WLC_GET_SHORTSLOT_RESTRICT         ( (uint32_t)147 )
#define WLC_SET_SHORTSLOT_RESTRICT         ( (uint32_t)148 )
#define WLC_GET_GMODE_PROTECTION           ( (uint32_t)149 )
#define WLC_GET_GMODE_PROTECTION_OVERRIDE  ( (uint32_t)150 )
#define WLC_SET_GMODE_PROTECTION_OVERRIDE  ( (uint32_t)151 )
#define WLC_UPGRADE                        ( (uint32_t)152 )
#define WLC_GET_IGNORE_BCNS                ( (uint32_t)155 )
#define WLC_SET_IGNORE_BCNS                ( (uint32_t)156 )
#define WLC_GET_SCB_TIMEOUT                ( (uint32_t)157 )
#define WLC_SET_SCB_TIMEOUT                ( (uint32_t)158 )
#define WLC_GET_ASSOCLIST                  ( (uint32_t)159 )
#define WLC_GET_CLK                        ( (uint32_t)160 )
#define WLC_SET_CLK                        ( (uint32_t)161 )
#define WLC_GET_UP                         ( (uint32_t)162 )
#define WLC_OUT                            ( (uint32_t)163 )
#define WLC_GET_WPA_AUTH                   ( (uint32_t)164 )
#define WLC_SET_WPA_AUTH                   ( (uint32_t)165 )
#define WLC_GET_UCFLAGS                    ( (uint32_t)166 )
#define WLC_SET_UCFLAGS                    ( (uint32_t)167 )
#define WLC_GET_PWRIDX                     ( (uint32_t)168 )
#define WLC_SET_PWRIDX                     ( (uint32_t)169 )
#define WLC_GET_TSSI                       ( (uint32_t)170 )
#define WLC_GET_SUP_RATESET_OVERRIDE       ( (uint32_t)171 )
#define WLC_SET_SUP_RATESET_OVERRIDE       ( (uint32_t)172 )
#define WLC_GET_PROTECTION_CONTROL         ( (uint32_t)178 )
#define WLC_SET_PROTECTION_CONTROL         ( (uint32_t)179 )
#define WLC_GET_PHYLIST                    ( (uint32_t)180 )
#define WLC_ENCRYPT_STRENGTH               ( (uint32_t)181 )
#define WLC_DECRYPT_STATUS                 ( (uint32_t)182 )
#define WLC_GET_KEY_SEQ                    ( (uint32_t)183 )
#define WLC_GET_SCAN_CHANNEL_TIME          ( (uint32_t)184 )
#define WLC_SET_SCAN_CHANNEL_TIME          ( (uint32_t)185 )
#define WLC_GET_SCAN_UNASSOC_TIME          ( (uint32_t)186 )
#define WLC_SET_SCAN_UNASSOC_TIME          ( (uint32_t)187 )
#define WLC_GET_SCAN_HOME_TIME             ( (uint32_t)188 )
#define WLC_SET_SCAN_HOME_TIME             ( (uint32_t)189 )
#define WLC_GET_SCAN_NPROBES               ( (uint32_t)190 )
#define WLC_SET_SCAN_NPROBES               ( (uint32_t)191 )
#define WLC_GET_PRB_RESP_TIMEOUT           ( (uint32_t)192 )
#define WLC_SET_PRB_RESP_TIMEOUT           ( (uint32_t)193 )
#define WLC_GET_ATTEN                      ( (uint32_t)194 )
#define WLC_SET_ATTEN                      ( (uint32_t)195 )
#define WLC_GET_SHMEM                      ( (uint32_t)196 )
#define WLC_SET_SHMEM                      ( (uint32_t)197 )
#define WLC_SET_WSEC_TEST                  ( (uint32_t)200 )
#define WLC_SCB_DEAUTHENTICATE_FOR_REASON  ( (uint32_t)201 )
#define WLC_TKIP_COUNTERMEASURES           ( (uint32_t)202 )
#define WLC_GET_PIOMODE                    ( (uint32_t)203 )
#define WLC_SET_PIOMODE                    ( (uint32_t)204 )
#define WLC_SET_ASSOC_PREFER               ( (uint32_t)205 )
#define WLC_GET_ASSOC_PREFER               ( (uint32_t)206 )
#define WLC_SET_ROAM_PREFER                ( (uint32_t)207 )
#define WLC_GET_ROAM_PREFER                ( (uint32_t)208 )
#define WLC_SET_LED                        ( (uint32_t)209 )
#define WLC_GET_LED                        ( (uint32_t)210 )
#define WLC_GET_INTERFERENCE_MODE          ( (uint32_t)211 )
#define WLC_SET_INTERFERENCE_MODE          ( (uint32_t)212 )
#define WLC_GET_CHANNEL_QA                 ( (uint32_t)213 )
#define WLC_START_CHANNEL_QA               ( (uint32_t)214 )
#define WLC_GET_CHANNEL_SEL                ( (uint32_t)215 )
#define WLC_START_CHANNEL_SEL              ( (uint32_t)216 )
#define WLC_GET_VALID_CHANNELS             ( (uint32_t)217 )
#define WLC_GET_FAKEFRAG                   ( (uint32_t)218 )
#define WLC_SET_FAKEFRAG                   ( (uint32_t)219 )
#define WLC_GET_PWROUT_PERCENTAGE          ( (uint32_t)220 )
#define WLC_SET_PWROUT_PERCENTAGE          ( (uint32_t)221 )
#define WLC_SET_BAD_FRAME_PREEMPT          ( (uint32_t)222 )
#define WLC_GET_BAD_FRAME_PREEMPT          ( (uint32_t)223 )
#define WLC_SET_LEAP_LIST                  ( (uint32_t)224 )
#define WLC_GET_LEAP_LIST                  ( (uint32_t)225 )
#define WLC_GET_CWMIN                      ( (uint32_t)226 )
#define WLC_SET_CWMIN                      ( (uint32_t)227 )
#define WLC_GET_CWMAX                      ( (uint32_t)228 )
#define WLC_SET_CWMAX                      ( (uint32_t)229 )
#define WLC_GET_WET                        ( (uint32_t)230 )
#define WLC_SET_WET                        ( (uint32_t)231 )
#define WLC_GET_PUB                        ( (uint32_t)232 )
#define WLC_GET_KEY_PRIMARY                ( (uint32_t)235 )
#define WLC_SET_KEY_PRIMARY                ( (uint32_t)236 )
#define WLC_GET_ACI_ARGS                   ( (uint32_t)238 )
#define WLC_SET_ACI_ARGS                   ( (uint32_t)239 )
#define WLC_UNSET_CALLBACK                 ( (uint32_t)240 )
#define WLC_SET_CALLBACK                   ( (uint32_t)241 )
#define WLC_GET_RADAR                      ( (uint32_t)242 )
#define WLC_SET_RADAR                      ( (uint32_t)243 )
#define WLC_SET_SPECT_MANAGMENT            ( (uint32_t)244 )
#define WLC_GET_SPECT_MANAGMENT            ( (uint32_t)245 )
#define WLC_WDS_GET_REMOTE_HWADDR          ( (uint32_t)246 )
#define WLC_WDS_GET_WPA_SUP                ( (uint32_t)247 )
#define WLC_SET_CS_SCAN_TIMER              ( (uint32_t)248 )
#define WLC_GET_CS_SCAN_TIMER              ( (uint32_t)249 )
#define WLC_MEASURE_REQUEST                ( (uint32_t)250 )
#define WLC_INIT                           ( (uint32_t)251 )
#define WLC_SEND_QUIET                     ( (uint32_t)252 )
#define WLC_KEEPALIVE                      ( (uint32_t)253 )
#define WLC_SEND_PWR_CONSTRAINT            ( (uint32_t)254 )
#define WLC_UPGRADE_STATUS                 ( (uint32_t)255 )
#define WLC_CURRENT_PWR                    ( (uint32_t)256 )
#define WLC_GET_SCAN_PASSIVE_TIME          ( (uint32_t)257 )
#define WLC_SET_SCAN_PASSIVE_TIME          ( (uint32_t)258 )
#define WLC_LEGACY_LINK_BEHAVIOR           ( (uint32_t)259 )
#define WLC_GET_CHANNELS_IN_COUNTRY        ( (uint32_t)260 )
#define WLC_GET_COUNTRY_LIST               ( (uint32_t)261 )
#define WLC_GET_VAR                        ( (uint32_t)262 )
#define WLC_SET_VAR                        ( (uint32_t)263 )
#define WLC_NVRAM_GET                      ( (uint32_t)264 )
#define WLC_NVRAM_SET                      ( (uint32_t)265 )
#define WLC_NVRAM_DUMP                     ( (uint32_t)266 )
#define WLC_REBOOT                         ( (uint32_t)267 )
#define WLC_SET_WSEC_PMK                   ( (uint32_t)268 )
#define WLC_GET_AUTH_MODE                  ( (uint32_t)269 )
#define WLC_SET_AUTH_MODE                  ( (uint32_t)270 )
#define WLC_GET_WAKEENTRY                  ( (uint32_t)271 )
#define WLC_SET_WAKEENTRY                  ( (uint32_t)272 )
#define WLC_NDCONFIG_ITEM                  ( (uint32_t)273 )
#define WLC_NVOTPW                         ( (uint32_t)274 )
#define WLC_OTPW                           ( (uint32_t)275 )
#define WLC_IOV_BLOCK_GET                  ( (uint32_t)276 )
#define WLC_IOV_MODULES_GET                ( (uint32_t)277 )
#define WLC_SOFT_RESET                     ( (uint32_t)278 )
#define WLC_GET_ALLOW_MODE                 ( (uint32_t)279 )
#define WLC_SET_ALLOW_MODE                 ( (uint32_t)280 )
#define WLC_GET_DESIRED_BSSID              ( (uint32_t)281 )
#define WLC_SET_DESIRED_BSSID              ( (uint32_t)282 )
#define WLC_DISASSOC_MYAP                  ( (uint32_t)283 )
#define WLC_GET_NBANDS                     ( (uint32_t)284 )
#define WLC_GET_BANDSTATES                 ( (uint32_t)285 )
#define WLC_GET_WLC_BSS_INFO               ( (uint32_t)286 )
#define WLC_GET_ASSOC_INFO                 ( (uint32_t)287 )
#define WLC_GET_OID_PHY                    ( (uint32_t)288 )
#define WLC_SET_OID_PHY                    ( (uint32_t)289 )
#define WLC_SET_ASSOC_TIME                 ( (uint32_t)290 )
#define WLC_GET_DESIRED_SSID               ( (uint32_t)291 )
#define WLC_GET_CHANSPEC                   ( (uint32_t)292 )
#define WLC_GET_ASSOC_STATE                ( (uint32_t)293 )
#define WLC_SET_PHY_STATE                  ( (uint32_t)294 )
#define WLC_GET_SCAN_PENDING               ( (uint32_t)295 )
#define WLC_GET_SCANREQ_PENDING            ( (uint32_t)296 )
#define WLC_GET_PREV_ROAM_REASON           ( (uint32_t)297 )
#define WLC_SET_PREV_ROAM_REASON           ( (uint32_t)298 )
#define WLC_GET_BANDSTATES_PI              ( (uint32_t)299 )
#define WLC_GET_PHY_STATE                  ( (uint32_t)300 )
#define WLC_GET_BSS_WPA_RSN                ( (uint32_t)301 )
#define WLC_GET_BSS_WPA2_RSN               ( (uint32_t)302 )
#define WLC_GET_BSS_BCN_TS                 ( (uint32_t)303 )
#define WLC_GET_INT_DISASSOC               ( (uint32_t)304 )
#define WLC_SET_NUM_PEERS                  ( (uint32_t)305 )
#define WLC_GET_NUM_BSS                    ( (uint32_t)306 )
#define WLC_GET_WSEC_PMK                   ( (uint32_t)318 )
#define WLC_GET_RANDOM_BYTES               ( (uint32_t)319 )
#define WLC_LAST                           ( (uint32_t)320 )

// EOF
