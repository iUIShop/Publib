#include "Internet.h"
#include <iostream>
#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <Iphlpapi.h>
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "iphlpapi.lib")

// 代码来自MSDN. 如果是24H2及之后的版本，需要开启位置权限：
// 【设置 | 隐私和安全性 】:
// 1. 开启【定位服务】
// 2. 开启【允许应用访问位置】
// 3. 开启【让桌面应用程序访问位置】
// 某次输出如下：
//Num Entries: 1
//Current Index: 0
//  Interface Index[0]:    0
//  InterfaceGUID[0]:      {D661F997-AB45-4A92-A123-8AD38C8D1C79}
//  Interface Description[0]: Realtek RTL8852BE WiFi 6 802.11ax PCIe Adapter
//  Interface State[0]:    Connected
//
//  WLAN_CONNECTION_ATTRIBUTES for this interface
//  Interface State:       Connected
//  Connection Mode:       A profile is used to make the connection
//  Profile name used:     TP-LINK_A18F
//  Association Attributes for this connection
//    SSID:                TP-LINK_A18F
//    BSS Network type:    Infrastructure
//    MAC address:         CC-08-FB-D8-A1-91
//    PHY network type:    Unknown = 8
//    PHY index:           4
//    Signal Quality:      93
//    Receiving Rate:      866700
//    Transmission Rate:   866700
//
//  Security Attributes for this connection
//    Security enabled:    Yes
//    802.1X enabled:      No
//    Authentication Algorithm: RSNA with PSK
//    Cipher Algorithm:    CCMP
int PrintConnectedWiFi()
{
	HANDLE hClient = NULL;
	DWORD dwMaxClient = 2;      //    
	DWORD dwCurVersion = 0;
	DWORD dwResult = 0;
	DWORD dwRetVal = 0;
	int iRet = 0;

	WCHAR GuidString[39] = { 0 };

	unsigned int i, k;

	// variables used for WlanEnumInterfaces

	PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
	PWLAN_INTERFACE_INFO pIfInfo = NULL;

	// variables used for WlanQueryInterfaces for opcode = wlan_intf_opcode_current_connection
	PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
	DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
	WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;

	dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
	if (dwResult != ERROR_SUCCESS) {
		wprintf(L"WlanOpenHandle failed with error: %u\n", dwResult);
		return 1;
		// You can use FormatMessage here to find out why the function failed
	}

	dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
	if (dwResult != ERROR_SUCCESS) {
		wprintf(L"WlanEnumInterfaces failed with error: %u\n", dwResult);
		return 1;
		// You can use FormatMessage here to find out why the function failed
	}
	else {
		wprintf(L"Num Entries: %lu\n", pIfList->dwNumberOfItems);
		wprintf(L"Current Index: %lu\n", pIfList->dwIndex);
		for (i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
			pIfInfo = (WLAN_INTERFACE_INFO*)&pIfList->InterfaceInfo[i];
			wprintf(L"  Interface Index[%u]:\t %lu\n", i, i);
			iRet =
				StringFromGUID2(pIfInfo->InterfaceGuid, (LPOLESTR)&GuidString,
					sizeof(GuidString) / sizeof(*GuidString));
			// For c rather than C++ source code, the above line needs to be
			// iRet = StringFromGUID2(&pIfInfo->InterfaceGuid, (LPOLESTR) &GuidString, 
			//     sizeof(GuidString)/sizeof(*GuidString)); 
			if (iRet == 0)
				wprintf(L"StringFromGUID2 failed\n");
			else {
				wprintf(L"  InterfaceGUID[%d]:\t %ws\n", i, GuidString);
			}
			wprintf(L"  Interface Description[%d]: %ws", i, pIfInfo->strInterfaceDescription);
			wprintf(L"\n");
			wprintf(L"  Interface State[%d]:\t ", i);
			switch (pIfInfo->isState) {
			case wlan_interface_state_not_ready:
				wprintf(L"Not ready\n");
				break;
			case wlan_interface_state_connected:
				wprintf(L"Connected\n");
				break;
			case wlan_interface_state_ad_hoc_network_formed:
				wprintf(L"First node in a ad hoc network\n");
				break;
			case wlan_interface_state_disconnecting:
				wprintf(L"Disconnecting\n");
				break;
			case wlan_interface_state_disconnected:
				wprintf(L"Not connected\n");
				break;
			case wlan_interface_state_associating:
				wprintf(L"Attempting to associate with a network\n");
				break;
			case wlan_interface_state_discovering:
				wprintf(L"Auto configuration is discovering settings for the network\n");
				break;
			case wlan_interface_state_authenticating:
				wprintf(L"In process of authenticating\n");
				break;
			default:
				wprintf(L"Unknown state %ld\n", pIfInfo->isState);
				break;
			}
			wprintf(L"\n");

			// If interface state is connected, call WlanQueryInterface
			// to get current connection attributes
			if (pIfInfo->isState == wlan_interface_state_connected) {
				dwResult = WlanQueryInterface(hClient,
					&pIfInfo->InterfaceGuid,
					wlan_intf_opcode_current_connection,
					NULL,
					&connectInfoSize,
					(PVOID*)&pConnectInfo,
					&opCode);

				if (dwResult != ERROR_SUCCESS) {
					wprintf(L"WlanQueryInterface failed with error: %u\n", dwResult);
					dwRetVal = 1;
					// You can use FormatMessage to find out why the function failed
				}
				else {
					wprintf(L"  WLAN_CONNECTION_ATTRIBUTES for this interface\n");

					wprintf(L"  Interface State:\t ");
					switch (pConnectInfo->isState) {
					case wlan_interface_state_not_ready:
						wprintf(L"Not ready\n");
						break;
					case wlan_interface_state_connected:
						wprintf(L"Connected\n");
						break;
					case wlan_interface_state_ad_hoc_network_formed:
						wprintf(L"First node in a ad hoc network\n");
						break;
					case wlan_interface_state_disconnecting:
						wprintf(L"Disconnecting\n");
						break;
					case wlan_interface_state_disconnected:
						wprintf(L"Not connected\n");
						break;
					case wlan_interface_state_associating:
						wprintf(L"Attempting to associate with a network\n");
						break;
					case wlan_interface_state_discovering:
						wprintf
						(L"Auto configuration is discovering settings for the network\n");
						break;
					case wlan_interface_state_authenticating:
						wprintf(L"In process of authenticating\n");
						break;
					default:
						wprintf(L"Unknown state %ld\n", pIfInfo->isState);
						break;
					}

					wprintf(L"  Connection Mode:\t ");
					switch (pConnectInfo->wlanConnectionMode) {
					case wlan_connection_mode_profile:
						wprintf(L"A profile is used to make the connection\n");
						break;
					case wlan_connection_mode_temporary_profile:
						wprintf(L"A temporary profile is used to make the connection\n");
						break;
					case wlan_connection_mode_discovery_secure:
						wprintf(L"Secure discovery is used to make the connection\n");
						break;
					case wlan_connection_mode_discovery_unsecure:
						wprintf(L"Unsecure discovery is used to make the connection\n");
						break;
					case wlan_connection_mode_auto:
						wprintf
						(L"connection initiated by wireless service automatically using a persistent profile\n");
						break;
					case wlan_connection_mode_invalid:
						wprintf(L"Invalid connection mode\n");
						break;
					default:
						wprintf(L"Unknown connection mode %ld\n",
							pConnectInfo->wlanConnectionMode);
						break;
					}

					wprintf(L"  Profile name used:\t %ws\n", pConnectInfo->strProfileName);

					wprintf(L"  Association Attributes for this connection\n");
					wprintf(L"    SSID:\t\t ");
					if (pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength == 0)
						wprintf(L"\n");
					else {
						for (k = 0;
							k < pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength;
							k++) {
							wprintf(L"%c",
								(int)pConnectInfo->wlanAssociationAttributes.dot11Ssid.
								ucSSID[k]);
						}
						wprintf(L"\n");
					}

					wprintf(L"    BSS Network type:\t ");
					switch (pConnectInfo->wlanAssociationAttributes.dot11BssType) {
					case dot11_BSS_type_infrastructure:
						wprintf(L"Infrastructure\n");
						break;
					case dot11_BSS_type_independent:
						wprintf(L"Infrastructure\n");
						break;
					default:
						wprintf(L"Other = %lu\n",
							pConnectInfo->wlanAssociationAttributes.dot11BssType);
						break;
					}

					wprintf(L"    MAC address:\t ");
					for (k = 0; k < sizeof(pConnectInfo->wlanAssociationAttributes.dot11Bssid);
						k++) {
						if (k == 5)
							wprintf(L"%.2X\n",
								pConnectInfo->wlanAssociationAttributes.dot11Bssid[k]);
						else
							wprintf(L"%.2X-",
								pConnectInfo->wlanAssociationAttributes.dot11Bssid[k]);
					}

					wprintf(L"    PHY network type:\t ");
					switch (pConnectInfo->wlanAssociationAttributes.dot11PhyType) {
					case dot11_phy_type_fhss:
						wprintf(L"Frequency-hopping spread-spectrum (FHSS)\n");
						break;
					case dot11_phy_type_dsss:
						wprintf(L"Direct sequence spread spectrum (DSSS)\n");
						break;
					case dot11_phy_type_irbaseband:
						wprintf(L"Infrared (IR) baseband\n");
						break;
					case dot11_phy_type_ofdm:
						wprintf(L"Orthogonal frequency division multiplexing (OFDM)\n");
						break;
					case dot11_phy_type_hrdsss:
						wprintf(L"High-rate DSSS (HRDSSS) = \n");
						break;
					case dot11_phy_type_erp:
						wprintf(L"Extended rate PHY type\n");
						break;
					case dot11_phy_type_ht:
						wprintf(L"802.11n PHY type\n");
						break;
					default:
						wprintf(L"Unknown = %lu\n",
							pConnectInfo->wlanAssociationAttributes.dot11PhyType);
						break;
					}

					wprintf(L"    PHY index:\t\t %u\n",
						pConnectInfo->wlanAssociationAttributes.uDot11PhyIndex);

					wprintf(L"    Signal Quality:\t %d\n",
						pConnectInfo->wlanAssociationAttributes.wlanSignalQuality);

					wprintf(L"    Receiving Rate:\t %ld\n",
						pConnectInfo->wlanAssociationAttributes.ulRxRate);

					wprintf(L"    Transmission Rate:\t %ld\n",
						pConnectInfo->wlanAssociationAttributes.ulTxRate);
					wprintf(L"\n");

					wprintf(L"  Security Attributes for this connection\n");

					wprintf(L"    Security enabled:\t ");
					if (pConnectInfo->wlanSecurityAttributes.bSecurityEnabled == 0)
						wprintf(L"No\n");
					else
						wprintf(L"Yes\n");

					wprintf(L"    802.1X enabled:\t ");
					if (pConnectInfo->wlanSecurityAttributes.bOneXEnabled == 0)
						wprintf(L"No\n");
					else
						wprintf(L"Yes\n");

					wprintf(L"    Authentication Algorithm: ");
					switch (pConnectInfo->wlanSecurityAttributes.dot11AuthAlgorithm) {
					case DOT11_AUTH_ALGO_80211_OPEN:
						wprintf(L"802.11 Open\n");
						break;
					case DOT11_AUTH_ALGO_80211_SHARED_KEY:
						wprintf(L"802.11 Shared\n");
						break;
					case DOT11_AUTH_ALGO_WPA:
						wprintf(L"WPA\n");
						break;
					case DOT11_AUTH_ALGO_WPA_PSK:
						wprintf(L"WPA-PSK\n");
						break;
					case DOT11_AUTH_ALGO_WPA_NONE:
						wprintf(L"WPA-None\n");
						break;
					case DOT11_AUTH_ALGO_RSNA:
						wprintf(L"RSNA\n");
						break;
					case DOT11_AUTH_ALGO_RSNA_PSK:
						wprintf(L"RSNA with PSK\n");
						break;
					default:
						wprintf(L"Other (%lu)\n", pConnectInfo->wlanSecurityAttributes.dot11AuthAlgorithm);
						break;
					}

					wprintf(L"    Cipher Algorithm:\t ");
					switch (pConnectInfo->wlanSecurityAttributes.dot11CipherAlgorithm) {
					case DOT11_CIPHER_ALGO_NONE:
						wprintf(L"None\n");
						break;
					case DOT11_CIPHER_ALGO_WEP40:
						wprintf(L"WEP-40\n");
						break;
					case DOT11_CIPHER_ALGO_TKIP:
						wprintf(L"TKIP\n");
						break;
					case DOT11_CIPHER_ALGO_CCMP:
						wprintf(L"CCMP\n");
						break;
					case DOT11_CIPHER_ALGO_WEP104:
						wprintf(L"WEP-104\n");
						break;
					case DOT11_CIPHER_ALGO_WEP:
						wprintf(L"WEP\n");
						break;
					default:
						wprintf(L"Other (0x%x)\n", pConnectInfo->wlanSecurityAttributes.dot11CipherAlgorithm);
						break;
					}
					wprintf(L"\n");
				}
			}
		}

	}
	if (pConnectInfo != NULL) {
		WlanFreeMemory(pConnectInfo);
		pConnectInfo = NULL;
	}

	if (pIfList != NULL) {
		WlanFreeMemory(pIfList);
		pIfList = NULL;
	}

	return dwRetVal;
}

int GetConnectedWiFiSSID(WLAN_INTERFACE* pWifi)
{
	int nRet = 0;
	HANDLE hClient = NULL;
	DWORD dwMaxClient = 2;
	DWORD dwCurVersion = 0;
	DWORD dwResult = 0;
	int iRet = 0;
	WCHAR GuidString[39] = { 0 };
	unsigned int i = 0;
	unsigned int k = 0;
	PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
	PWLAN_INTERFACE_INFO pIfInfo = NULL;
	PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
	DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
	WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;

	do
	{
		if (nullptr == pWifi)
		{
			nRet = -1;
			break;
		}

		dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
		if (dwResult != ERROR_SUCCESS)
		{
			nRet = -2;
			break;
		}

		dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
		if (dwResult != ERROR_SUCCESS)
		{
			nRet = -3;
			break;
		}

		pWifi->m_nCurrentIndex = pIfList->dwIndex;
		for (i = 0; i < (int)pIfList->dwNumberOfItems; i++)
		{
			WIFI_AP info;
			pIfInfo = (WLAN_INTERFACE_INFO*)&pIfList->InterfaceInfo[i];

			info.m_guidInterface = pIfInfo->InterfaceGuid;
			info.m_strInterfaceDescription = pIfInfo->strInterfaceDescription;
			info.m_eInterfaceState = pIfInfo->isState;


			// If interface state is connected, call WlanQueryInterface
			// to get current connection attributes
			if (pIfInfo->isState == wlan_interface_state_connected)
			{
				dwResult = WlanQueryInterface(hClient,
					&pIfInfo->InterfaceGuid,
					wlan_intf_opcode_current_connection,
					NULL,
					&connectInfoSize,
					(PVOID*)&pConnectInfo,
					&opCode);

				if (dwResult != ERROR_SUCCESS)
				{
					nRet = -4;
					break;
				}
				else
				{
					info.m_eConnectionMode = pConnectInfo->wlanConnectionMode;
					info.m_strProfileName = pConnectInfo->strProfileName;
					info.m_strSSID = pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID;
					info.m_eBSSNetworkType = pConnectInfo->wlanAssociationAttributes.dot11BssType;

					for (k = 0; k < sizeof(pConnectInfo->wlanAssociationAttributes.dot11Bssid); k++)
					{
						CStringW s;
						if (k == 5)
						{
							s.Format(L"%.2X", pConnectInfo->wlanAssociationAttributes.dot11Bssid[k]);
						}
						else
						{
							s.Format(L"%.2X-", pConnectInfo->wlanAssociationAttributes.dot11Bssid[k]);
						}
						info.m_strMAC.Append(s);
					}

					info.m_ePHYNetworkType = pConnectInfo->wlanAssociationAttributes.dot11PhyType;
					info.m_ulPHYIndex = pConnectInfo->wlanAssociationAttributes.uDot11PhyIndex;
					info.m_ulSignalQuality = pConnectInfo->wlanAssociationAttributes.wlanSignalQuality;
					info.m_ulReceivingRate = pConnectInfo->wlanAssociationAttributes.ulRxRate;
					info.m_ulTransmissionRate = pConnectInfo->wlanAssociationAttributes.ulTxRate;
					info.m_bSecurityEnabled = pConnectInfo->wlanSecurityAttributes.bSecurityEnabled;
					info.m_b8021XEnable = pConnectInfo->wlanSecurityAttributes.bOneXEnabled;
					info.m_eAuthenticationAlgorithm = pConnectInfo->wlanSecurityAttributes.dot11AuthAlgorithm;
					info.m_eCipherAlgorithm = pConnectInfo->wlanSecurityAttributes.dot11CipherAlgorithm;
				}

				pWifi->m_vWifi.push_back(info);
			}
		}
	} while (false);

	if (pConnectInfo != NULL)
	{
		WlanFreeMemory(pConnectInfo);
		pConnectInfo = NULL;
	}

	if (pIfList != NULL)
	{
		WlanFreeMemory(pIfList);
		pIfList = NULL;
	}

	if (nullptr != hClient)
	{
		WlanCloseHandle(hClient, nullptr);
		hClient = nullptr;
	}

	return nRet;
}

int GetCurrentWifiIp(std::vector<std::string>* pvIp)
{
	PIP_ADAPTER_INFO pAdapterInfo = nullptr;

	do
	{
		// 初始化 Winsock
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			std::cerr << "WSAStartup failed." << std::endl;
			break;
		}

		// 获取适配器信息
		ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
		if (pAdapterInfo == NULL)
		{
			std::cerr << "Error allocating memory needed to call GetAdaptersinfo" << std::endl;
			break;
		}

		// 获取适配器信息
		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
		{
			free(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
			if (pAdapterInfo == NULL)
			{
				std::cerr << "Error allocating memory needed to call GetAdaptersinfo" << std::endl;
				break;
			}
		}

		// 获取适配器信息
		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
		{
			PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
			while (pAdapter)
			{
				if (pAdapter->Type == IF_TYPE_IEEE80211 && pAdapter->LeaseExpires != 0)
				{
					pvIp->push_back(pAdapter->IpAddressList.IpAddress.String);
				}
				pAdapter = pAdapter->Next;
			}
		}
		else
		{
			std::cerr << "GetAdaptersInfo failed with error: " << GetLastError() << std::endl;
		}
	} while (false);

	// 释放内存
	if (pAdapterInfo)
	{
		free(pAdapterInfo);
	}

	// 清理 Winsock
	WSACleanup();

	return 0;
}

// From deepseek.
int GetSavedWifiList(std::vector<SAVED_WIFI>* pvWifiList)
{
	if (nullptr == pvWifiList)
	{
		return -1;
	}

	HANDLE hClient = NULL;
	DWORD dwMaxClient = 2;
	DWORD dwCurVersion = 0;
	DWORD dwResult = 0;
	PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
	PWLAN_PROFILE_INFO_LIST pProfileList = NULL;

	// 初始化WLAN API
	dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
	if (dwResult != ERROR_SUCCESS) {
		printf("WlanOpenHandle failed with error: %u\n", dwResult);
		return 1;
	}

	// 枚举无线接口
	dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
	if (dwResult != ERROR_SUCCESS) {
		printf("WlanEnumInterfaces failed with error: %u\n", dwResult);
		WlanCloseHandle(hClient, NULL);
		return 1;
	}

	// 遍历每个无线接口
	for (DWORD i = 0; i < pIfList->dwNumberOfItems; i++) {
		WLAN_INTERFACE_INFO wlanIfInfo = pIfList->InterfaceInfo[i];

		printf("Interface: %S\n", wlanIfInfo.strInterfaceDescription);

		// 获取此接口的所有配置文件
		dwResult = WlanGetProfileList(hClient, &wlanIfInfo.InterfaceGuid, NULL, &pProfileList);
		if (dwResult != ERROR_SUCCESS) {
			printf("WlanGetProfileList failed with error: %u\n", dwResult);
			continue;
		}

		// 遍历每个配置文件
		for (DWORD j = 0; j < pProfileList->dwNumberOfItems; j++) {
			WLAN_PROFILE_INFO profileInfo = pProfileList->ProfileInfo[j];
			DWORD dwFlags = WLAN_PROFILE_GET_PLAINTEXT_KEY;
			DWORD dwAccess = 0;
			DWORD dwGrantedAccess = 0;
			LPWSTR pProfileXml = NULL;

			printf("\tProfile Name: %S\n", profileInfo.strProfileName);

			// 获取配置文件XML内容（包含密码）
			dwResult = WlanGetProfile(hClient, &wlanIfInfo.InterfaceGuid,
				profileInfo.strProfileName,
				NULL, &pProfileXml, &dwFlags, &dwGrantedAccess);

			if (dwResult == ERROR_SUCCESS) {
				printf("\tProfile XML:\n%S\n", pProfileXml);
				// 这里可以解析XML获取明文密码
				// 密码位于<sharedKey><keyMaterial>标签中
				// 由于整个xml文件中，只有一处<keyMaterial>，所以我们可以简单的通过字符串处理来解析密码
				CStringW strProfileXml = pProfileXml;
				int nPos1 = strProfileXml.Find(L"<keyMaterial>");
				int nPos2 = strProfileXml.Find(L"</keyMaterial>");
				if (nPos2 > nPos1 && nPos1 > 0)
				{
					int nLength = CStringW(L"<keyMaterial>").GetLength();
					CStringW strPassword = strProfileXml.Mid(nPos1 + nLength, nPos2 - nPos1 - nLength);

					SAVED_WIFI savedWifi;
					savedWifi.m_strSSID = profileInfo.strProfileName;
					savedWifi.m_strPassword = strPassword;

					pvWifiList->push_back(savedWifi);
				}
			}
			else {
				printf("\tWlanGetProfile failed with error: %u\n", dwResult);
			}

			if (pProfileXml != NULL) {
				WlanFreeMemory(pProfileXml);
			}
		}

		if (pProfileList != NULL) {
			WlanFreeMemory(pProfileList);
			pProfileList = NULL;
		}
	}

	// 清理资源
	if (pIfList != NULL) {
		WlanFreeMemory(pIfList);
	}

	WlanCloseHandle(hClient, NULL);

	return 0;
}

