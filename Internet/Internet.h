#pragma once

#include <vector>
#include <string>
#include <guiddef.h>
#include <atlstr.h>
#include <wlanapi.h>


struct WIFI_AP
{
	// Interface
	GUID m_guidInterface;								// {D661F997-AB45-4A92-A123-8AD38C8D1C79}
	CStringW m_strInterfaceDescription;					// Realtek RTL8852BE WiFi 6 802.11ax PCIe Adapter

	WLAN_INTERFACE_STATE m_eInterfaceState;				// wlan_interface_state_connected

	// 以下信息，在m_eInterfaceState为wlan_interface_state_connected时有效
	WLAN_CONNECTION_MODE m_eConnectionMode;				// Connection Mode : A profile is used to make the connection
	CStringW m_strProfileName;							// Profile name used : motoa

	// Association Attributes for this connection
	CStringW m_strSSID;									//	SSID: motoa
	CStringW m_strIPv4;
	DOT11_BSS_TYPE m_eBSSNetworkType;					//	BSS Network type : Infrastructure
	CStringW m_strMAC;									//	MAC address : BE - F2 - F1 - FE - 80 - ED
	DOT11_PHY_TYPE m_ePHYNetworkType;					//	PHY network type : 802.11n PHY type
	ULONG m_ulPHYIndex = 0;								//	PHY index : 0
	WLAN_SIGNAL_QUALITY m_ulSignalQuality = 0;			//	Signal Quality : 90
	ULONG m_ulReceivingRate = 0;							//	Receiving Rate : 72200
	ULONG m_ulTransmissionRate;							// 	Transmission Rate : 72200

	// Security Attributes for this connection
	BOOL m_bSecurityEnabled;							//	Security enabled : Yes
	BOOL m_b8021XEnable;								//	802.1X enabled : No
	DOT11_AUTH_ALGORITHM m_eAuthenticationAlgorithm;	// 	Authentication Algorithm : Other(9)
	DOT11_CIPHER_ALGORITHM m_eCipherAlgorithm;			// 	Cipher Algorithm : CCMP
};

struct WLAN_INTERFACE
{
	// Current Index
	int m_nCurrentIndex = 0;

	std::vector<WIFI_AP> m_vWifi;
};

struct SAVED_WIFI
{
	CStringW m_strSSID;
	CStringW m_strPassword;
};

int GetConnectedWiFiSSID(WLAN_INTERFACE* pWifi);
int GetCurrentWifiIp(std::vector<std::string>* pvIp);
int GetSavedWifiList(std::vector<SAVED_WIFI>* pvWifiList);
